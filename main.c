#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"

#define SCHEDULES_PER_GB 135500
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;
long long int n_gen_ok = 10000000; //= SCHEDULES_PER_GB * 5;
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

/*End of block constraints*/
int verifica_ore_classe_materia(void) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ * INSEGNANTE_SZ; i++)
        if (ore_classi_docenti[i] != ore_classi_docenti_svolte[i]) {
            /*printf("Wrong: Classe: %s, Materia: %s, Teacher: %s %d but got %d\n",
                   classe_sezione[i / MATERIA_SZ / INSEGNANTE_SZ % CLASSE_SEZIONE_SZ],
                   materie[i / INSEGNANTE_SZ % MATERIA_SZ],
                   insegnanti[i % INSEGNANTE_SZ],
                   ore_insegnanti_classe_materia[i], ora_classi_materie_effettuate[i]);*/
            return 0;
        }
    return 1;
}

int *int_alloc(size_t len) {
    int *p = malloc(len * sizeof(int));
    if (p == NULL) {
        printf("Errore allocazione memoria\n");
        exit(EXIT_FAILURE);
    }
    return p;
}


void print_orario(const int pos, head_tail_queue priority_queues[]) {
    if (n_gen_ok <= n_gen) return;


    if (pos == orario_len) {
        if (verifica_ore_classe_materia()) {
            n_gen++;
            //print(orario, pos, n_gen);
        }
        return;
    }
    //print(orario, pos, n_gen);

    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;
    const int base2d_ci = cla * INSEGNANTE_SZ;
    const int base2d_gc = giorno * CLASSE_SEZIONE_SZ;
    const int base3d_gci = giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ;
    if (activities_for_this_class_and_hour(pos, giorno, ora, cla)) {
        for (int jj = 0; jj < actual_insegnanti; jj++) {
            sort_queue_descending(&priority_queues[cla]);
            int j = peek(priority_queues[cla].head, jj);
            if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                !prof_gia_impegnato_giorno_ora[base3d_goi + j]
                // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                && ore_classi_docenti_svolte[base2d_ci + j] < ore_classi_docenti[base2d_ci + j]
                //max 2 ore stesso prof in ciascuna classe ogni giorno, anche se non contigue
                && ore_prof_giorno_classe[base3d_gci + j] < MAX_ORE_STESSA_MATERIA_GIORNO
                && 1) {
                ore_classi_docenti_svolte[base2d_ci + j]++;
                prof_gia_impegnato_giorno_ora[base3d_goi + j] = 1;
                ore_classe_svolte[cla]++;
                ore_docenti_svolte[j]++;
                ore_prof_giorno_classe[base3d_gci + j]++;

                set_priority(priority_queues[cla].head, giorno, cla, j);


                orario[pos] = j;
                print_orario(pos + 1, priority_queues);

                ore_classi_docenti_svolte[base2d_ci + j]--;
                prof_gia_impegnato_giorno_ora[base3d_goi + j] = 0;
                ore_classe_svolte[cla]--;
                ore_docenti_svolte[j]--;
                ore_prof_giorno_classe[base3d_gci + j]--;
            }
        }
    }


    if (ora >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = -1;
        n_ore_buche[base2d_gc + cla]++;
        print_orario(pos + 1, priority_queues);
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

    //TODO: priority queue for each class ?
    //head_tail_queue ret = initialize_priority_queue(actual_insegnanti);

    head_tail_queue queues[actual_classi_sezione];

    for (int i = 0; i < actual_classi_sezione; i++) {
        queues[i] = initialize_priority_queue(i);
        sort_queue_descending(&queues[i]);
        print_prioritt_queue(queues[i].head, queues[i].tail);
    }

    //f = fopen("orario.txt", "w");
    f = fopen("/Volumes/ESD-USB/orario.txt", "w");
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(0, queues);
    fclose(f);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s.", n_gen, (int) elapsed / 60, (int) elapsed % 60);
    fflush(stdout);

    return 0;
}
