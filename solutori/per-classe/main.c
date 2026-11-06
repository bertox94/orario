#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "../../priority_queue/prio_queue.h"
#include "../../data_initialization/database_initializers.h"
#include "../../printer/printer.h"
#include "../../ingresso_uscita/dati_json.h"
#include "../../ingresso_uscita/traccia.h"
#include "../../potature/potature.h"

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;
long long int n_gen_max = SCHEDULES_PER_GB / 5;
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ...
        GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1
    ] = (teaching_slot){-1, -1}
};

int orario_len;
FILE *f;

// Solver «per classe»: dentro ogni giorno la ricerca riempie una classe alla volta (tutte le ore di C01, poi C02...)
// invece di un'ora alla volta. L'orario resta indicizzato per cella come in main, pos = (giorno*ore + ora)*classi +
// classe; la ricerca scorre gli indici s = 0, 1, 2... e cella_di[s] dice quale cella decide al passo s.
int cella_di[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
int indice_di[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];

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


int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

int activities_for_this_class_and_hour(int pos, int gio, const int ora, const int cla) {
    int ore_rimanenti = ore_classi[cla] - ore_classe_svolte[cla];
    int slot_obligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    int slot_minimi_da_riempire = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana;
    int slot_mancanti = slot_minimi_da_riempire - slot_obligatori_gia_occupati;

    if (ore_rimanenti < slot_mancanti) {
        return 0;
    }
    //se c'è almeno un ora buca, non mettere altre lezioni
    if (n_ore_buche[gio * CLASSE_SEZIONE_SZ + cla] > 0)
        return 0;

    return 1;
}

int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        int prev_prof1 = orario[pos - actual_classi_sezione].insegnante1;
        int prev_prof2 = orario[pos - actual_classi_sezione].insegnante2;

        if (prev_prof1 != -1 && prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] > 0
                && ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] > 0) {
                return prof1 == prev_prof1 && prof2 == prev_prof2
                       || prof1 == prev_prof2 && prof2 == prev_prof1;
            } else if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                       0) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            } else if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                       0) {
                return prof1 == prev_prof2 || prof2 == prev_prof2;
            }
        } else if (prev_prof1 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                0) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            }
        } else if (prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                0) {
                return prof1 == prev_prof2 || prof2 == prev_prof2;
            }
        }
    }
    return 1;
}

int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof) {
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        int prev_prof1 = orario[pos - actual_classi_sezione].insegnante1;
        int prev_prof2 = orario[pos - actual_classi_sezione].insegnante2;

        if (prev_prof1 != -1 && prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] > 0
                && ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] > 0) {
                return 0; // both docenti need to be there in this hour, so one will never complete the hours
            } else if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                       0) {
                return prof == prev_prof1;
            } else if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                       0) {
                return prof == prev_prof2;
            }
        } else if (prev_prof1 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                0) {
                return prof == prev_prof1;
            }
        } else if (prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                0) {
                return prof == prev_prof2;
            }
        }
    }
    return 1;
}

// Comparison function for qsort
int compare_queue_items(const void *a, const void *b) {
    // Cast the void pointers to pointers to QueueItem*
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;

    // Sort by priority (descending: highest priority first)
    if (itemA->priority < itemB->priority) return 1;
    if (itemA->priority > itemB->priority) return -1;
    // a parita' di priorita' decide l'indice del docente (come in main, PR #70): l'ordine non dipende da qsort
    return (itemA->teacher > itemB->teacher) - (itemA->teacher < itemB->teacher);
}

// Assuming you know the number of elements in the array, say 'n'
void sort_queue_mul(size_t start, size_t n) {
    // qsort has an average time complexity of O(N log N)
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof) {
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[
               giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof] <
           MAX_ORE_PROF_IN_UNA_CLASSE;
}

int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof) {
    if (ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof]) {
        return orario[giorno * actual_ore_giornaliere * actual_classi_sezione + (ora - 1) * actual_classi_sezione + cla]
               .insegnante1 == prof
               || orario[giorno * actual_ore_giornaliere * actual_classi_sezione + (ora - 1) * actual_classi_sezione +
                         cla].insegnante2 == prof;
    }
    return 1;
}

