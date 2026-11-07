// Solver «per classe»: la ricerca di main (stessi controlli, stessa coda, stesse potature), ma dentro ogni giorno
// riempie una classe alla volta (tutte le ore di C01, poi C02...) invece di un'ora alla volta.
//
// L'orario resta indicizzato per cella come in main, pos = (giorno * ore + ora) * classi + classe; la ricerca scorre i
// passi s = 0, 1, 2... e cella_di[s] dice quale cella decide al passo s (indice_di[cella] il passo). Nella traccia
// "pos" e' il passo, e i dati hanno "ordine".
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../priority_queue/prio_queue.h"
#include "../../data_initialization/database_initializers.h"
#include "../../printer/printer.h"
#include "../../ingresso_uscita/dati_json.h"
#include "../../ingresso_uscita/traccia.h"
#include "../../potature/potature.h"
#include "../../controllo_in_avanti/controllo_in_avanti.h"
#include "../../controlli/comuni.h"  // lo stato e i controlli comuni (con le definizioni)

// ---- l'ordine delle celle: dentro il giorno una classe alla volta ----
#define N_CELLE (GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ)
static int cella_di[N_CELLE], indice_di[N_CELLE];  // la cella decisa al passo s, e il passo di ogni cella

static void prepara_ordine(void) {
    int s = 0;
    for (int g = 0; g < actual_giorni_settimana; g++)
        for (int c = 0; c < actual_classi_sezione; c++)
            for (int o = 0; o < actual_ore_giornaliere; o++) {
                const int cella = (g * actual_ore_giornaliere + o) * actual_classi_sezione + c;
                cella_di[s] = cella;
                indice_di[cella] = s++;
            }
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

// ---- i controlli sul singolo docente ----

// Mettendo il docente nella classe cla, oggi resta entro MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse?
// s e' il passo: le celle gia' decise di oggi sono quelle del giorno con un passo prima di s (in tutto il giorno,
// perche' le classi vanno una alla volta).
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int s, int giorno, int cla, const int prof) {
    const int inizio = giorno * actual_ore_giornaliere * actual_classi_sezione;
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO], n = 0;
    classi[n++] = cla;
    for (int i = inizio; i < inizio + actual_ore_giornaliere * actual_classi_sezione; i++) {
        if (indice_di[i] >= s || (orario[i].insegnante1 != prof && orario[i].insegnante2 != prof)) continue;
        const int c = g_classe(i);
        int gia = 0;
        for (int j = 0; j < n; j++) gia |= classi[j] == c;
        if (!gia) {
            if (n >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
            classi[n++] = c;
        }
    }
    return 1;
}

// ---- la ricerca ----
// print_orario(s) decide la cella del passo s, pos = cella_di[s], e scende a s + 1. Alla fine (s == orario_len)
// l'orario e' completo.

void print_orario(const int s) {
    if (n_gen_max <= n_gen) return;
    // traccia: solo conteggio dei nodi e, ogni 65.536 nodi o al limite di --ferma-nodi, il progresso
    // (non cambia la ricerca)
    if (++traccia_nodi >= traccia_prossimo) traccia_progresso(s);
    if (s > traccia_pos_max) traccia_pos_max = s;  // la posizione piu' avanti raggiunta (solo traccia)
    if (traccia_con_nodi) traccia_nodo(s);

    if (s == orario_len) {  // tutte le celle decise: un orario
        // ogni classe ha fatto tutte le sue ore (con ora_buca_possibile non dovrebbe mai scattare: per sicurezza)
        for (int c = 0; c < actual_classi_sezione; c++) if (ore_classe_svolte[c] != ore_classi[c]) return;
        n_gen++;
        if (f) print(orario, orario_len, n_gen, f);
        traccia_orario_trovato();
        return;
    }
    const int pos = cella_di[s];
    // le potature (potature/potature.h): la scelta nella cella prima lascia un resto ancora possibile?
    if (potatura_dopo(s)) {
        traccia_potatura(s, g_giorno(pos), g_ora(pos), g_classe(pos), &potatura);
        return;
    }

    const int giorno = g_giorno(pos), ora = g_ora(pos), cla = g_classe(pos);
    const int base2d_ci = cla * INSEGNANTE_SZ;                                                   // classe, docente
    const int base2d_gc = giorno * CLASSE_SEZIONE_SZ;                                            // giorno, classe
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;    // giorno, ora, docente
    const int base4d_queue = ((giorno * ORA_GIORNALIERA_SZ + ora) * CLASSE_SEZIONE_SZ + cla) * INSEGNANTE_SZ;  // la coda

    sort_queue_mul(base4d_queue, queue_sz, ora, actual_giorni_settimana - 1 - giorno);
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
                  && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(s, giorno, cla, p1)
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
                      && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(s, giorno, cla, p2)
                      && verifica_ora_ingresso(p2, ora)))
                    continue;
                prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                found_prof = 1;
                orario[pos] = (teaching_slot){p1, p2};
                print_orario(s + 1);
                post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
            }
            // da solo
            if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)) {
                prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                found_prof = 1;
                orario[pos] = (teaching_slot){p1, -1};
                print_orario(s + 1);
                post_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
            }
        }
        ore_classe_svolte[cla]--;
        if (!found_prof) {  // backtrack: nessuna scelta possibile in questa cella
            if (f) {  // in orario.txt tutta la griglia: e' la parte piu' lenta, per questo con --txt /dev/null si salta
                fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                        giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
                print(orario, orario_len, n_gen, f);
            }
            traccia_backtrack(s, giorno, ora, cla, base2d_ci, base3d_goi, base4d_queue,
                              ora_buca_possibile(giorno, ora, cla));
        }
    } else if (ora < PRIME_N_ORE_OBBLIGATORIE) {
        // la cella non puo' avere lezione e l'ora buca non e' ammessa: la ricerca torna indietro (solo traccia)
        traccia_slot_chiuso(s, giorno, ora, cla);
    }

    if (ora_buca_possibile(giorno, ora, cla)) {
        orario[pos] = (teaching_slot){-1, -1};
        n_ore_buche[base2d_gc + cla]++;
        print_orario(s + 1);
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
    prepara_ordine();
    traccia_imposta_ordine(cella_di, indice_di);  // nella traccia "pos" e' il passo s, e i dati hanno "ordine"
    prepara_potature(cella_di);

    traccia_dati();
    {  // la verifica dei dati (controllo_in_avanti/): se una condizione necessaria non vale, la ricerca non parte
        char errore[512];
        if (!verifica_fattibilita(errore, sizeof errore)) {
            traccia_errore(errore);
            traccia_chiudi();
            return 2;
        }
    }
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
