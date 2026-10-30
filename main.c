#include <assert.h>
#include <iso646.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <sys/time.h>

#include "priority_queue/prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;
long long int n_gen_max = SCHEDULES_PER_GB / 1;
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ...
        GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1
    ] = (teaching_slot){-1, -1}
};

int orario_len;
FILE *f;


int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

/*End of block constraints*/
int verifica_ore_classe_materia(void) {
    // 1. Check standard teacher hours (2D array flattened logic)
    int idx_2d = 0;
    for (int c = 0; c < CLASSE_SEZIONE_SZ; c++) {
        for (int t = 0; t < INSEGNANTE_SZ; t++) {
            if (ore_classi_docenti[idx_2d] != ore_classi_docenti_svolte[idx_2d]
            ) {
                print(orario, orario_len, n_gen, f);
                fprintf(f, "Wrong: Classe: %s, Teacher: %s %d but got %d\n\n",
                        classe_sezione[c],
                        insegnanti[t],
                        ore_classi_docenti[idx_2d],
                        ore_classi_docenti_svolte[idx_2d]);
                return 0;
            }
            idx_2d++;
        }
    }
    /*
        // 2. Check co-teacher hours (3D array flattened logic)
        int idx_3d = 0;
        for (int c = 0; c < CLASSE_SEZIONE_SZ; c++) {
            for (int t1 = 0; t1 < INSEGNANTE_SZ; t1++) {
                for (int t2 = 0; t2 < INSEGNANTE_SZ; t2++) {
                    if (ore_classi_docenti_codocenti[idx_3d] != ore_classi_docenti_codocenti_svolte[idx_3d]) {
                        printf("Wrong: Classe: %s, Codocente: %s, Teacher: %s %d but got %d\n",
                               classe_sezione[c],
                               insegnanti[t2], // t2 matches (idx_3d % INSEGNANTE_SZ)
                               insegnanti[t1], // t1 matches ((idx_3d / INSEGNANTE_SZ) % INSEGNANTE_SZ)
                               ore_classi_docenti_codocenti[idx_3d],
                               ore_classi_docenti_codocenti_svolte[idx_3d]);
                        return 0;
                    }
                    idx_3d++;
                }
            }
        }*/

    return 1;
}

/*Progressive contraints*/
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

    //le ore di lezione in ogni clase rimanenti per ogni insegnante devono essere <= a 2*giorni rimanenti
    /*
        int base2dc = cla * INSEGNANTE_SZ;
        for (int i = 0; i < actual_insegnanti; i++) {
            if (ore_insegnante_classi[base2dc + i] > 0
                && ore_insegnante_classi[base2dc + i] - ore_classi_docenti_svolte[base2dc + i] >
                MAX_ORE_STESSA_MATERIA_GIORNO * (actual_giorni_settimana - gio))
                return 0;
        }
    */
    return 1;
}