int prof_è_disponibile(const int base3d_goi, int p1) {
    return !prof_gia_impegnato_giorno_ora[base3d_goi + p1];
}

int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) {
    return ore_classi_docenti_svolte[base2d_ci + p1] < ore_classi_docenti[base2d_ci + p1];
}

int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, int p1, int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2) <
           get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2);
}

void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof]++;
}

void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + it->teacher]++;
    jt->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + jt->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + jt->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + jt->teacher]++;
    increment_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof]--;
}

void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + it->teacher]--;
    jt->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + jt->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + jt->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + jt->teacher]--;
    decrement_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < ore_classi_docenti_alone[
               base2d_ci + prof];
}

// s: l'indice (nell'ordine della ricerca) della cella che si sta decidendo; contano le celle del giorno gia' decise.
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int s, int giorno, int cla, const int prof) {
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO];
    int classi_sz = 0;
    classi[classi_sz++] = cla;
    for (int o = 0; o < actual_ore_giornaliere; o++)
        for (int c = 0; c < actual_classi_sezione; c++) {
            const int i = (giorno * actual_ore_giornaliere + o) * actual_classi_sezione + c;
            if (indice_di[i] >= s || (orario[i].insegnante1 != prof && orario[i].insegnante2 != prof)) continue;
            int found = 0;
            for (int j = 0; j < classi_sz; j++) found |= classi[j] == c;
            if (!found) {
                if (classi_sz >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
                classi[classi_sz++] = c;
            }
        }
    return 1;
}

int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= ora_di_entrata_prof[prof];
}

// L'ora buca e' possibile solo dopo le ore obbligatorie e se le ore che restano alla classe stanno ancora nei giorni
// dopo (un'ora buca chiude la giornata). Se no la classe deve fare lezione: senza docenti la ricerca torna indietro.
// (Come in main.)
int ora_buca_possibile(const int giorno, const int ora, const int cla) {
    return ora >= PRIME_N_ORE_OBBLIGATORIE
           && ore_classi[cla] - ore_classe_svolte[cla] <= actual_ore_giornaliere * (actual_giorni_settimana - 1 - giorno);
}

