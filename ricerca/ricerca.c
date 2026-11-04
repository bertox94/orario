#include "ricerca.h"

#include <stdlib.h>
#include <string.h>

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
long long passi_max = 0;
int ferma_ai_passi = 0;
static long long passi_scritti = 0;
FILE *file_orario;
int primo_orario_1[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
int primo_orario_2[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];

/* ---- coda dei docenti di uno slot ---- */

// Chiave di ordinamento di un docente nella coda: priorita' (piu' alta prima) e, a parita', indice piu' basso prima.
// L'ordine e' totale e dipende solo dallo stato della ricerca, non dalla storia: cosi' --analizza-slot (che rifa' lo
// stato da zero) mostra la coda nell'ordine in cui la ricerca la prova davvero.
static inline long long chiave_coda(const QueueItem *x) {
    return ((long long) x->priority << 20) | (0xFFFFF - x->teacher);
}

// Ordina la copia della coda di uno slot (da start, n docenti) per chiave decrescente: si prova prima chi ha piu' ore
// ancora da fare, a parita' chi ha l'indice piu' basso. Ordinamento per inserzione: da una visita all'altra la coda
// cambia di poco ed e' quasi gia' in ordine, e cosi' costa molto meno di qsort.
static void sort_queue_mul(size_t start, size_t n) {
    QueueItem **q = &queue_mul[start];
    long long k[INSEGNANTE_SZ];
    for (size_t i = 0; i < n; i++) k[i] = chiave_coda(q[i]);
    for (size_t i = 1; i < n; i++) {
        QueueItem *x = q[i];
        const long long kx = k[i];
        size_t j = i;
        while (j > 0 && k[j - 1] < kx) {
            q[j] = q[j - 1];
            k[j] = k[j - 1];
            j--;
        }
        q[j] = x;
        k[j] = kx;
    }
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

// Verifica di potatura prima di assegnare: si aggiornano solo i contatori che il conto delle lezioni legge (4 per
// docente), si fa la verifica, e si annullano. Se la verifica fallisce non si entra mai nel nodo e non si registra
// nessun passo: il docente viene scartato qui, come gli altri vincoli. Solo i candidati che hanno gia' passato tutti
// gli altri controlli (libero, idoneo, sabato) arrivano fin qui.
static inline int potatura_prima_di_assegnare(teaching_slot *slot, const int pos, QueueItem *it, QueueItem *jt) {
    if (pos + 1 >= orario_len) return -1;  // ultimo slot: nessuna verifica, l'orario sara' completo
    // aggiorna i contatori che il conto delle lezioni legge: le ore fatte e l'assegnazione dello slot (serve per la
    // capienza di oggi, che guarda se il docente era nella classe nell'ora prima). Non si toccano la priorita',
    // l'occupazione del docente (prof_gia_impegnato_giorno_ora) e il contatore delle assegnazioni.
    classe_docente[slot->base2d_ci + it->teacher].svolte++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + it->teacher]++;
    slot->insegnante1 = it->teacher;
    if (jt) {
        classe_docente[slot->base2d_ci + jt->teacher].svolte++;
        ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + jt->teacher]++;
        codocenza[idx_codocenza(slot->classe, it->teacher, jt->teacher)].svolte++;
        slot->insegnante2 = jt->teacher;
    } else {
        classe_docente[slot->base2d_ci + it->teacher].alone_svolte++;
    }
    classe_info[slot->classe].svolte++;
    // verifica
    int motivo = -1;
    if (!lezioni_della_classe_bastano_dopo(slot))
        motivo = SCARTATO_LEZIONI_DELLA_CLASSE;
    else if ((pos + 1) % actual_classi_sezione == 0
             && !ore_rimaste_stanno_nel_tempo_rimasto(orario[pos + 1].giorno, orario[pos + 1].ora))
        motivo = SCARTATO_ORE_DEI_DOCENTI;
    // annulla
    classe_docente[slot->base2d_ci + it->teacher].svolte--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + it->teacher]--;
    slot->insegnante1 = -1;
    if (jt) {
        classe_docente[slot->base2d_ci + jt->teacher].svolte--;
        ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + jt->teacher]--;
        codocenza[idx_codocenza(slot->classe, it->teacher, jt->teacher)].svolte--;
        slot->insegnante2 = -1;
    } else {
        classe_docente[slot->base2d_ci + it->teacher].alone_svolte--;
    }
    classe_info[slot->classe].svolte--;
    return motivo;
}

