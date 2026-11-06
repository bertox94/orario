// Solver «in-avanti»: la ricerca di main (stessi controlli, stesso ordine delle celle e della coda) più un controllo in
// avanti dopo ogni scelta (vedi «controllo in avanti» qui sotto e solutori/LEGGIMI.md).
//
// L'orario e' un vettore di celle: pos = (giorno * ore + ora) * classi + classe, cioe' un giorno alla volta, un'ora
// alla volta, tutte le classi di quell'ora. print_orario(pos) decide la cella pos e scende alla successiva.

#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "../../priority_queue/prio_queue.h"
#include "../../data_initialization/database_initializers.h"
#include "../../printer/printer.h"
#include "../../ingresso_uscita/dati_json.h"
#include "../../ingresso_uscita/traccia.h"
#include "../../controllo_in_avanti/controllo_in_avanti.h"
#include "../../potature/potature.h"

int traccia_c_avanti(int pos, int giorno, int ora, int cla, int base2d_ci, int base3d_goi, QueueItem *it, QueueItem *jt,
                     int *fermato_a, int k);  // traccia_in_avanti.c

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

long long int n_gen = 0;                              // orari trovati
long long int n_gen_max = SCHEDULES_PER_GB / 5;      // quanti cercarne al massimo (-n)
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ... GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1] = (teaching_slot){-1, -1}
};
int orario_len;                                       // quante celle ha l'orario
FILE *f;                                              // orario.txt (gli orari trovati e i backtrack); NULL: niente

// ---- posizioni e indici ----

int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) { return p % actual_classi_sezione; }

// la cella (giorno, ora, classe)
static int cella(const int giorno, const int ora, const int cla) {
    return (giorno * actual_ore_giornaliere + ora) * actual_classi_sezione + cla;
}

// l'indice di ore_fatte_da_un_prof_in_una_classe_in_giorno: le ore che il docente p ha fatto oggi nella classe
static int idx_oggi(const int giorno, const int cla, const int p) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + p;
}

// le ore che restano al docente p nella classe (base2d_ci = classe * INSEGNANTE_SZ)
static int ore_rimaste(const int base2d_ci, const int p) {
    return ore_classi_docenti[base2d_ci + p] - ore_classi_docenti_svolte[base2d_ci + p];
}

// ---- i controlli della cella (come in main) ----

// La classe puo' fare lezione in quest'ora: le ore che le restano bastano per gli slot obbligatori che mancano (questo
// compreso) e oggi non ha ancora avuto un'ora buca (dopo un'ora buca, oggi niente lezioni).
int activities_for_this_class_and_hour(int pos, const int gio, const int ora, const int cla) {
    (void) pos;
    const int ore_rimanenti = ore_classi[cla] - ore_classe_svolte[cla];
    const int slot_obbligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    const int slot_mancanti = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana - slot_obbligatori_gia_occupati;
    if (ore_rimanenti < slot_mancanti) return 0;
    return n_ore_buche[gio * CLASSE_SEZIONE_SZ + cla] == 0;
}

// Regole del sabato (l'ultimo giorno): chi era nella classe nell'ora prima e ha ancora ore deve restare, perche' non ci
// sono altri giorni per finirle e le sue ore devono essere contigue.
// - in coppia: se a tutti e due quelli dell'ora prima restano ore, la coppia deve essere proprio loro; se resta a uno
//   solo, deve esserci lui;
// - da solo: se restano ore a tutti e due quelli dell'ora prima, nessuno da solo va bene; se resta a uno, deve essere lui.
static int ha_ancora_ore(const int base2d_ci, const int p) { return p >= 0 && ore_rimaste(base2d_ci, p) > 0; }

int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno != actual_giorni_settimana - 1 || ora == 0) return 1;
    const int a = orario[pos - actual_classi_sezione].insegnante1, b = orario[pos - actual_classi_sezione].insegnante2;
    const int resta_a = ha_ancora_ore(base2d_ci, a), resta_b = ha_ancora_ore(base2d_ci, b);
    if (resta_a && resta_b) return (prof1 == a && prof2 == b) || (prof1 == b && prof2 == a);
    if (resta_a) return prof1 == a || prof2 == a;
    if (resta_b) return prof1 == b || prof2 == b;
    return 1;
}

int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof) {
    if (giorno != actual_giorni_settimana - 1 || ora == 0) return 1;
    const int a = orario[pos - actual_classi_sezione].insegnante1, b = orario[pos - actual_classi_sezione].insegnante2;
    const int resta_a = ha_ancora_ore(base2d_ci, a), resta_b = ha_ancora_ore(base2d_ci, b);
    if (resta_a && resta_b) return 0;  // devono esserci tutti e due: da solo nessuno va bene
    if (resta_a) return prof == a;
    if (resta_b) return prof == b;
    return 1;
}

