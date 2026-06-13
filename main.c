#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "data_initialization/database_initializers.h"
#include "constraints/progressive_constraints.h"
#include "constraints/end_of_block_constraints.h"

long long int n_gen = 0;
long long int n_gen_ok = 1000000;
int orario_len;

int *int_alloc(size_t len) {
    int *p = malloc(len * sizeof(int));
    if (p == NULL) {
        printf("Errore allocazione memoria\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

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
                  int prof_in_classe[], int aule_occupate[], int classe_finisce_ultima_ora_num_giorni[]) {
    if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        if (verifica_ore_classe_materia(ora_classi_docenti_materia_effettuate)) {
            n_gen++;
            //printf("%lld\n", n_gen);
            print_orario_form(orario, tot, n_gen);
        }
        return;
    }

    //print_orario_form(orario,pos, n_gen);

    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base3dprof_impegnati = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;
    const int base3daule_impegnate = giorno * ORA_GIORNALIERA_SZ * AULA_SZ + ora * AULA_SZ;
    if (still_activities_for_this_class_and_hour(ora, cla, classe_finisce_ultima_ora_num_giorni)) {
        for (int h = 0; h < actual_aula; h++) {
            if (!aule_occupate[base3daule_impegnate + h])
                for (int m = 0; m < actual_materie; m++) {
                    int i = -1, j = 0;
                    const int base3dore_classi = cla * MATERIA_SZ * INSEGNANTE_SZ + m * INSEGNANTE_SZ;

                    //insegnanti da soli
                    for (; j < actual_insegnanti; j++) {
                        if ( //solo docenti
                            is_docente(j)
                            // verifica che nella stessa ora ciascun prof non sia già in altre classi
                            && !prof_in_classe[base3dprof_impegnati + j]
                            // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                            && ora_classi_docenti_materia_effettuate[base3dore_classi + j] <
                            ore_insegnanti_classe_materia_lonely[base3dore_classi + j]
                            && 1) {
                            orario[pos] = (elem){h, m, i, j};
                            ora_classi_docenti_materia_effettuate[base3dore_classi + j]++;
                            prof_in_classe[base3dprof_impegnati + j] = 1;
                            aule_occupate[base3daule_impegnate + h] = 1;
                            if (ora == actual_ore_giornaliere - 1)
                                classe_finisce_ultima_ora_num_giorni[cla]++;

                            print_orario(orario, pos + 1, tot, ora_classi_docenti_materia_effettuate, prof_in_classe,
                                         aule_occupate, classe_finisce_ultima_ora_num_giorni);

                            ora_classi_docenti_materia_effettuate[base3dore_classi + j]--;
                            prof_in_classe[base3dprof_impegnati + j] = 0;
                            aule_occupate[base3daule_impegnate + h] = 0;
                            if (ora == actual_ore_giornaliere - 1)
                                classe_finisce_ultima_ora_num_giorni[cla]--;
                        }
                    }

                    //codocenze
                    if (laboratori[h])
                        for (i = 0; i < actual_insegnanti; i++) {
                            if (!prof_in_classe[base3dprof_impegnati + i]
                                && ora_classi_docenti_materia_effettuate[base3dore_classi + i] <
                                ore_insegnanti_classe_materia[base3dore_classi + i])
                                for (j = i + 1; j < actual_insegnanti; j++) {
                                    if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                                        !prof_in_classe[base3dprof_impegnati + j]
                                        // non si possono avere due docenti teorici insieme, nè due codocenti, ...
                                        && controllo_prof_insieme(i, j)
                                        // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                                        && ora_classi_docenti_materia_effettuate[base3dore_classi + j] <
                                        ore_insegnanti_classe_materia[base3dore_classi + j]
                                        && 1) {
                                        orario[pos] = (elem){h, m, i, j};
                                        ora_classi_docenti_materia_effettuate[base3dore_classi + i]++;
                                        ora_classi_docenti_materia_effettuate[base3dore_classi + j]++;
                                        prof_in_classe[base3dprof_impegnati + i] = 1;
                                        prof_in_classe[base3dprof_impegnati + j] = 1;
                                        aule_occupate[base3daule_impegnate + h] = 1;
                                        if (ora == actual_ore_giornaliere - 1)
                                            classe_finisce_ultima_ora_num_giorni[cla]++;

                                        print_orario(orario, pos + 1, tot, ora_classi_docenti_materia_effettuate,
                                                     prof_in_classe, aule_occupate,
                                                     classe_finisce_ultima_ora_num_giorni);

                                        ora_classi_docenti_materia_effettuate[base3dore_classi + i]--;
                                        ora_classi_docenti_materia_effettuate[base3dore_classi + j]--;
                                        prof_in_classe[base3dprof_impegnati + i] = 0;
                                        prof_in_classe[base3dprof_impegnati + j] = 0;
                                        aule_occupate[base3daule_impegnate + h] = 0;
                                        if (ora == actual_ore_giornaliere - 1)
                                            classe_finisce_ultima_ora_num_giorni[cla]--;
                                    }
                                }
                        }
                }
        }
    }
    if (ora >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = (elem){-1, -1, -1, -1};
        print_orario(orario, pos + 1, tot, ora_classi_docenti_materia_effettuate, prof_in_classe, aule_occupate,
                     classe_finisce_ultima_ora_num_giorni);
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
    int *ora_classi_docenti_materie_effettuate = int_alloc(CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ);
    memset(ora_classi_docenti_materie_effettuate, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ * sizeof(int));
    int *prof_in_classe = int_alloc(GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ);
    memset(prof_in_classe, 0, GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ*sizeof(int));
    int *aule_occupate = int_alloc(GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * AULA_SZ);
    memset(aule_occupate, 0, GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * AULA_SZ*sizeof(int));
    int *classe_finisce_tardi_num = int_alloc(CLASSE_SEZIONE_SZ);
    memset(classe_finisce_tardi_num, 0, CLASSE_SEZIONE_SZ*sizeof(int));
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len, ora_classi_docenti_materie_effettuate, prof_in_classe, aule_occupate,
                 classe_finisce_tardi_num);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s.", n_gen, (int) elapsed / 60, (int) elapsed % 60);
    fflush(stdout);

    return 0;
}
