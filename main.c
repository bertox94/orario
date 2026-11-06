// Il solver dell'orario: una ricerca con backtracking, cella per cella (giorno, ora, classe).
//
// In ogni cella prova i docenti della classe in ordine di priorita' (la coda), prima in coppia con un codocente e poi
// da solo; chi passa i controlli si mette nella cella e la ricerca scende alla cella dopo. Se nessuno va, torna
// indietro (backtrack). Le potature (potature/) scartano prima i rami senza soluzioni. L'ingresso e l'uscita per il
// sito (dati da JSON, traccia della ricerca) sono in ingresso_uscita/.
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "priority_queue/prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"
#include "ingresso_uscita/dati_json.h"
#include "ingresso_uscita/traccia.h"
#include "potature/potature.h"

#define SCHEDULES_PER_GB 55000  // orari completi per GB di orario.txt; senza -n se ne cercano un quinto (circa 200 MB)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;                        // orari completi trovati
long long int n_gen_max = SCHEDULES_PER_GB / 5; // quanti cercarne al massimo (-n)
// l'orario: una cella per (giorno, ora, classe), con i due docenti (-1: nessuno)
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ... GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1] = (teaching_slot){-1, -1}
};
int orario_len;  // quante celle ha l'orario (giorni x ore x classi)
FILE *f;         // orario.txt (NULL con --txt /dev/null)

// ---- le celle: pos = (giorno * ore + ora) * classi + classe ----

int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) { return p % actual_classi_sezione; }

// la cella dell'ora prima della stessa classe
static const teaching_slot *cella_prima(const int pos) { return &orario[pos - actual_classi_sezione]; }

// le ore che restano al docente p nella classe (base2d_ci = classe * INSEGNANTE_SZ)
static int ore_rimaste(const int base2d_ci, const int p) {
    return ore_classi_docenti[base2d_ci + p] - ore_classi_docenti_svolte[base2d_ci + p];
}

// l'indice di (giorno, classe, docente) in ore_fatte_da_un_prof_in_una_classe_in_giorno
static int idx_oggi(const int giorno, const int cla, const int p) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + p;
}

// ---- i controlli sulla classe (la cella) ----

// La classe puo' fare lezione in questa cella? Si', se le ore che le restano bastano per le celle obbligatorie che
// restano (questa compresa) e se oggi non ha ancora avuto un'ora buca (dopo una buca la giornata della classe e'
// finita). Se no la cella puo' solo restare vuota (vedi ora_buca_possibile).
int activities_for_this_class_and_hour(int pos, int gio, const int ora, const int cla) {
    const int ore_rimanenti = ore_classi[cla] - ore_classe_svolte[cla];
    const int slot_obligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    const int slot_mancanti = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana - slot_obligatori_gia_occupati;
    if (ore_rimanenti < slot_mancanti) return 0;
    if (n_ore_buche[gio * CLASSE_SEZIONE_SZ + cla] > 0) return 0;
    return 1;
}

// L'ora buca e' possibile solo dopo le ore obbligatorie e se le ore che restano alla classe stanno ancora nei giorni
// dopo (un'ora buca chiude la giornata). Se no la classe deve fare lezione: senza docenti la ricerca torna indietro,
// cosi' una classe non chiude la settimana con meno ore di quelle che deve fare.
int ora_buca_possibile(const int giorno, const int ora, const int cla) {
    return ora >= PRIME_N_ORE_OBBLIGATORIE
           && ore_classi[cla] - ore_classe_svolte[cla] <= actual_ore_giornaliere * (actual_giorni_settimana - 1 - giorno);
}

