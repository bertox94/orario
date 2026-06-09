#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "database_initializers.h"
#include "printing.h"
#include "progressive_constraints.h"
#include "end_of_block_constraints.h"
#include "professor_functions.h"

long long int n_gen = 0;
long long int n_gen_ok = 30;
int orario_len;
int giorno_ora_classi_sz;

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

void print_orario(elem orario[], const int pos, const int tot, int prof_gia_in_classe_curr[],
                  int ore_classe_materia_curr[], int ore_insegnanti_classe_materia_curr[]) {
    if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        if (verifica_ore_classe_materia(ore_insegnanti_classe_materia_curr)
            /*&& verifica_ore_settimana_prof(ore_per_settimana_prof_curr)*/) {
            n_gen++;
            print_orario_form(orario, tot);
        }
        return;
    }

    pos % giorno_ora_classi_sz != 0 ? NULL : memset(prof_gia_in_classe_curr, 0, INSEGNANTE_SZ*sizeof(int));

    orario[pos] = (elem){-1, -1, -1, -1};
    print_orario(orario, pos + 1, tot, prof_gia_in_classe_curr, ore_classe_materia_curr,
                 ore_insegnanti_classe_materia_curr);

    for (int h = 0; h < actual_aula; h++) {
        for (int m = 0; m < actual_materie; m++) {
            //ci può essere un solo insegnante perciò parte da -1
            for (int i = -1; i < actual_insegnanti; i++) {
                for (int j = i + 1; j < actual_insegnanti; j++) {
                    if (prof_in_classe(i, j, prof_gia_in_classe_curr)
                        && insegnanti_ore_materia_adeguata(g_classe(pos), m, i, j)
                        && codocente_deve_essere_in_lab_no_aula(h, i, j)
                        && controllo_prof_insieme(i, j)
                        //&& ore_classe_materia_curr[g_classe(pos) * MATERIA_SZ + m] <
                        //ore_classe_materia[g_classe(pos) * MATERIA_SZ + m]
                        //&& ore_per_settimana_prof_curr[i] < ore_settimana_prof[i]
                        && 1) {
                        orario[pos] = (elem){h, m, i, j};

                        int ore_classe_materia_new[CLASSE_SEZIONE_SZ * MATERIA_SZ];
                        int prof_gia_in_classe_new[INSEGNANTE_SZ];
                        memcpy(ore_classe_materia_new, ore_insegnanti_classe_materia_curr,
                               CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
                        memcpy(prof_gia_in_classe_new, prof_gia_in_classe_curr, INSEGNANTE_SZ*sizeof(int));

                        if (!is_vuoto(i)) {
                            prof_gia_in_classe_new[i] = 1;
                        }
                        ore_classe_materia_new[g_classe(pos) * MATERIA_SZ + m]++;
                        prof_gia_in_classe_new[j] = 1;

                        print_orario(orario, pos + 1, tot, prof_gia_in_classe_new, ore_classe_materia_new,
                                     ore_insegnanti_classe_materia_curr);
                    }
                }
            }
        }
    }
}

int main(void) {
    inizializza_dati();
    inizializza_vincoli();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    giorno_ora_classi_sz = actual_classi_sezione;
    //test(100);

    print_intestazioni(orario_len);
    elem orario[orario_len];
    int ore_insegnanti_classe_materia_curr[CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ];
    int ore_classe_materia_curr[CLASSE_SEZIONE_SZ * MATERIA_SZ];
    int prof_gia_in_classe[INSEGNANTE_SZ];
    memset(orario, 0, orario_len*sizeof(int));
    memset(ore_insegnanti_classe_materia_curr, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ*sizeof(int));
    memset(prof_gia_in_classe, 0, INSEGNANTE_SZ*sizeof(int));
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len, prof_gia_in_classe, ore_classe_materia_curr,
                 ore_insegnanti_classe_materia_curr);

    printf("Totale generati: %llu\n", n_gen);
    fflush(stdout);

    return 0;
}
