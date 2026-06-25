#include <assert.h>
#include <iso646.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"

#define SCHEDULES_PER_GB 60000
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;
long long int n_gen_ok = SCHEDULES_PER_GB;
int orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ... GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1] = -1
};

int orario_len;
FILE *f;

typedef struct {
    int aula;
    int materia;
    int insegnante;
} teaching_slot;

int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

/*End of block constraints*/
int verifica_ore_classe_materia(void) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ * INSEGNANTE_SZ; i++)
        if (ore_classi_docenti[i] != ore_classi_docenti_svolte[i]) {
            printf("Wrong: Classe: %s, Teacher: %s %d but got %d\n",
                   classe_sezione[i / INSEGNANTE_SZ % CLASSE_SEZIONE_SZ],
                   insegnanti[i % INSEGNANTE_SZ],
                   ore_classi_docenti[i], ore_classi_docenti_svolte[i]);
            return 0;
        }
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

int solo_lezioni_consecutive(int pos, int base3d_gci, int cla, int insegnante) {
    //no_teacher(x) -> True
    //teacher(x,p) -> teacher(x, p+1)
    if (ore_prof_giorno_classe[base3d_gci + insegnante] == 0) return 1;
    // no need to check pos - actual_classi_sezione > 0 since the first hour has no teacher,
    // so it would just have returned
    return ultimo_prof_classe[cla] == insegnante;
}

int *int_alloc(size_t len) {
    int *p = malloc(len * sizeof(int));
    if (p == NULL) {
        printf("Errore allocazione memoria\n");
        exit(EXIT_FAILURE);
    }
    return p;
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
    if (ore_prof_giorno_classe[base3d_gci + j] >= MAX_ORE_STESSO_PROF_GIORNO) printf("Già fatto n ore oggi ");
    printf("\n");
}

void print_orario(const int pos) {
    if (n_gen_ok <= n_gen) return;


    if (pos == orario_len) {
        if (verifica_ore_classe_materia()) {
            n_gen++;
            //printf("%lld ", n_gen);
            print(orario, pos, n_gen, f);
        }
        return;
    }

    //print(orario, pos, n_gen, NULL);

    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;
    const int base2d_ci = cla * INSEGNANTE_SZ;
    const int base2d_gc = giorno * CLASSE_SEZIONE_SZ;
    const int base3d_gci = giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ;
    const int base4d_queue = giorno * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + ora * CLASSE_SEZIONE_SZ *
                             INSEGNANTE_SZ + cla * INSEGNANTE_SZ;
    if (activities_for_this_class_and_hour(pos, giorno, ora, cla)) {
        const long long int jjmax = base4d_queue + actual_insegnanti;
        for (int jj = base4d_queue; jj < jjmax; jj++) {
            int max_idx = jj; // Track the INDEX of the maximum element, not just the value

            /*       for (int p = base4d_queue; p < jj; p++) {
                       for (int q = p + 1; q < jjmax; q++) {
                           if (!(queue_mul[p]->priority >= queue_mul[q]->priority))
                               exit(-1);
                       }
                   }
       */
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

            int j = queue_mul[jj]->teacher;

            // in case the previous teacher needed two hours they must be done one after another
            // maybe need a schedule like all the teachers
            if (giorno == actual_giorni_settimana - 1 && ora > 0) {
                int prev_prof = orario[pos - actual_classi_sezione];
                if (prev_prof != -1 && ore_classi_docenti[base2d_ci + prev_prof] - ore_classi_docenti_svolte[
                        base2d_ci + prev_prof] == 1) {
                    j = prev_prof;
                }
            }
            if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                !prof_gia_impegnato_giorno_ora[base3d_goi + j]
                // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                && ore_classi_docenti_svolte[base2d_ci + j] < ore_classi_docenti[base2d_ci + j]
                //max 2 ore stesso prof in ciascuna classe ogni giorno, anche se non contigue
                && ore_prof_giorno_classe[base3d_gci + j] < MAX_ORE_STESSO_PROF_GIORNO
                && solo_lezioni_consecutive(pos, base3d_gci, cla, j)
                && 1) {
                ore_classi_docenti_svolte[base2d_ci + j]++;
                prof_gia_impegnato_giorno_ora[base3d_goi + j] = 1;
                ore_classe_svolte[cla]++;
                ore_docenti_svolte[j]++;
                ore_prof_giorno_classe[base3d_gci + j]++;
                queue_mul[jj]->priority--;
                int ultimo_prof_copy = ultimo_prof_classe[cla];
                ultimo_prof_classe[cla] = j;

                /* QueueItem new[actual_insegnanti];
                 memcpy(new, queue, sizeof(QueueItem) * actual_insegnanti);
                 new[jj].priority--;*/

                orario[pos] = j;
                print_orario(pos + 1);

                queue_mul[jj]->priority++;
                ore_classi_docenti_svolte[base2d_ci + j]--;
                prof_gia_impegnato_giorno_ora[base3d_goi + j] = 0;
                ore_classe_svolte[cla]--;
                ore_docenti_svolte[j]--;
                ore_prof_giorno_classe[base3d_gci + j]--;
                ultimo_prof_classe[cla] = ultimo_prof_copy;
            }
        }
    }


    if (ora >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = -1;
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

    // if you initialize with less elements it will set the remaming elements to 0.
    QueueItem *queue[INSEGNANTE_SZ] = {0};

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
    }

    //f = fopen("orario.txt", "w");
    f = fopen("orario.txt", "w");
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(0);
    fclose(f);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f KHz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed / 1000);
    fflush(stdout);

    return 0;
}