// s: l'indice della cella nell'ordine della ricerca; pos = cella_di[s] e' la cella (come in main)
void print_orario(const int s) {
    if (n_gen_max <= n_gen) return;
    // traccia: solo conteggio dei nodi e, ogni tanto, il progresso (non cambia la ricerca)
    if (!(++traccia_nodi & 0xFFFF)) traccia_progresso(s);
    if (s > traccia_pos_max) traccia_pos_max = s;  // la posizione piu' avanti raggiunta (solo traccia)
    if (traccia_con_nodi) traccia_nodo(s);

    if (s == orario_len) {
        // (solo nei solver nuovi) ogni classe deve aver fatto tutte le sue ore: in main questo controllo e' commentato
        // e una classe puo' chiudere la settimana con ore buche al posto di lezioni
        for (int c = 0; c < actual_classi_sezione; c++) if (ore_classe_svolte[c] != ore_classi[c]) return;
        n_gen++;
        if (f) print(orario, orario_len, n_gen, f);
        traccia_orario_trovato();
        return;
    }

    // le potature (potature/potature.h): la scelta nella cella prima lascia un resto ancora possibile?
    if (potatura_dopo(s)) {
        traccia_potatura(s, g_giorno(cella_di[s]), g_ora(cella_di[s]), g_classe(cella_di[s]), &potatura);
        return;
    }

    const int pos = cella_di[s];
    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base2d_ci = cla * INSEGNANTE_SZ;
    const int base2d_gc = giorno * CLASSE_SEZIONE_SZ;
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;
    const int base4d_queue = giorno * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + ora *
                             CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ;


    sort_queue_mul(base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(pos, giorno, ora, cla)) {
        int found_prof = 0;
        ore_classe_svolte[cla]++;
        const long long int queue_size = base4d_queue + queue_sz;
        for (int ii = base4d_queue; ii < queue_size; ii++) {
            QueueItem *it = queue_mul[ii];
            const int p1 = it->teacher;

            if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                prof_è_disponibile(base3d_goi, p1)
                // controlla ore svolte per materia, verifica che siano inferiori a quelle totali
                && deve_ancora_svolgere_ore_in_questa_classe(base2d_ci, p1)
                // un prof fa al max n ore in una classe
                && prof_fa_al_max_n_ore(giorno, ora, cla, p1)
                // se un prof fa più ore in una classe in un giorno, devono essere contigue
                && prof_fa_ore_contigue(giorno, ora, cla, p1)
                //se un prof ha due ore il sabato deve farle consecutive, altrimenti non la fa, essendo non contigue
                //ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO materie (classi) diverse ogni giorno
                && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(s, giorno, cla, p1)
                && verifica_ora_ingresso(p1, ora)
                // NB. non e possibile verificare qua di esaurire ore il sabato (?)
                && 1
            ) {
                // assert(iitem->priority > 0);

                for (int jj = ii + 1; jj < queue_size; jj++) {
                    QueueItem *jt = queue_mul[jj];
                    const int p2 = jt->teacher;
                    if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                        prof_è_disponibile(base3d_goi, p2)
                        && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(cla, p1, p2)
                        // un prof fa al max n ore in una classe
                        && prof_fa_al_max_n_ore(giorno, ora, cla, p2)
                        // se un prof fa più ore in una classe in un giorno, devono essere contigue
                        && prof_fa_ore_contigue(giorno, ora, cla, p2)
                        // poiche un prof se deve fare due ore no può farle spezzettate, e sabato è 'ulyimo giorno, non si puo mettere un altro prof,
                        // la seconda ora deve farla subito
                        && sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, p1, p2)
                        //ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO materie (classi) diverse ogni giorno
                        && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(s, giorno, cla, p2)
                        // (solo nei solver nuovi) anche il codocente deve essere gia' entrato
                        && verifica_ora_ingresso(p2, ora)
                        && 1
                    ) {
                        prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                        found_prof = 1;
                        orario[pos] = (teaching_slot){p1, p2};
                        print_orario(s + 1);
                        post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                    }
                }
                //non abbiamo trovato combinazioni di codocenza, verifica se all'insegnante mancano ore di docenza da solo
                if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                    && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)
                    && 1) {
                    prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                    found_prof = 1;
                    orario[pos] = (teaching_slot){p1, -1};
                    print_orario(s + 1);
                    post_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                }
            }
        }
        ore_classe_svolte[cla]--;
        if (!found_prof) {
            if (f) fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                    giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
            if (f) print(orario, orario_len, n_gen, f);
            traccia_backtrack(s, giorno, ora, cla, base2d_ci, base3d_goi, base4d_queue,
                              ora_buca_possibile(giorno, ora, cla));
        }
    } else if (ora < PRIME_N_ORE_OBBLIGATORIE) {
        // lo slot non puo' avere lezione e l'ora buca non e' ammessa: la ricerca torna indietro (solo traccia)
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
//   --txt FILE:         dove scrivere orario.txt (per esempio /dev/null)
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

    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    printf("Program started at: %s\n", ctime(&currentTime));
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
    //imposta la lunghezza dell'orario
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    prepara_ordine();
    traccia_imposta_ordine(cella_di, indice_di);  // la traccia della radice: "pos" e' il passo s
    prepara_potature(cella_di);


    traccia_dati();
    // con --txt /dev/null orario.txt non si scrive per niente (come in main): la ricerca e' la stessa, ma non perde
    // tempo a formattare la griglia a ogni backtrack (dal sito e' quasi tutto il tempo)
    f = strcmp(file_txt, "/dev/null") ? fopen(file_txt, "w") : NULL;
    print_orario(0);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed);
    fflush(stdout);
    traccia_fine(elapsed);
    traccia_chiudi();
    if (f) fclose(f);

    return 0;
}