// Assegna lo slot (da solo o in coppia), prosegue la ricerca e annulla. Prima verifica la potatura: se il ramo
// verrebbe scartato al nodo successivo, non si entra e si registra lo scarto qui (nessun passo in piu').
static inline void assegna_e_prosegui(teaching_slot *slot, const int pos, QueueItem *it, QueueItem *jt) {
    const int scarto = potatura_prima_di_assegnare(slot, pos, it, jt);
    if (scarto >= 0) {
        diagnosi_ramo_scartato(pos, scarto);
        return;
    }
    prep_chiamata_ricorsiva(slot, it, jt);
    cerca_orario(pos + 1);
    post_chiamata_ricorsiva(slot, it, jt);
}

// Vedi ricerca.h: le stesse verifiche di potatura che assegna_e_prosegui fa prima di entrare nel nodo successivo.
int potatura_se_messo(const int pos, const int ii, const int jj) {
    QueueItem *jt = jj >= 0 ? queue_mul[jj] : NULL;
    return potatura_prima_di_assegnare(&orario[pos], pos, queue_mul[ii], jt);
}

/* ---- un nodo della ricerca ---- */

// Un orario completo: lo si conta e lo si scrive.
static void registra_orario_completo(const int pos) {
    n_gen++;
    if (n_gen == 1)
        for (int i = 0; i < pos; i++) {
            primo_orario_1[i] = orario[i].insegnante1;
            primo_orario_2[i] = orario[i].insegnante2;
        }
    print(orario, pos, n_gen, file_orario);
}