int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        int prev_prof1 = orario[pos - actual_classi_sezione].insegnante1;
        int prev_prof2 = orario[pos - actual_classi_sezione].insegnante2;

        if (prev_prof1 != -1 && prev_prof2 == -1) {
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

        if (prev_prof1 != -1 && prev_prof2 == -1) {
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

void print_up_to(QueueItem arr[], int pos) {
    for (int i = 0; i < pos; i++) {
        printf("%d)\t %d %.0f ", i, arr[i].teacher, arr[i].priority);
    }
    printf("\n");
}


void debug(const int base3d_goi, const int base2d_ci, const int base3d_gci, int j) {
    if (ore_classi_docenti[base2d_ci + j] == 0) return;
    printf("Insegnante: %s\n", insegnanti[j]);
    if (prof_gia_impegnato_giorno_ora[base3d_goi + j]) printf("Già impegnato ");
    if (ore_classi_docenti_svolte[base2d_ci + j] >= ore_classi_docenti[base2d_ci + j]) printf("Ore eccessive ");
    printf("\n");
}

int assert_non_decreasing(const int base4d_queue, const long long int jjmax, int jj) {
    for (int p = base4d_queue; p < jj; p++) {
        for (int q = p + 1; q < jjmax; q++) {
            if (!(queue_mul[p]->priority >= queue_mul[q]->priority))
                exit(-1);
        }
    }
    return 0;
}

/*
QueueItem *set_max_at_of_remaining(const long long int jjmax, int jj) {
    int max_idx = jj; // Track the INDEX of the maximum element, not just the value

    for (int k = jj + 1; k < jjmax; k++) {
        // k can start at jj + 1
        if (queue_mul[k]->priority > queue_mul[max_idx]->priority) {
            max_idx = k; // Found a higher priority, record its position
        }
    }

    // ONLY SWAP ONCE after finding the true maximum for this position
    if (max_idx != jj) {
        QueueItem *tmp = queue_mul[jj];
        queue_mul[jj] = queue_mul[max_idx];
        queue_mul[max_idx] = tmp;
    }

    return queue_mul[jj];
}
*/

// Comparison function for qsort
int compare_queue_items(const void *a, const void *b) {
    // Cast the void pointers to pointers to QueueItem*
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;

    // Sort by priority (descending: highest priority first)
    if (itemA->priority < itemB->priority) return 1;
    if (itemA->priority > itemB->priority) return -1;
    return 0;
}

// Assuming you know the number of elements in the array, say 'n'
void sort_queue_mul(size_t start, size_t n) {
    // qsort has an average time complexity of O(N log N)
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

int prof_non_ha_ancora_insegnato_in_questa_classe_oppure_ha_insegnato_l_ora_precedente(
    int giorno, int ora, int cla, int prof) {
    int start = giorno * actual_ore_giornaliere * actual_classi_sezione + cla;

    // controlla fino a
    int end = giorno * actual_ore_giornaliere * actual_classi_sezione + (ora - 2) * actual_classi_sezione + cla;

    for (int i = start; i <= end; i += actual_classi_sezione) {
        if (orario[i].insegnante1 == prof || orario[i].insegnante2 == prof) {
            return 0;
        }
    }
    return 1;
}

int prof_è_disponibile(const int base3d_goi, int p1) {
    return !prof_gia_impegnato_giorno_ora[base3d_goi + p1];
}

int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) {
    return ore_classi_docenti_svolte[base2d_ci + p1] < ore_classi_docenti[base2d_ci + p1];
}

int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int base3d_cii, const int prof1,
                                                                   const int prof2) {
    const int p1 = fmin(prof1, prof2);
    const int p2 = fmax(prof1, prof2);
    return ore_classi_codocenza_svolte[base3d_cii + p1 * INSEGNANTE_SZ + p2] <
           ore_classi_codocenza[base3d_cii + p1 * INSEGNANTE_SZ + p2];
}

void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
}

void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int base3d_cii, QueueItem *it,
                                     QueueItem *jt) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    jt->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + jt->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + jt->teacher]++;
    const int p1 = fmin(it->teacher, jt->teacher);
    const int p2 = fmax(it->teacher, jt->teacher);
    ore_classi_codocenza_svolte[base3d_cii + p1 * INSEGNANTE_SZ + p2]++;
}

void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
}

void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int base3d_cii, QueueItem *it,
                                     QueueItem *jt) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    jt->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + jt->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + jt->teacher]--;
    const int p1 = fmin(it->teacher, jt->teacher);
    const int p2 = fmax(it->teacher, jt->teacher);
    ore_classi_codocenza_svolte[base3d_cii + p1 * INSEGNANTE_SZ + p2]--;
}

int è_un_docente_che_insegna_da_solo_in_questa_classe(const int base2d_ci, const int prof) {
    return classi_docenti_teorici[base2d_ci + prof];
}

int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < ore_classi_docenti_alone[
               base2d_ci + prof];
}

int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, const int prof) {
    // Assuming start calculation based on your layout structure
    int start = giorno * actual_ore_giornaliere * actual_classi_sezione;

    // Fixed-size array instead of VLA (Variable Length Array).
    // Since max distinct classes allowed is n, we only ever need n elements.
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO];
    int classi_sz = 0;

    // Add the initial/current class
    classi[classi_sz++] = cla;

    for (int i = start; i < pos; i++) {
        if (orario[i].insegnante1 == prof || orario[i].insegnante2 == prof) {
            int current_class = g_classe(i);

            // Check if the class is already recorded
            int found = 0;
            for (int j = 0; j < classi_sz; j++) {
                if (classi[j] == current_class) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                // If we already have 'max_diff_class' distinct classes and find another one, fail early
                if (classi_sz >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) {
                    return 0;
                }
                classi[classi_sz++] = current_class;
            }

            // Jump to the next hour slot (-1 to compensate for the loop's i++)
            i = start + (g_ora(i) + 1) * actual_classi_sezione - 1;
        }
    }

    return 1;
}

int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= ora_di_entrata_prof[prof];
}

