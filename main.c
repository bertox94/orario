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
long long int n_gen_ok = 300000000;
int orario_len;
int giorno_ora_classi_sz;

void print_orario(elem orario[], const int pos, const int tot) {
    if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        // da verificare che non ci siano ore buche per le classi, a meo che sia l'ultima ora
        if (verifica_ore_classe_materia(orario, tot)) {
            n_gen++;
            print_orario_form(orario, tot);
        }
        return;
    }

    orario[pos] = (elem){-1, -1, -1, -1};
    print_orario(orario, pos + 1, tot);

    for (int h = 0; h < actual_aula; h++) {
        for (int m = 0; m < actual_materie; m++) {
            //ci può essere un solo insegnante perciò parte da -1
            for (int i = -1; i < actual_insegnanti; i++) {
                for (int j = i + 1; j < actual_insegnanti; j++) {
                    if ( // verifica che nella stessa ora i prof i,j non siano già in altre classi
                        prof_in_classe(orario, pos, i, j)
                        && codocente_deve_essere_in_lab_no_aula(h, i, j)
                        // non si possono avere due docenti teorici, nè due codocenti, ...
                        && controllo_prof_insieme(i, j)
                        // docente non ha raggiunto le ore di insegnamento per quella materia e classe
                        && is_teacher_eligible_for_class(orario, pos, m, i)
                        && is_teacher_eligible_for_class(orario, pos, m, j)
                        && 1) {
                        orario[pos] = (elem){h, m, i, j};

                        print_orario(orario, pos + 1, tot);
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
    memset(orario, 0, orario_len*sizeof(int));
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len);

    printf("Totale generati: %llu\n", n_gen);
    fflush(stdout);

    return 0;
}