// il docente e' libero in quest'ora (base3d_goi: giorno e ora)
int prof_è_disponibile(const int base3d_goi, const int p1) { return !prof_gia_impegnato_giorno_ora[base3d_goi + p1]; }

// il docente ha ancora ore da fare nella classe
int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) { return ore_rimaste(base2d_ci, p1) > 0; }

// oggi il docente ha fatto nella classe meno del massimo di ore
int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof) {
    (void) ora;
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

// le ore di oggi del docente nella classe restano contigue: se oggi ci ha gia' insegnato, c'era nell'ora prima
int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof) {
    if (!ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]) return 1;
    const teaching_slot *prima = &orario[cella(giorno, ora - 1, cla)];
    return prima->insegnante1 == prof || prima->insegnante2 == prof;
}

// docente e codocente hanno ancora ore da fare insieme nella classe
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, int p1, int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2) <
           get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2);
}

// il docente ha ancora ore da solo (senza codocente) nella classe
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < ore_classi_docenti_alone[base2d_ci + prof];
}

// Oggi il docente resta entro MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse, contando questa: si guardano
// le celle di oggi gia' decise (prima di pos). Un docente sta in una sola classe per ora: trovato in un'ora, si passa
// all'ora dopo.
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, const int prof) {
    const int inizio = cella(giorno, 0, 0);
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO], n = 0;
    classi[n++] = cla;
    for (int i = inizio; i < pos; i++) {
        if (orario[i].insegnante1 != prof && orario[i].insegnante2 != prof) continue;
        const int c = g_classe(i);
        int gia = 0;
        for (int j = 0; j < n; j++) gia |= classi[j] == c;
        if (!gia) {
            if (n >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
            classi[n++] = c;
        }
        i = inizio + (g_ora(i) + 1) * actual_classi_sezione - 1;  // all'ora dopo (il ciclo aggiunge 1)
    }
    return 1;
}

// il docente e' gia' entrato a quest'ora
int verifica_ora_ingresso(const int prof, const int ora) { return ora >= ora_di_entrata_prof[prof]; }

// L'ora buca e' possibile solo dopo le ore obbligatorie e se le ore che restano alla classe stanno ancora nei giorni
// dopo (un'ora buca chiude la giornata). Se no la classe deve fare lezione: senza docenti la ricerca torna indietro.
int ora_buca_possibile(const int giorno, const int ora, const int cla) {
    return ora >= PRIME_N_ORE_OBBLIGATORIE
           && ore_classi[cla] - ore_classe_svolte[cla] <= actual_ore_giornaliere * (actual_giorni_settimana - 1 - giorno);
}

// ---- la coda dei docenti ----
// In ogni cella i docenti si provano in ordine di priorita' (le ore che restano loro), la piu' alta prima; a parita'
// decide l'indice del docente, cosi' l'ordine non dipende da qsort (e da macOS o Linux).
int compare_queue_items(const void *a, const void *b) {
    const QueueItem *x = *(const QueueItem **) a, *y = *(const QueueItem **) b;
    if (x->priority < y->priority) return 1;
    if (x->priority > y->priority) return -1;
    return (x->teacher > y->teacher) - (x->teacher < y->teacher);
}

void sort_queue_mul(size_t start, size_t n) { qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items); }

// ---- mettere e togliere una scelta (prima e dopo la discesa) ----
// mettere: il docente (o la coppia) e' occupato in quest'ora, ha un'ora in piu' nella classe (da solo o insieme) e oggi;
// la sua priorita' scende. Togliere fa il contrario.

void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]++;
}

void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]--;
}

void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    QueueItem *q[2] = {it, jt};
    for (int z = 0; z < 2; z++) {
        q[z]->priority--;
        prof_gia_impegnato_giorno_ora[base3d_goi + q[z]->teacher] = 1;
        ore_classi_docenti_svolte[base2d_ci + q[z]->teacher]++;
        ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, q[z]->teacher)]++;
    }
    increment_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    QueueItem *q[2] = {it, jt};
    for (int z = 0; z < 2; z++) {
        q[z]->priority++;
        prof_gia_impegnato_giorno_ora[base3d_goi + q[z]->teacher] = 0;
        ore_classi_docenti_svolte[base2d_ci + q[z]->teacher]--;
        ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, q[z]->teacher)]--;
    }
    decrement_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

// ---- il controllo in avanti ----
// Dopo ogni scelta nella cella pos (una coppia, un docente, o l'ora buca), prima di scendere: il resto e' ancora
// risolvibile? E' il modulo condiviso controllo_in_avanti/ (gli altri solver lo fanno all'ingresso della cella dopo,
// con le potature): qui la scelta si scarta senza scendere. -1: si puo' andare avanti.
static int cella_bloccata_dopo_la_scelta(const int pos) { return controllo_in_avanti(pos + 1); }

