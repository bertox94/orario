#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "data_initialization/database_initializers.h"
#include "constraints/progressive_constraints.h"
#include "constraints/end_of_block_constraints.h"

long long int n_gen = 0;
long long int n_gen_ok = 10;
int orario_len;


void print_orario_form(const elem orario[], const int tot, const long attempt) {
    time_t currentTime;
    time(&currentTime); // Get the current time

    printf("Current time: %s", ctime(&currentTime));
    printf("Orario n. %lu\n", attempt);
    printf("Giorno:     ");
    for (int i = 0; i < tot; i++)
        printf(" %c%c",
               giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][0],
               giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][1]);
    printf("\n");
    printf("Ora:        ");
    for (int i = 0; i < tot; i++)
        printf(" %s", ora_giornaliera[i / actual_classi_sezione % actual_ore_giornaliere]);
    printf("\n");

    printf("Classe:     ");
    for (int i = 0; i < tot; i++)
        printf(" %s", classe_sezione[i % actual_classi_sezione]);
    printf("\n");

    printf("Aula:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].aula == -1)
            printf("  -");
        else
            printf("  %d", orario[i].aula);
    }
    printf("\n");

    printf("Materia:    ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].materia == -1)
            printf("  -");
        else
            printf("%c%c%c", materie[orario[i].materia][0], materie[orario[i].materia][1],
                   materie[orario[i].materia][2]);
    }
    printf("\n");

    printf("Prof:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].insegnante1 == -1)
            printf("  -");
        else
            printf("%c%c%c", insegnanti[orario[i].insegnante1][0], insegnanti[orario[i].insegnante1][1],
                   insegnanti[orario[i].insegnante1][2]);
    }
    printf("\n");
    printf("Prof:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].insegnante2 == -1)
            printf("  -");
        else
            printf("%c%c%c", insegnanti[orario[i].insegnante2][0], insegnanti[orario[i].insegnante2][1],
                   insegnanti[orario[i].insegnante2][2]);
    }
    printf("\n            ");
    for (int i = 0; i < tot; i++) {
        printf("---");
    }
    printf("\n");
}

void print_orario(elem orario[], const int pos, const int tot, int ora_classi_docenti_materia_effettuate[],
                  int prof_in_classe[]) {
    //if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        if (verifica_ore_classe_materia(orario, tot)) {
            n_gen++;
            printf("%lld\n", n_gen);
            //print_orario_form(orario, tot, n_gen);
        }
        return;
    }

    print_orario_form(orario, pos, n_gen);

    if (still_activities_for_today(orario, pos)) {
        for (int h = 0; h < actual_aula; h++) {
            for (int m = 0; m < actual_materie; m++) {
                int i = -1, j = 0;
                const int base3dore_classi = g_classe(pos) * MATERIA_SZ * INSEGNANTE_SZ + m * INSEGNANTE_SZ;
                const int base3dprof_impegnati =
                        g_giorno(pos) * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + g_ora(pos) * INSEGNANTE_SZ;
                //insegnanti da soli
                for (; j < actual_insegnanti; j++) {
                    if ( //solo docenti
                        is_docente(j)
                        // verifica che nella stessa ora ciascun prof non sia già in altre classi
                        && !prof_in_classe[base3dprof_impegnati + j]
                        // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                        && ora_classi_docenti_materia_effettuate[base3dore_classi + j] <
                        ore_insegnanti_classe_materia_lonely[
                            base3dore_classi + j]
                        && 1) {
                        orario[pos] = (elem){h, m, i, j};
                        ora_classi_docenti_materia_effettuate[base3dore_classi + j]++;
                        prof_in_classe[base3dprof_impegnati + j] = 1;

                        print_orario(orario, pos + 1, tot, ora_classi_docenti_materia_effettuate, prof_in_classe);

                        ora_classi_docenti_materia_effettuate[base3dore_classi + j]--;
                    }
                }

                //codocenze
                for (i = 0; i < actual_insegnanti; i++) {
                    for (j = i + 1; j < actual_insegnanti; j++) {
                        if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                            !prof_in_classe[base3dprof_impegnati + i]
                            && !prof_in_classe[base3dprof_impegnati + j]
                            // le codocenze sono in solo laboratorio
                            && laboratori[h]
                            // non si possono avere due docenti teorici insieme, nè due codocenti, ...
                            && controllo_prof_insieme(i, j)
                            // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                            && ora_classi_docenti_materia_effettuate[base3dore_classi + i] <
                            ore_insegnanti_classe_materia[
                                base3dore_classi + i]
                            && ora_classi_docenti_materia_effettuate[base3dore_classi + j] <
                            ore_insegnanti_classe_materia[
                                base3dore_classi + j]
                            && 1) {
                            orario[pos] = (elem){h, m, i, j};
                            ora_classi_docenti_materia_effettuate[base3dore_classi + i]++;
                            ora_classi_docenti_materia_effettuate[base3dore_classi + j]++;
                            prof_in_classe[base3dprof_impegnati + i] = 1;
                            prof_in_classe[base3dprof_impegnati + j] = 1;
                            print_orario(orario, pos + 1, tot, ora_classi_docenti_materia_effettuate, prof_in_classe);

                            ora_classi_docenti_materia_effettuate[base3dore_classi + i]--;
                            ora_classi_docenti_materia_effettuate[base3dore_classi + j]--;
                        }
                    }
                }
            }
        }
    }
    if (pos / giorno_ora_classi_sz % actual_ore_giornaliere >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = (elem){-1, -1, -1, -1};
        print_orario(orario, pos + 1, tot, ora_classi_docenti_materia_effettuate, prof_in_classe);
    }
}

int main(void) {
    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    printf("Program started at: %s\n", ctime(&currentTime));
    inizializza_dati();
    inizializza_vincoli();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    giorno_ora_classi_sz = actual_classi_sezione;
    //test(100);

    //print_intestazioni(orario_len);
    elem orario[orario_len];
    //memset(orario, (elem){-1,-1,-1,-1}, orario_len*sizeof(int));
    int ora_classi_docenti_materie_effettuate[CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ] = {};
    memset(ora_classi_docenti_materie_effettuate, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ * sizeof(int));
    int prof_in_classe[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ] = {};
    memset(prof_in_classe, 0, GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ*sizeof(int));
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len, ora_classi_docenti_materie_effettuate, prof_in_classe);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s.", n_gen, (int) elapsed / 60, (int) elapsed % 60);
    fflush(stdout);

    return 0;
}
