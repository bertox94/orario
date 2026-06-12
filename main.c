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

void print_orario(elem orario[], const int pos, const int tot) {
    if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        if (verifica_ore_classe_materia(orario, tot)) {
            n_gen++;
            // printf("%d\n", n_gen);
            print_orario_form(orario, tot, n_gen);
        }
        return;
    }

    if (still_activities_for_today(orario, pos)) {
        for (int h = 0; h < actual_aula; h++) {
            for (int m = 0; m < actual_materie; m++) {
                //ci può essere un solo insegnante perciò parte da -1
                for (int i = -1; i < actual_insegnanti; i++) {
                    for (int j = i + 1; j < actual_insegnanti; j++) {
                        if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                            prof_in_classe(orario, pos, i, j)
                            // se non è un laboratorio, non possono esserci codocenti
                            && (laboratori[h] || !is_codocente(i) && !is_codocente(j))
                            // non si possono avere due docenti teorici insieme, nè due codocenti, ...
                            && controllo_prof_insieme(i, j)
                            // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                            && verify_teacher_hours(orario, pos, g_classe(pos), m, i, j)
                            && 1) {
                            orario[pos] = (elem){h, m, i, j};

                            print_orario(orario, pos + 1, tot);
                        }
                    }
                }
            }
        }
    }
    if (pos / giorno_ora_classi_sz % actual_ore_giornaliere >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = (elem){-1, -1, -1, -1};
        print_orario(orario, pos + 1, tot);
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
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s.", n_gen, (int) elapsed / 60, (int) elapsed % 60);
    fflush(stdout);

    return 0;
}