// ---- la ricerca ----
// print_orario(pos) decide la cella pos. Se la classe puo' fare lezione, prova i docenti della coda in ordine: per
// ognuno che passa i controlli base prova le coppie con i docenti dopo di lui nella coda, poi lui da solo; ogni scelta
// che passa anche il controllo in avanti scende alla cella dopo. Se nessuna scelta e' possibile e' un backtrack (scritto
// in orario.txt e nella traccia). Poi, se si puo', prova l'ora buca.
void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;
    // traccia: solo conteggio dei nodi e, ogni 65.536 nodi o al limite di --ferma-nodi, il progresso
    // (non cambia la ricerca)
    if (++traccia_nodi >= traccia_prossimo) traccia_progresso(pos);
    if (pos > traccia_pos_max) traccia_pos_max = pos;  // la posizione piu' avanti raggiunta (solo traccia)
    if (traccia_con_nodi) traccia_nodo(pos);

    if (pos == orario_len) {  // tutte le celle decise: un orario
        // ogni classe ha fatto tutte le sue ore (con ora_buca_possibile non dovrebbe mai scattare: per sicurezza)
        for (int c = 0; c < actual_classi_sezione; c++) if (ore_classe_svolte[c] != ore_classi[c]) return;
        n_gen++;
        if (f) print(orario, pos, n_gen, f);
        traccia_orario_trovato();
        return;
    }
    // le potature L, C e T (potature/potature.h), come negli altri solver; A, il controllo in avanti, qui si fa gia'
    // prima di scendere
    if (potatura_dopo(pos)) {
        traccia_potatura(pos, g_giorno(pos), g_ora(pos), g_classe(pos), &potatura);
        return;
    }

    const int giorno = g_giorno(pos), ora = g_ora(pos), cla = g_classe(pos);
    const int base2d_ci = cla * INSEGNANTE_SZ;                                         // classe, docente
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;  // giorno, ora, docente
    const int base4d_queue = ((giorno * ORA_GIORNALIERA_SZ + ora) * CLASSE_SEZIONE_SZ + cla) * INSEGNANTE_SZ;  // la coda

    sort_queue_mul(base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(pos, giorno, ora, cla)) {
        int found_prof = 0;
        ore_classe_svolte[cla]++;
        const int fine_coda = base4d_queue + queue_sz;
        for (int ii = base4d_queue; ii < fine_coda; ii++) {
            QueueItem *it = queue_mul[ii];
            const int p1 = it->teacher;
            // i controlli base del docente
            if (!(prof_è_disponibile(base3d_goi, p1)
                  && deve_ancora_svolgere_ore_in_questa_classe(base2d_ci, p1)
                  && prof_fa_al_max_n_ore(giorno, ora, cla, p1)
                  && prof_fa_ore_contigue(giorno, ora, cla, p1)
                  && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p1)
                  && verifica_ora_ingresso(p1, ora)))
                continue;
            // in coppia, con i docenti dopo di lui nella coda
            for (int jj = ii + 1; jj < fine_coda; jj++) {
                QueueItem *jt = queue_mul[jj];
                const int p2 = jt->teacher;
                if (!(prof_è_disponibile(base3d_goi, p2)
                      && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(cla, p1, p2)
                      && prof_fa_al_max_n_ore(giorno, ora, cla, p2)
                      && prof_fa_ore_contigue(giorno, ora, cla, p2)
                      && sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, p1, p2)
                      && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p2)
                      && verifica_ora_ingresso(p2, ora)))
                    continue;
                prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                orario[pos] = (teaching_slot){p1, p2};
                if (cella_bloccata_dopo_la_scelta(pos) < 0) {  // controllo in avanti
                    found_prof = 1;
                    print_orario(pos + 1);
                }
                post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
            }
            // da solo
            if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)) {
                prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                orario[pos] = (teaching_slot){p1, -1};
                if (cella_bloccata_dopo_la_scelta(pos) < 0) {  // controllo in avanti
                    found_prof = 1;
                    print_orario(pos + 1);
                }
                post_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
            }
        }
        ore_classe_svolte[cla]--;
        if (!found_prof) {  // backtrack: nessuna scelta possibile in questa cella
            if (f) {  // in orario.txt tutta la griglia: e' la parte piu' lenta, per questo con --txt /dev/null si salta
                fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                        giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
                print(orario, pos, n_gen, f);
            }
            traccia_backtrack(pos, giorno, ora, cla, base2d_ci, base3d_goi, base4d_queue,
                              ora_buca_possibile(giorno, ora, cla));
        }
    } else if (ora < PRIME_N_ORE_OBBLIGATORIE) {
        // lo slot non puo' avere lezione e l'ora buca non e' ammessa: la ricerca torna indietro (solo traccia)
        traccia_slot_chiuso(pos, giorno, ora, cla);
    }

    if (ora_buca_possibile(giorno, ora, cla)) {
        orario[pos] = (teaching_slot){-1, -1};
        n_ore_buche[giorno * CLASSE_SEZIONE_SZ + cla]++;
        if (cella_bloccata_dopo_la_scelta(pos) < 0) print_orario(pos + 1);  // controllo in avanti anche dopo l'ora buca
        n_ore_buche[giorno * CLASSE_SEZIONE_SZ + cla]--;
    }
}

