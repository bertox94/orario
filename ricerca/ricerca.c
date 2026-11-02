#include "ricerca.h"

#include <stdlib.h>

#include "../data_initialization/database_initializers.h"
#include "../orario/orario.h"
#include "../printer/printer.h"
#include "../priority_queue/prio_queue.h"
#include "diagnosi.h"
#include "vincoli.h"

// Come funziona: cerca_orario(pos) decide lo slot pos e chiama se stessa per pos + 1. Per ogni docente possibile
// (da solo o con un codocente) aggiorna lo stato (prep_*), prosegue, e poi annulla esattamente quello che ha fatto
// (post_*): cosi' tornando indietro lo stato e' sempre quello dello slot corrente. Uno slot senza docenti possibili
// (e che non puo' restare vuoto) e' una marcia indietro. Prima di decidere uno slot, la potatura (vincoli.c) scarta
// i rami che non possono piu' avere soluzioni.

long long int n_gen = 0;
long long int n_gen_max = SCHEDULES_PER_GB / 5;
long long int n_backtrack = 0;
int stampa_backtrack = 0;
FILE *file_orario;

/* ---- coda dei docenti di uno slot ---- */

// Comparison function for qsort
static int compare_queue_items(const void *a, const void *b) {
    // Cast the void pointers to pointers to QueueItem*
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;

    // Sort by priority (descending: highest priority first)
    if (itemA->priority < itemB->priority) return 1;
    if (itemA->priority > itemB->priority) return -1;
    return 0;
}

// Ordina la copia della coda di uno slot (da start, n docenti) per priorita' decrescente: si prova prima chi ha piu'
// ore ancora da fare. A parita' di priorita' l'ordine non e' fissato (qsort non e' stabile) e dipende da come la copia
// era ordinata la volta prima: per questo cambiare la potatura puo' cambiare l'ordine in cui si trovano gli orari.
static void sort_queue_mul(size_t start, size_t n) {
    // qsort has an average time complexity of O(N log N)
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

/* ---- assegnare uno slot e annullare ---- */

// Quante volte e' stato assegnato uno slot (in qualunque punto della ricerca). Serve a capire, senza una variabile
// per nodo, se un nodo ha trovato almeno un docente: le chiamate piu' profonde partono solo dopo un'assegnazione
// fatta da quel nodo, quindi il contatore cresce se e solo se il nodo ha assegnato qualcuno.
static long long int n_assegnazioni = 0;

// Operazioni comuni a ogni docente di un'assegnazione (da solo o in coppia) e il loro annullamento.
static inline void prep_chiamata_ric_basic(const teaching_slot *slot, QueueItem *it) {
    const int p = it->teacher;
    it->priority--;
    prof_gia_impegnato_giorno_ora[slot->base3d_goi + p] = 1;
    classe_docente[slot->base2d_ci + p].svolte++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + p]++;
}

static inline void post_chiamata_ric_basic(const teaching_slot *slot, QueueItem *it) {
    const int p = it->teacher;
    it->priority++;
    prof_gia_impegnato_giorno_ora[slot->base3d_goi + p] = 0;
    classe_docente[slot->base2d_ci + p].svolte--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + p]--;
}

// Le funzioni prep/post fanno tutto l'aggiornamento di stato di un'assegnazione di uno slot e il suo annullamento:
// l'assegnazione dei docenti nello slot (insegnante1/insegnante2), le operazioni per docente (basic), quelle
// specifiche (ore da solo, codocenza) e, una volta per slot, classe_info[].svolte e il contatore n_assegnazioni.
// post rimette lo slot a "non assegnato" (-1, -1): uno slot fuori dal percorso corrente e' sempre -1/-1.
// jt == NULL: it insegna da solo; altrimenti it e jt insegnano insieme (codocenza).
static inline void prep_chiamata_ricorsiva(teaching_slot *slot, QueueItem *it, QueueItem *jt) {
    prep_chiamata_ric_basic(slot, it);
    if (jt) {
        prep_chiamata_ric_basic(slot, jt);
        codocenza[idx_codocenza(slot->classe, it->teacher, jt->teacher)].svolte++;
    } else {
        classe_docente[slot->base2d_ci + it->teacher].alone_svolte++;
    }
    slot->insegnante1 = it->teacher;
    slot->insegnante2 = jt ? jt->teacher : -1;
    classe_info[slot->classe].svolte++;
    n_assegnazioni++;
}

static inline void post_chiamata_ricorsiva(teaching_slot *slot, QueueItem *it, QueueItem *jt) {
    post_chiamata_ric_basic(slot, it);
    if (jt) {
        post_chiamata_ric_basic(slot, jt);
        codocenza[idx_codocenza(slot->classe, it->teacher, jt->teacher)].svolte--;
    } else {
        classe_docente[slot->base2d_ci + it->teacher].alone_svolte--;
    }
    slot->insegnante1 = -1;
    slot->insegnante2 = -1;
    classe_info[slot->classe].svolte--;
}

// Assegna lo slot (da solo o in coppia), prosegue la ricerca e annulla.
static inline void assegna_e_prosegui(teaching_slot *slot, const int pos, QueueItem *it, QueueItem *jt) {
    prep_chiamata_ricorsiva(slot, it, jt);
    cerca_orario(pos + 1);
    post_chiamata_ricorsiva(slot, it, jt);
}

/* ---- un nodo della ricerca ---- */

// Un orario completo: lo si conta e lo si scrive.
static void registra_orario_completo(const int pos) {
    n_gen++;
    print(orario, pos, n_gen, file_orario);
}