// ---- la regola del sabato ----
// L'ultimo giorno, dalla seconda ora in poi: chi c'era nell'ora prima e ha ancora ore in questa classe deve esserci
// anche adesso (le ore di un docente in una classe in un giorno sono contigue, e non c'e' un giorno dopo in cui
// recuperarle). Per la coppia (prof1, prof2); da solo e' la stessa regola con prof2 = -1: se nell'ora prima c'erano
// due docenti con ore da fare, da solo non va, perche' servirebbero tutti e due.
int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno != actual_giorni_settimana - 1 || ora == 0) return 1;
    const int prev1 = cella_prima(pos)->insegnante1, prev2 = cella_prima(pos)->insegnante2;
    const int resta1 = prev1 != -1 && ore_rimaste(base2d_ci, prev1) > 0;
    const int resta2 = prev2 != -1 && ore_rimaste(base2d_ci, prev2) > 0;
    if (resta1 && resta2) return (prof1 == prev1 && prof2 == prev2) || (prof1 == prev2 && prof2 == prev1);
    if (resta1) return prof1 == prev1 || prof2 == prev1;
    if (resta2) return prof1 == prev2 || prof2 == prev2;
    return 1;
}

int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof) {
    return sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, prof, -1);
}

// ---- i controlli sul singolo docente ----

int prof_è_disponibile(const int base3d_goi, int p1) {
    return !prof_gia_impegnato_giorno_ora[base3d_goi + p1];
}

int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) {
    return ore_classi_docenti_svolte[base2d_ci + p1] < ore_classi_docenti[base2d_ci + p1];
}

// oggi in questa classe ha fatto meno di MAX_ORE_PROF_IN_UNA_CLASSE ore
int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof) {
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

// se oggi ha gia' fatto ore in questa classe, c'era anche nell'ora prima (il blocco continua); altrimenti il suo blocco
// e' finito e oggi non puo' tornare
int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof) {
    if (!ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]) return 1;
    const teaching_slot *prima = &orario[(giorno * actual_ore_giornaliere + ora - 1) * actual_classi_sezione + cla];
    return prima->insegnante1 == prof || prima->insegnante2 == prof;
}

// Mettendo il docente nella classe cla, oggi resta entro MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse?
// Scorre le celle gia' decise di oggi (dalla prima del giorno a pos) e conta le classi diverse in cui compare.
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, const int prof) {
    const int inizio = giorno * actual_ore_giornaliere * actual_classi_sezione;
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
        // in un'ora il docente e' in una classe sola: si salta alla prossima ora (-1 per l'i++ del ciclo)
        i = inizio + (g_ora(i) + 1) * actual_classi_sezione - 1;
    }
    return 1;
}

int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= ora_di_entrata_prof[prof];
}

// ---- i controlli sulla coppia e sul docente da solo ----

int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, int p1, int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2) <
           get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2);
}

int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < ore_classi_docenti_alone[base2d_ci + prof];
}

// ---- mettere e togliere un docente dalla cella (lo stato della ricerca) ----

// verso: +1 mette il docente nella cella (priorita' -1, occupato in quest'ora, un'ora in piu' nella classe e oggi),
// -1 lo toglie
static void segna(const int base2d_ci, const int base3d_goi, QueueItem *it, const int giorno, const int cla,
                  const int verso) {
    it->priority -= verso;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = verso > 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher] += verso;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, it->teacher)] += verso;
}

void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, +1);
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
}

void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, -1);
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
}

void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, +1);
    segna(base2d_ci, base3d_goi, jt, giorno, cla, +1);
    increment_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, -1);
    segna(base2d_ci, base3d_goi, jt, giorno, cla, -1);
    decrement_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

// ---- la coda dei docenti ----

// priorita' piu' alta prima; a parita' decide l'indice del docente (prima il piu' basso): senza, l'ordine dei pari lo
// sceglierebbe qsort, che su Linux (glibc) e su macOS li mette in modo diverso, e la ricerca cambierebbe da una
// macchina all'altra
int compare_queue_items(const void *a, const void *b) {
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;
    if (itemA->priority < itemB->priority) return 1;
    if (itemA->priority > itemB->priority) return -1;
    return (itemA->teacher > itemB->teacher) - (itemA->teacher < itemB->teacher);
}