// Uso: solutore [--input FILE] [--traccia FILE] [-n N] [--txt FILE] [--max-backtrack N] [--max-nodi N] [--ferma-nodi N] [--progresso]
//   senza opzioni: i dati di inizializza_dati() e gli orari in orario.txt
//   --input FILE:       i dati da un file JSON (vedi ingresso_uscita/dati_json.h) invece che da inizializza_dati()
//   --traccia FILE:     la traccia della ricerca per il sito, un JSON per riga (vedi ingresso_uscita/traccia.h)
//   -n N:               quanti orari cercare al massimo (n_gen_max; senza: SCHEDULES_PER_GB / 5)
//   --txt FILE:         dove scrivere orario.txt (con /dev/null non si scrive per niente: piu' veloce)
//   --max-backtrack N:  quanti backtrack registrare per esteso nella traccia (gli altri si contano), default 1000
//   --max-nodi N:       registra nella traccia anche i nodi della ricerca, al massimo N (default 0: nessuno)
//   --ferma-nodi N:     ferma la ricerca dopo N nodi esatti, con esito "limite di nodi":
//                       sempre allo stesso punto, per i test notturni
//   --progresso:        ogni mezzo secondo una riga JSON di progresso su stderr
int main(int argc, char **argv) {
    const char *file_input = NULL, *file_traccia = NULL, *file_txt = "orario.txt";
    long long int max_backtrack = 1000, max_nodi = 0;
    int progresso = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--input") && i + 1 < argc) file_input = argv[++i];
        else if (!strcmp(argv[i], "--traccia") && i + 1 < argc) file_traccia = argv[++i];
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) n_gen_max = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--txt") && i + 1 < argc) file_txt = argv[++i];
        else if (!strcmp(argv[i], "--max-backtrack") && i + 1 < argc) max_backtrack = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--max-nodi") && i + 1 < argc) max_nodi = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--ferma-nodi") && i + 1 < argc) traccia_ferma_nodi = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--progresso")) progresso = 1;
        else {
            fprintf(stderr, "Opzione sconosciuta: %s\n", argv[i]);
            return 2;
        }
    }
    if (!traccia_apri(file_traccia, max_backtrack, max_nodi, progresso)) {
        fprintf(stderr, "Non riesco a scrivere la traccia in %s\n", file_traccia);
        return 2;
    }

    time_t adesso;
    time(&adesso);
    const clock_t inizio = clock();
    printf("Program started at: %s\n", ctime(&adesso));

    if (file_input) {
        char errore[512];
        if (!inizializza_dati_da_json(file_input, errore, sizeof errore)) {
            traccia_errore(errore);
            traccia_chiudi();
            return 2;
        }
    } else {
        inizializza_dati();
    }
    stampa_dati();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    prepara_potature(NULL);    // prepara anche il controllo in avanti, con l'ordine di main
    potature_in_avanti(0);     // A si fa prima di scendere (cella_bloccata_dopo_la_scelta), non all'ingresso
    traccia_controllo_extra = traccia_c_avanti;  // nella traccia, il controllo in avanti per ogni coppia e da solo

    traccia_dati();
    {
        char errore[512];
        if (!verifica_fattibilita(errore, sizeof errore)) {  // l'orario non esiste: la ricerca non parte
            traccia_errore(errore);
            traccia_chiudi();
            return 2;
        }
    }
    // con --txt /dev/null orario.txt non si scrive per niente: la ricerca e' la stessa, ma non perde tempo a formattare
    // la griglia a ogni backtrack (dal sito e' quasi tutto il tempo)
    f = strcmp(file_txt, "/dev/null") ? fopen(file_txt, "w") : NULL;
    print_orario(0);

    const double secondi = (clock() - inizio) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) secondi / 60, (int) secondi % 60,
           (double) n_gen / secondi);
    fflush(stdout);
    traccia_fine(secondi);
    traccia_chiudi();
    if (f) fclose(f);
    return 0;
}