// Nessun docente utilizzabile per questo slot: si torna indietro. Lo si conta e, se richiesto, lo si scrive.
static void registra_marcia_indietro(const teaching_slot *slot, const int pos, const int queue_size) {
    n_backtrack++;
    diagnosi_marcia_indietro(slot, pos, slot->base4d_queue, queue_size);
    if (stampa_backtrack) {
        fprintf(file_orario, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                giorni_settimana[slot->giorno], ore_giornaliere[slot->ora], classe_sezione[slot->classe]);
        print(orario, pos, n_gen, file_orario);
    }
}

// Prova a mettere nello slot il docente it insieme a ciascun codocente utilizzabile che lo segue in coda.
static void prova_con_codocenti(teaching_slot *slot, const int pos, const int ii, const int queue_size) {
    QueueItem *it = queue_mul[ii];
    const int p1 = it->teacher;
    for (int jj = ii + 1; jj < queue_size; jj++) {
        QueueItem *jt = queue_mul[jj];
        const int p2 = jt->teacher;
        if (prof_è_disponibile(slot->base3d_goi, p2)
            && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p1, p2)
            // poiche un prof se deve fare due ore no può farle spezzettate, e sabato è l'ultimo giorno,
            // non si puo mettere un altro prof: la seconda ora deve farla subito
            && sabato_esaurisci_priorita_subito(pos, slot->giorno, slot->ora, slot->base2d_ci, p1, p2)
            && docente_idoneo(slot, pos, p2)) {
            assegna_e_prosegui(slot, pos, it, jt);
        }
    }
}

// Prova a mettere nello slot il docente it da solo, se gli mancano ore da fare senza codocente.
static void prova_da_solo(teaching_slot *slot, const int pos, QueueItem *it) {
    const int p1 = it->teacher;
    if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(slot->base2d_ci, p1)
        && sabato_esaurisci_priorita_subito(pos, slot->giorno, slot->ora, slot->base2d_ci, p1, -1)) {
        assegna_e_prosegui(slot, pos, it, NULL);
    }
}

// Prova ogni docente della coda come docente dello slot: prima con i codocenti, poi da solo.
static void prova_tutti_i_docenti(teaching_slot *slot, const int pos, const int queue_size) {
    for (int ii = slot->base4d_queue; ii < queue_size; ii++) {
        QueueItem *it = queue_mul[ii];
        const int p1 = it->teacher;
        if (prof_è_disponibile(slot->base3d_goi, p1) // non e' gia' in un'altra classe nella stessa ora
            // ha ancora ore da fare in questa classe
            && deve_ancora_svolgere_ore_in_questa_classe(slot->base2d_ci, p1)
            && docente_idoneo(slot, pos, p1)) {
            prova_con_codocenti(slot, pos, ii, queue_size);
            //non abbiamo trovato combinazioni di codocenza, verifica se all'insegnante mancano ore di docenza da solo
            prova_da_solo(slot, pos, it);
        }
    }
}

// Prova a lasciare lo slot vuoto (ora buca, solo oltre le ore obbligatorie): lo slot e' gia' -1/-1 (vedi post_chiamata_*).
// Dopo la buca la classe non fa altre lezioni quel giorno: le ore che le mancano devono stare nei giorni dopo.
static void prova_ora_buca(const teaching_slot *slot, const int pos) {
    const int cla = slot->classe;
    if (slot->ora >= PRIME_N_ORE_OBBLIGATORIE
        && classe_info[cla].ore - classe_info[cla].svolte
           <= (actual_giorni_settimana - slot->giorno - 1) * actual_ore_giornaliere) {
        n_ore_buche[slot->base2d_gc + cla]++;
        cerca_orario(pos + 1);
        n_ore_buche[slot->base2d_gc + cla]--;
    }
}

// Prova tutti i docenti per questo slot; se nessuno va bene e' una marcia indietro.
static void prova_slot_con_docenti(teaching_slot *slot, const int pos) {
    // se alla fine il contatore non e' cresciuto, questo nodo non ha assegnato nessun docente (marcia indietro)
    const long long int assegnazioni_prima = n_assegnazioni;
    const int queue_size = slot->base4d_queue + queue_sz;
    prova_tutti_i_docenti(slot, pos, queue_size);
    if (n_assegnazioni == assegnazioni_prima) registra_marcia_indietro(slot, pos, queue_size);
}

void cerca_orario(const int pos) {
    // gia' trovati abbastanza orari (o tempo scaduto, vedi sotto): si torna indietro senza fare nulla
    if (n_gen_max <= n_gen) return;
    if (diagnosi_nuovo_nodo(pos)) {
        n_gen_max = 0; // tutte le chiamate tornano subito e lo stato viene annullato normalmente
        return;
    }
    if (pos == orario_len) {
        registra_orario_completo(pos);
        return;
    }

    teaching_slot *slot = &orario[pos];
    // dopo lo slot di una classe: le lezioni che le restano bastano per gli slot che deve ancora riempire?
    if (pos > 0 && !lezioni_della_classe_bastano_dopo(&orario[pos - 1])) {
        diagnosi_ramo_scartato(pos - 1, SCARTATO_LEZIONI_DELLA_CLASSE);
        return;
    }
    // a ogni inizio ora: le ore che mancano ai docenti stanno ancora nel tempo rimasto?
    if (pos > 0 && pos % actual_classi_sezione == 0 && !ore_rimaste_stanno_nel_tempo_rimasto(slot->giorno, slot->ora)) {
        diagnosi_ramo_scartato(pos - 1, SCARTATO_ORE_DEI_DOCENTI);
        return;
    }

    // prima si provano i docenti (se nello slot puo' esserci lezione), poi lo slot vuoto (se ammesso)
    sort_queue_mul(slot->base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(slot)) prova_slot_con_docenti(slot, pos);
    prova_ora_buca(slot, pos);
}
