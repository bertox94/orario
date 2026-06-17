#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "data_initialization/database_initializers.h"
#include "constraints/progressive_constraints.h"
#include "constraints/end_of_block_constraints.h"
#include "printer.h"

long long int n_gen = 0;
long long int n_gen_ok = 1;
int orario_len;
FILE *f;

typedef struct el {
    int teacher;
    struct el *prev;
    struct el *next;
} el, *elp;

typedef struct {
    elp head;
    elp tail;
} head_tail_queue;


int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

int *int_alloc(size_t len) {
    int *p = malloc(len * sizeof(int));
    if (p == NULL) {
        printf("Errore allocazione memoria\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

void print_orario(teaching_slot orario[], const int pos, const int tot, int ora_classi_docenti_materia_effettuate[],
                  int prof_in_classe[], int aule_occupate[], int classe_finisce_ultima_ora_num_giorni[]) {
    if (n_gen_ok <= n_gen) return;


    if (pos == tot) {
        print_orario_form(orario, pos, n_gen, f);
        if (verifica_ore_classe_materia(ora_classi_docenti_materia_effettuate)) {
            n_gen++;
            //printf("%lld\n", n_gen);
            print_orario_form(orario, tot, n_gen, f);
        }
        return;
    }

    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base3dprof_impegnati = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;
    const int base3daule_impegnate = giorno * ORA_GIORNALIERA_SZ * AULA_SZ + ora * AULA_SZ;

    if (activities_for_this_class_and_hour(orario, pos, ora, cla, classe_finisce_ultima_ora_num_giorni))
        for (int h = 0; h < actual_aula; h++) {
            const int ran = pos; //(int)(&ran);//rand();
            if (!aule_occupate[base3daule_impegnate + h])
                for (int mat = 0; mat < actual_materie; mat++) {
                    const int m = mat; //(mat + ran) % actual_materie;
                    int i = -1, j = 0;
                    const int base3dore_classi = cla * MATERIA_SZ * INSEGNANTE_SZ + m * INSEGNANTE_SZ;

                    //insegnanti da soli
                    for (; j < actual_insegnanti; j++) {
                        if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                            !prof_in_classe[base3dprof_impegnati + j]
                            //se un docente fa lezione solo in lab controlla che l'aula lo sia
                            && (!solo_in_lab[j] || laboratori[h])
                            // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                            && ora_classi_docenti_materia_effettuate[base3dore_classi + j] <
                            ore_insegnanti_classe_materia_lonely[base3dore_classi + j]
                            && 1) {
                            orario[pos] = (teaching_slot){h, m, i, j};
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
                    for (i = 0; i < actual_insegnanti; i++) {
                        if (!prof_in_classe[base3dprof_impegnati + i]
                            && ora_classi_docenti_materia_effettuate[base3dore_classi + i] <
                            ore_insegnanti_classe_materia[base3dore_classi + i])
                            for (j = i + 1; j < actual_insegnanti; j++) {
                                if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                                    !prof_in_classe[base3dprof_impegnati + j]
                                    && (!solo_in_lab[i] || laboratori[h])
                                    && (!solo_in_lab[j] || laboratori[h])
                                    // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                                    && ora_classi_docenti_materia_effettuate[base3dore_classi + j] <
                                    ore_insegnanti_classe_materia[base3dore_classi + j]
                                    && 1) {
                                    orario[pos] = (teaching_slot){h, m, i, j};
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

    if (ora >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = (teaching_slot){-1, -1, -1, -1};
        print_orario(orario, pos + 1, tot, ora_classi_docenti_materia_effettuate, prof_in_classe, aule_occupate,
                     classe_finisce_ultima_ora_num_giorni);
    }
}

int pop(int pos, elp *priority_queue, elp *tail) {
    assert(pos < actual_insegnanti);

    elp curr = *priority_queue;
    while (pos > 0)
        curr = curr->next, pos--;

    if (curr == *tail) return curr->teacher;

    if (curr == *priority_queue) {
        *priority_queue = (*priority_queue)->next;
    } else {
        curr->prev->next = curr->next;
    }
    curr->next->prev = curr->prev;
    curr->next = NULL;
    curr->prev = *tail;
    (*tail)->next = curr;
    *tail = curr;

    return curr->teacher;
}

int peek(int pos, elp priority_queue) {
    assert(pos < actual_insegnanti);
    assert(pos>=0);
    elp curr = priority_queue;
    while (pos > 0 && curr != NULL)
        curr = curr->next, pos--;
    return curr->teacher;
}

head_tail_queue initialize_teacher_queue(const int num, const int maxelems) {
    if (num < maxelems) {
        // S -> aS
        const elp curr = malloc(sizeof(el));
        // verify prev is NULL for first element
        curr->teacher = num;
        head_tail_queue ret = initialize_teacher_queue(num + 1, maxelems);
        if (ret.head != NULL)
            ret.head->prev = curr;
        curr->next = ret.head;
        return (head_tail_queue){curr, ret.tail == NULL ? curr : ret.tail};
    }

    if (num == maxelems) {
        // S -> eps
        return (head_tail_queue){NULL, NULL};
    }
    exit(EXIT_FAILURE);
}

head_tail_queue copy_list(const elp curr) {
    if (curr != NULL) {
        elp copy = malloc(sizeof(el));
        copy->teacher = curr->teacher;
        copy->prev = NULL;
        const head_tail_queue ret = copy_list(curr->next);
        if (ret.head != NULL)
            ret.head->prev = copy;
        copy->next = ret.head;
        return (head_tail_queue){copy, ret.tail == NULL ? curr : ret.tail};
    }

    if (curr == NULL) {
        return (head_tail_queue){NULL, NULL};
    }
}

void print_prioritt_queue(elp priority_queue) {
    for (elp i = priority_queue; i != NULL; i = i->next) {
        printf("%d ", i->teacher);
    }
}

head_tail_queue initialize_priority_queue() {
    const head_tail_queue ret = initialize_teacher_queue(0, actual_insegnanti);
    return (head_tail_queue){ret.head, ret.tail};
}

int main(void) {
    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    printf("Program started at: %s\n", ctime(&currentTime));
    inizializza_dati();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    giorno_ora_classi_sz = actual_classi_sezione;
    //test(100);

    //print_intestazioni(orario_len);
    teaching_slot orario[orario_len];
    //memset(orario, (elem){-1,-1,-1,-1}, orario_len*sizeof(int));
    int *ora_classi_docenti_materie_effettuate = int_alloc(CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ);
    memset(ora_classi_docenti_materie_effettuate, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ * sizeof(int));
    int *prof_in_classe = int_alloc(GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ);
    memset(prof_in_classe, 0, GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ*sizeof(int));
    int *aule_occupate = int_alloc(GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * AULA_SZ);
    memset(aule_occupate, 0, GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * AULA_SZ*sizeof(int));
    int *classe_finisce_tardi_num = int_alloc(CLASSE_SEZIONE_SZ);
    memset(classe_finisce_tardi_num, 0, CLASSE_SEZIONE_SZ*sizeof(int));
    int *ora_buca_classe = int_alloc(CLASSE_SEZIONE_SZ);
    memset(ora_buca_classe, 0, CLASSE_SEZIONE_SZ*sizeof(int));

    head_tail_queue ret = initialize_priority_queue();
    elp priority_queue = ret.head;
    elp tail = ret.tail;

    f = fopen("orario.txt", "w");
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len, ora_classi_docenti_materie_effettuate, prof_in_classe, aule_occupate,
                 classe_finisce_tardi_num, priority_queue, tail);
    fclose(f);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s.", n_gen, (int) elapsed / 60, (int) elapsed % 60);
    fflush(stdout);

    return 0;
}