// Nessun docente utilizzabile per questo slot: si torna indietro. Lo si conta e, con -b, lo si scrive (riga B, piu' la
// riga M dei motivi con --motivi). Con --passi si scrive solo se anche il nodo e' stato registrato (passo_scritto): le
// righe B senza la loro P non servirebbero e il file deve restare piccolo.
static void registra_marcia_indietro(const teaching_slot *slot, const int pos, const int queue_size,
                                     const int passo_scritto) {
    n_backtrack++;
    diagnosi_marcia_indietro(slot, pos, slot->base4d_queue, queue_size);
    if (stampa_backtrack == BACKTRACK_COMPATTO && (passi_max == 0 || passo_scritto)) {
        stampa_marcia_compatta(file_orario, orario, pos);
        if (diagnosi_con_motivi()) diagnosi_scrivi_motivi_ultima_marcia(file_orario);
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
// La potatura si fa qui: la buca non passa per assegna_e_prosegui.
static void prova_ora_buca(const teaching_slot *slot, const int pos) {
    const int cla = slot->classe;
    if (slot->ora >= PRIME_N_ORE_OBBLIGATORIE
        && classe_info[cla].ore - classe_info[cla].svolte
           <= (actual_giorni_settimana - slot->giorno - 1) * actual_ore_giornaliere) {
        n_ore_buche[slot->base2d_gc + cla]++;
        // potatura: con la buca le lezioni della classe bastano ancora? (tiene conto degli ingressi e del massimo giornaliero)
        if (pos + 1 < orario_len && !lezioni_della_classe_bastano_dopo(slot)) {
            diagnosi_ramo_scartato(pos, SCARTATO_LEZIONI_DELLA_CLASSE);
        } else {
            cerca_orario(pos + 1);
        }
        n_ore_buche[slot->base2d_gc + cla]--;
    }
}

// Prova tutti i docenti per questo slot; se nessuno va bene e' una marcia indietro.
static void prova_slot_con_docenti(teaching_slot *slot, const int pos, const int passo_scritto) {
    // se alla fine il contatore non e' cresciuto, questo nodo non ha assegnato nessun docente (marcia indietro)
    const long long int assegnazioni_prima = n_assegnazioni;
    const int queue_size = slot->base4d_queue + queue_sz;
    prova_tutti_i_docenti(slot, pos, queue_size);
    if (n_assegnazioni == assegnazioni_prima) registra_marcia_indietro(slot, pos, queue_size, passo_scritto);
}

void cerca_orario(const int pos) {
    // gia' trovati abbastanza orari (o tempo scaduto, o fermato: vedi diagnosi_controllo_periodico): si torna indietro
    if (n_gen_max <= n_gen) return;
    if (diagnosi_nuovo_nodo(pos)) {
        n_gen_max = 0; // tutte le chiamate tornano subito e lo stato viene annullato normalmente
        return;
    }
    // con --passi: l'orario parziale di questo nodo (riga P), finche' non si sono registrati passi_max nodi
    int passo_scritto = 0;
    if (pos > 0 && passi_scritti < passi_max && stampa_backtrack == BACKTRACK_COMPATTO) {
        passi_scritti++;
        passo_scritto = 1;
        stampa_passo_compatto(file_orario, orario, pos);
        if (passi_scritti == passi_max) {
            fflush(file_orario);  // l'ultimo passo registrato arriva subito a chi legge
            if (ferma_ai_passi) {  // fermata esatta al passo N: deterministica, al contrario di tempo o segnali
                interrotto_su_richiesta = 1;
                n_gen_max = 0;  // tutte le chiamate tornano subito e lo stato viene annullato normalmente
                return;
            }
        }
    }
    if (pos == orario_len) {
        registra_orario_completo(pos);
        return;
    }

    teaching_slot *slot = &orario[pos];
    // la potatura (lezioni della classe, ore dei docenti) si fa in assegna_e_prosegui, prima di entrare nel nodo:
    // il docente che non passa viene scartato li', senza registrare un passo in piu'

    // prima si provano i docenti (se nello slot puo' esserci lezione), poi lo slot vuoto (se ammesso)
    sort_queue_mul(slot->base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(slot)) prova_slot_con_docenti(slot, pos, passo_scritto);
    prova_ora_buca(slot, pos);
}

/* ---- analisi di uno slot ---- */

// Rifa' lo stato della ricerca fino allo slot pos, con l'orario parziale letto da `file` (prima riga: pos; poi i primi
// pos slot come nel formato compatto: "i", "i+j" o "-"), e scrive la coda dei docenti dello slot pos con per ognuno il
// motivo per cui non va bene (vedi diagnosi_analizza_slot). Ritorna 0 se il file non va bene.
// La priorita', le ore e l'ordine della coda sono esattamente quelli della ricerca (vedi chiave_coda).
int analizza_slot(const char *file, FILE *out) {
    FILE *f = fopen(file, "r");
    if (!f) return 0;
    int pos;
    if (fscanf(f, "%d", &pos) != 1 || pos < 0 || pos >= orario_len) {
        fclose(f);
        return 0;
    }
    for (int i = 0; i < pos; i++) {
        char token[64];
        if (fscanf(f, "%63s", token) != 1) {
            fclose(f);
            return 0;
        }
        teaching_slot *slot = &orario[i];
        if (!strcmp(token, "-")) {  // ora buca
            n_ore_buche[slot->base2d_gc + slot->classe]++;
            continue;
        }
        const int a = atoi(token);
        const char *piu = strchr(token, '+');
        const int b = piu ? atoi(piu + 1) : -1;
        QueueItem *it = NULL, *jt = NULL;
        for (int k = 0; k < queue_sz; k++) {
            if (queue_mul[k]->teacher == a) it = queue_mul[k];
            if (b >= 0 && queue_mul[k]->teacher == b) jt = queue_mul[k];
        }
        if (!it || (b >= 0 && !jt)) {
            fclose(f);
            return 0;
        }
        prep_chiamata_ricorsiva(slot, it, jt);
    }
    fclose(f);
    teaching_slot *slot = &orario[pos];
    // le verifiche di potatura che cerca_orario fa all'ingresso del nodo, ognuna con l'esito (anche quando e' rispettata):
    // se una fallisce la ricerca scarta il nodo senza provare i docenti, e la causa e' la scelta fatta nello slot pos-1
    char scarto[8192] = "";
    if (pos > 0) {
        FILE *m = fmemopen(scarto, sizeof scarto - 1, "w");
        if (m) {
            const teaching_slot *causa = &orario[pos - 1];
            fprintf(m, "{\"classe\": %d, \"ore_classe\": %d, \"ore_classe_fatte\": %d, \"scelta\": [%d, %d], \"verifiche\": [",
                    causa->classe, classe_info[causa->classe].ore, classe_info[causa->classe].svolte,
                    causa->insegnante1, causa->insegnante2);
            const int ok0 = lezioni_della_classe_bastano_dopo(causa);
            fprintf(m, "{\"motivo\": %d, \"controllata\": true, \"ok\": %s", SCARTATO_LEZIONI_DELLA_CLASSE, ok0 ? "true" : "false");
            if (!ok0) diagnosi_scrivi_dettaglio_scarto(m, SCARTATO_LEZIONI_DELLA_CLASSE, &dettaglio_scarto);
            fputs("}, ", m);
            const int inizio_ora = pos % actual_classi_sezione == 0;
            const int ok1 = !inizio_ora || ore_rimaste_stanno_nel_tempo_rimasto(slot->giorno, slot->ora);
            fprintf(m, "{\"motivo\": %d, \"controllata\": %s, \"ok\": %s", SCARTATO_ORE_DEI_DOCENTI, inizio_ora ? "true" : "false",
                    ok1 ? "true" : "false");
            if (!ok1) diagnosi_scrivi_dettaglio_scarto(m, SCARTATO_ORE_DEI_DOCENTI, &dettaglio_scarto);
            fputs("}]}", m);
            fclose(m);
        }
    }
    sort_queue_mul(slot->base4d_queue, queue_sz);
    diagnosi_analizza_slot(slot, pos, slot->base4d_queue, slot->base4d_queue + queue_sz, out, scarto[0] ? scarto : NULL);
    return 1;
}