void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;

    //print(orario, pos, n_gen, f);
    if (pos == orario_len) {
        //if (verifica_ore_classe_materia()) { //not needed ? all hours should match
        n_gen++;
        //printf("%lld \n", n_gen);
        print(orario, pos, n_gen, f);
        //  }
        return;
    }

    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base2d_ci = cla * INSEGNANTE_SZ;
    const int base2d_gc = giorno * CLASSE_SEZIONE_SZ;
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;
    const int base3d_cii = cla * INSEGNANTE_SZ * INSEGNANTE_SZ;
    const int base4d_queue = giorno * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ + ora *
                             CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ * INSEGNANTE_SZ;


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
                // un prof non puo vedere una classe due volte se non sono contigue
                && prof_non_ha_ancora_insegnato_in_questa_classe_oppure_ha_insegnato_l_ora_precedente(
                    giorno, ora, cla, p1)
                //se un prof ha due ore il sabato deve farle consecutive, altrimenti non la fa, essendo non contigue
                //ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO materie (classi) diverse ogni giorno
                && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p1)
                && verifica_ora_ingresso(p1, ora)
                // NB. non e possibile verificare qua di esaurire ore il sabato
                && 1
            ) {
                // assert(iitem->priority > 0);

                for (int jj = ii + 1; jj < queue_size; jj++) {
                    QueueItem *jt = queue_mul[jj];
                    const int p2 = jt->teacher;
                    if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                        prof_è_disponibile(base3d_goi, p2)
                        && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(base3d_cii, p1, p2)
                        // prof se fa due ore devono essere attaccate
                        && prof_non_ha_ancora_insegnato_in_questa_classe_oppure_ha_insegnato_l_ora_precedente(
                            giorno, ora, cla, p2)
                        // poiche un prof se deve fare due ore no può farle spezzettate, e sabato è 'ulyimo giorno, non si puo mettere un altro prof,
                        // la seconda ora deve farla subito
                        && sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, p1, p2)
                        //ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO materie (classi) diverse ogni giorno
                        && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p2)
                        && 1
                    ) {
                        prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, base3d_cii, it, jt);
                        found_prof = 1;
                        orario[pos] = (teaching_slot){p1, p2};
                        print_orario(pos + 1);
                        post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, base3d_cii, it, jt);
                    }
                }
                //non abbiamo trovato combinazioni di codocenza, verifica se all'insegnante mancano ore di docenza da solo
                if (è_un_docente_che_insegna_da_solo_in_questa_classe(base2d_ci, p1)
                    && deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                    && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)
                    && 1) {
                    prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it);
                    found_prof = 1;
                    orario[pos] = (teaching_slot){p1, -1};
                    print_orario(pos + 1);
                    post_chiamata_ricorsiva(base2d_ci, base3d_goi, it);
                }
            }
        }
        ore_classe_svolte[cla]--;
        if (!found_prof) {
            fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                    giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
            print(orario, pos, n_gen, f);
        }
    }


    if (ora >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = (teaching_slot){-1, -1};
        n_ore_buche[base2d_gc + cla]++;
        print_orario(pos + 1);
        n_ore_buche[base2d_gc + cla]--;
    }
}

int main(void) {
    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    printf("Program started at: %s\n", ctime(&currentTime));
    inizializza_dati();
    stampa_dati();
    //imposta la lunghezza dell'orario
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    //test(100);

    /*  // if you initialize with less elements it will set the remaming elements to 0.
      QueueItem *queue[INSEGNANTE_SZ] = {0};

      // Should do this while setting classi e insegnanti
      initialize_queue_arr(queue);
      print_priority_queue(queue);

      // 2. Extracted the total dimension to avoid recalculating
      const size_t num_blocks = GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ;
      const size_t total_elements = num_blocks * INSEGNANTE_SZ;

      // 3. Type-safe malloc (no explicit cast needed in C, but use sizeof(*ptr))
      queue_mul = malloc(total_elements * sizeof(*queue_mul));

      if (queue_mul == NULL) {
          // Always handle allocation failures!
          fprintf(stderr, "Memory allocation failed\n");
          return 0;
      }

      // 4. Highly optimized copying using memcpy instead of a nested loop
      for (size_t i = 0; i < num_blocks; i++) {
          memcpy(&queue_mul[i * INSEGNANTE_SZ], queue, sizeof(queue));
      }*/


    f = fopen("orario.txt", "w");
    //f = stdout;
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(0);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed);
    fflush(stdout);
    fclose(f);

    return 0;
}
