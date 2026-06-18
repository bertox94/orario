#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "data_initialization/database_initializers.h"

#define MAX_STR_SZ_IN_DISPLAY 18

long long int n_gen = 0;
long long int n_gen_ok = 99990000;
int orario_len;
int orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
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

void copy_and_pad(char *dest, const char *src, int maxlen) {
    int src_len = strlen(src);
    int copy_len = src_len;

    // 1. Determine how much to copy (leave 1 slot for the null terminator)
    if (copy_len > maxlen - 1) {
        copy_len = maxlen - 1;
    }

    // 2. Copy the actual string characters
    memcpy(dest, src, copy_len);

    // 3. Fill the remaining space with blank spaces ' '
    for (int i = copy_len; i < maxlen - 1; i++) {
        dest[i] = ' ';
    }

    // 4. Always force a null terminator at the very end
    dest[maxlen - 1] = '\0';
}

void draw_horizontal_separator(int pos) {
    printf("---+");
    for (int i = 0; i < actual_giorni_settimana; i++) {
        printf("---------------------------+");
    }
    printf("\n");
}

// Helper 1: Prints the header block containing the current time and attempt number
void print_header(unsigned long long attempt) {
    time_t current_time;
    time(&current_time);

    printf("Current time: %s", ctime(&current_time));
    printf("Orario n. %llu\n", attempt);
}

// Helper 2: Prints the days of the week header row for a specific class
void print_days_header(const int pos, int class_index) {
    printf("\nClasse: %s, ore: %d\n", classe_sezione[class_index], ore_classi_settimanali[class_index]);
    printf("   |");

    for (int i = 0; i < pos; i += actual_ore_giornaliere * actual_classi_sezione) {
        printf("            %s            |", giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione]);
    }
    printf("\n");
}

void print_row_prof(char ora[], int c, int o) {
    printf(" %s|", ora);
    char formatted_str[MAX_STR_SZ_IN_DISPLAY];
    for (int g = 0; g < actual_giorni_settimana; g++) {
        const int curr = orario[
            g * actual_ore_giornaliere * actual_classi_sezione + o * actual_classi_sezione + c];
        copy_and_pad(formatted_str, curr == -1 ? "       --        " : insegnanti[curr],
                     MAX_STR_SZ_IN_DISPLAY);
        printf("     %s     |", formatted_str);
    }
    printf("\n");
}

// Main Function: Clean, descriptive, and easy to read
void print(const int orario[], const int pos, unsigned long long attempt) {
    print_header(attempt);

    for (int c = 0; c < actual_classi_sezione; c++) {
        print_days_header(pos, c);
        draw_horizontal_separator(pos);

        for (int o = 0; o < actual_ore_giornaliere; o++) {
            // Print the row label (the specific hour slot)

            char formatted_str[MAX_STR_SZ_IN_DISPLAY];
            print_row_prof(ore_giornaliere[o], c, o);

            draw_horizontal_separator(pos);
        }
    }
}

/*Progressive contraints*/
int activities_for_this_class_and_hour(int orario[], int pos, const int ora, const int cla,
                                       const int classe_finisce_tardi_num[]) {
    //se ti trovi ad avere già n classe con orari pieni
    const int giorni_pieni = (ore_classi_settimanali[cla] - PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana) %
                             actual_ore_giornaliere;

    //l'ultima ora non è permessa se è di un giorno che supera i giorni pieni, le altre prima possono,
    //quelle fino a obbligatorie devono
    if (ora == actual_ore_giornaliere - 1 && classe_finisce_tardi_num[cla] >= giorni_pieni)
        return 0;


    //sostituire con parametro senza usare orario
    if (pos - actual_classi_sezione >= 0) {
        if (pos / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana == (
                pos - actual_classi_sezione) / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana)
            return orario[pos - actual_classi_sezione] != -1;
        //|| orario[pos - actual_classi_sezione].insegnante2 != -1;
    }

    return 1;
}

/*End of block constraints*/
int verifica_ore_classe_materia(int ora_classi_materie_effettuate[]) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ * INSEGNANTE_SZ; i++)
        if (ore_insegnante_classi[i] != ora_classi_materie_effettuate[i]) {
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
    curr->next->prev = curr->prev; // OK also if curr was the head, it just copies NULL
    curr->next = NULL;
    curr->prev = *tail;
    (*tail)->next = curr;
    *tail = curr;

    return curr->teacher;
}

int peek(elp priority_queue, int pos) {
    assert(pos < actual_insegnanti);
    assert(pos>=0);
    elp curr = priority_queue;
    while (pos > 0 && curr != NULL)
        curr = curr->next, pos--;
    return curr->teacher;
}