void sort_queue_mul(size_t start, size_t n) {
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

// ---- la ricerca ----
// print_orario(pos) decide la cella pos e scende a pos + 1. Alla fine (pos == orario_len) l'orario e' completo.

void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;
    // traccia: solo conteggio dei nodi e, ogni tanto, il progresso (non cambia la ricerca)
    if (!(++traccia_nodi & 0xFFFF)) traccia_progresso(pos);
    if (pos > traccia_pos_max) traccia_pos_max = pos;  // la posizione piu' avanti raggiunta (solo traccia)
    if (traccia_con_nodi) traccia_nodo(pos);

    if (pos == orario_len) {  // tutte le celle decise: un orario
        n_gen++;
        if (f) print(orario, pos, n_gen, f);
        traccia_orario_trovato();
        return;
    }
    // le potature (potature/potature.h): la scelta nella cella prima lascia un resto ancora possibile?
    if (potatura_dopo(pos)) {
        traccia_potatura(pos, g_giorno(pos), g_ora(pos), g_classe(pos), &potatura);
        return;
    }

    const int giorno = g_giorno(pos), ora = g_ora(pos), cla = g_classe(pos);
    const int base2d_ci = cla * INSEGNANTE_SZ;                                                   // classe, docente
    const int base2d_gc = giorno * CLASSE_SEZIONE_SZ;                                            // giorno, classe
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;    // giorno, ora, docente
    const int base4d_queue = ((giorno * ORA_GIORNALIERA_SZ + ora) * CLASSE_SEZIONE_SZ + cla) * INSEGNANTE_SZ;  // la coda

    sort_queue_mul(base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(pos, giorno, ora, cla)) {
        int found_prof = 0;
        ore_classe_svolte[cla]++;
        const long long int queue_size = base4d_queue + queue_sz;
        for (int ii = base4d_queue; ii < queue_size; ii++) {
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
            for (int jj = ii + 1; jj < queue_size; jj++) {
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
                found_prof = 1;
                orario[pos] = (teaching_slot){p1, p2};
                print_orario(pos + 1);
                post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
            }
            // da solo
            if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)) {
                prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                found_prof = 1;
                orario[pos] = (teaching_slot){p1, -1};
                print_orario(pos + 1);
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
        // la cella non puo' avere lezione e l'ora buca non e' ammessa: la ricerca torna indietro (solo traccia)
        traccia_slot_chiuso(pos, giorno, ora, cla);
    }

    if (ora_buca_possibile(giorno, ora, cla)) {
        orario[pos] = (teaching_slot){-1, -1};
        n_ore_buche[base2d_gc + cla]++;
        print_orario(pos + 1);
        n_ore_buche[base2d_gc + cla]--;
    }
}

// Uso: ./untitled [--input FILE] [--traccia FILE] [-n N] [--txt FILE] [--max-backtrack N] [--max-nodi N] [--ferma-nodi N] [--progresso]
//   senza opzioni: come sempre, i dati di inizializza_dati() e gli orari in orario.txt
//   --input FILE:       i dati da un file JSON (vedi ingresso_uscita/dati_json.h) invece che da inizializza_dati()
//   --traccia FILE:     la traccia della ricerca per il sito, un JSON per riga (vedi ingresso_uscita/traccia.h)
//   -n N:               quanti orari cercare al massimo (n_gen_max; senza: SCHEDULES_PER_GB / 5)
//   --txt FILE:         dove scrivere orario.txt (con /dev/null non si scrive per niente: piu' veloce)
//   --max-backtrack N:  quanti backtrack registrare per esteso nella traccia (gli altri si contano), default 1000
//   --max-nodi N:       registra nella traccia anche i nodi della ricerca, al massimo N (default 0: nessuno)
//   --ferma-nodi N:     ferma la ricerca dopo N nodi (al primo multiplo di 65.536), con esito "limite di nodi":
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
    prepara_potature(NULL);

    traccia_dati();
    // con --txt /dev/null orario.txt non si scrive per niente: la ricerca e' la stessa, ma non perde tempo a formattare
    // la griglia a ogni backtrack (dal sito era quasi tutto il tempo)
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