head_tail_queue initialize_queue(const int num, const int maxelems) {
    if (num < maxelems) {
        // S -> aS
        const elp curr = malloc(sizeof(el));
        curr->prev = NULL;
        curr->teacher = num;
        head_tail_queue ret = initialize_queue(num + 1, maxelems);
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
        copy->prev = NULL;
        copy->teacher = curr->teacher;
        const head_tail_queue ret = copy_list(curr->next);
        if (ret.head != NULL)
            ret.head->prev = copy;
        copy->next = ret.head;
        return (head_tail_queue){copy, ret.tail == NULL ? copy : ret.tail};
    }

    if (curr == NULL) {
        return (head_tail_queue){NULL, NULL};
    }
    exit(EXIT_FAILURE);
}

void print_prioritt_queue(elp priority_queue) {
    for (elp i = priority_queue; i != NULL; i = i->next) {
        printf("%p: %d\n", i, i->teacher);
    }
    printf("------------\n");
}

head_tail_queue initialize_priority_queue(const int len) {
    const head_tail_queue ret = initialize_queue(0, len);
    return (head_tail_queue){ret.head, ret.tail};
}

void print_orario(int orario[], const int pos, const int tot, int ora_classi_docenti_effettuate[],
                  int prof_in_classe[], int classe_finisce_ultima_ora_num_giorni[]) {
    if (n_gen_ok <= n_gen) return;


    if (pos == tot) {
        if (verifica_ore_classe_materia(ora_classi_docenti_effettuate)) {
            n_gen++;
            //printf("%lld\n", n_gen);
            print(orario, pos, n_gen);
        }
        return;
    }

    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base3dprof_impegnati = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;

    if (activities_for_this_class_and_hour(orario, pos, ora, cla, classe_finisce_ultima_ora_num_giorni)) {
        int j = 0;


        for (; j < actual_insegnanti; j++) {
            const int base3dore_classi = cla * INSEGNANTE_SZ + j;
            if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                !prof_in_classe[base3dprof_impegnati + j]
                // controlla ore svolte per materia, verifica se non deve svolgerne altre da solo
                && ora_classi_docenti_effettuate[base3dore_classi + j] <
                ore_insegnante_classi[base3dore_classi + j]
                && 1) {
                ora_classi_docenti_effettuate[base3dore_classi + j]++;
                prof_in_classe[base3dprof_impegnati + j] = 1;
                if (ora == actual_ore_giornaliere - 1)
                    classe_finisce_ultima_ora_num_giorni[cla]++;

                orario[pos] = j;
                print_orario(orario, pos + 1, tot, ora_classi_docenti_effettuate, prof_in_classe,
                             classe_finisce_ultima_ora_num_giorni);

                ora_classi_docenti_effettuate[base3dore_classi + j]--;
                prof_in_classe[base3dprof_impegnati + j] = 0;
                if (ora == actual_ore_giornaliere - 1)
                    classe_finisce_ultima_ora_num_giorni[cla]--;
            }
        }
    }


    if (ora >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos] = -1;
        print_orario(orario, pos + 1, tot, ora_classi_docenti_effettuate, prof_in_classe,
                     classe_finisce_ultima_ora_num_giorni);
    }
}

int main(void) {
    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    printf("Program started at: %s\n", ctime(&currentTime));
    inizializza_dati();
    stampa_dati();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    giorno_ora_classi_sz = actual_classi_sezione;
    //test(100);

    //print_intestazioni(orario_len);
    for (int i = 0; i < orario_len; i++) {
        orario[i] = -1;
    }
    //memset(orario, (elem){-1,-1,-1,-1}, orario_len*sizeof(int));
    int *ora_classi_docenti_effettuate = int_alloc(CLASSE_SEZIONE_SZ * INSEGNANTE_SZ);
    memset(ora_classi_docenti_effettuate, 0, CLASSE_SEZIONE_SZ*INSEGNANTE_SZ * sizeof(int));
    int *prof_in_classe = int_alloc(GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ);
    memset(prof_in_classe, 0, GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ*sizeof(int));
    int *classe_finisce_tardi_num = int_alloc(CLASSE_SEZIONE_SZ);
    memset(classe_finisce_tardi_num, 0, CLASSE_SEZIONE_SZ*sizeof(int));

    head_tail_queue ret = initialize_priority_queue(actual_insegnanti);
    elp priority_queue = ret.head;
    elp tail = ret.tail;

    f = fopen("orario.txt", "w");
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len, ora_classi_docenti_effettuate, prof_in_classe, classe_finisce_tardi_num);
    fclose(f);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s.", n_gen, (int) elapsed / 60, (int) elapsed % 60);
    fflush(stdout);

    return 0;
}
