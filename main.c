#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define GIORNO_SETTIMANA_SZ 1
#define GIORNO_SETTIMANA_ITEM_SZ 4

#define ORA_GIORNALIERA_SZ 2
#define ORA_GIORNALIERA_ITEM_SZ 2

#define CLASSE_SEZIONE_SZ 2
#define CLASSE_SEZIONE_ITEM_SZ 3

#define AULA_SZ 2
#define AULA_ITEM_SZ 15

#define MATERIA_SZ 2
#define MATERIA_ITEM_SZ 15

#define INSEGNANTE_SZ 3
#define INSEGNANTE_ITEM_SZ 15

#define MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT 2
#define MAX_MATERIE_PER_INSEGNANTE 4
#define MAX_INSEGNANTI_PER_INSEGNANTE 4

/*
char giorni_settimana[GIORNO_SETTIMANA_SZ][GIORNO_SETTIMANA_ITEM_SZ] = {"Lun", "Mar", "Mer", "Gio", "Ven"};
char ora_giornaliera[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ] = {"08", "09", "10", "11", "12", "13"};
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ] = {"3H", "3F", "3E"};
char aule[AULA_SZ][AULA_ITEM_SZ] = {"P1", "P2"};
char materie[MATERIA_SZ][MATERIA_ITEM_SZ] = {"Sistemi e Reti", "Informatica"};
char insegnanti[INSEGNANTE_SZ][INSEGNANTE_ITEM_SZ] = {"Ricciardi", "Rizzi", "Negrioli"};
*/
struct param {
    unsigned int current_group;
    int is_active_group;
    unsigned int active_group;
    int sum_prof;
};

unsigned long long int n_gen = 0;
unsigned long long int n_gen_ok = 100;
unsigned int orario_len = GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * AULA_SZ * MATERIA_SZ *
                          INSEGNANTE_SZ;

//Associazioni
char insegnanti_insegnanti[INSEGNANTE_SZ][MAX_INSEGNANTI_PER_INSEGNANTE] = {};
/* for example if 1 works with 3,5 and 3 works with 1 and 4
 * then we have:
 * i_i[1] = [3,5,-1,-1]
 * i_i[3] = [1, 4, -1, -1]
 * */
char insegnante_materie[INSEGNANTE_SZ][MAX_MATERIE_PER_INSEGNANTE] = {};


int p_materia(char mat[MATERIA_ITEM_SZ]) {
    for (int i = 0; i < MATERIA_SZ; i++) {
        //        if (!strcmp(materie[i], mat))
        //            return i;
    }
    return -1;
}

int p_classe_sezione(char cla[CLASSE_SEZIONE_ITEM_SZ]) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ; i++) {
        //        if (!strcmp(classe_sezione[i], cla))
        //            return i;
    }
    return -1;
}

int g_materia(const char s[], int p, int mat) {
    return s[p] == '1' && p % MATERIA_SZ;
}

int g_classe(const char s[], const int p, const int cla) {
    return s[p] == '1'
           && p >= AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ
           && p < AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ + AULA_SZ * MATERIA_SZ;
}

int g_prof(const char s[], const int p, const int prof) {
    return s[p] == '1'
           && prof == p % INSEGNANTE_SZ;
}

int ore_materia_classe(const char s[], char mat[], char cla[]) {
    int materia_pos = p_materia(mat);
    //    if (materia_pos == -1)
    //        assert(0);
    int classe_pos = p_classe_sezione(cla);
    //    if (classe_pos == -1)
    //        assert(0);
    int sum = 0;

    for (int i = 0; s[i] != '\0'; i++) {
        if (g_materia(s, i, materia_pos)
            && g_classe(s, i, classe_pos))
            sum++;
    }
    return sum;
}

int verifica_orario_completo(const char *s) {
    int res = 1;
    res &= ore_materia_classe(s, "Informatica", "3H") == 0;
    //...
    return 1;
}

int verifica_orario_parziale(char *s) {
    // ad es. numero ore materia per classe è possibile verificarlo alla fine,
    // ma possiamo già escludere orari che superano tale valore
    return 1;
}

void reset_params(struct param *parameters, const unsigned int pos) {
    unsigned int p = pos % (AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ);
    // una classe in ogni aula, una materia ogni aula, un prof ogni materia
    // nuovo blocco
    if (p == 0) {
        parameters->is_active_group = 0;
        parameters->sum_prof = 0;
    }
    parameters->current_group = p / INSEGNANTE_SZ;
}

void set_params(struct param *parameters) {
    parameters->is_active_group = 1;
    parameters->active_group = parameters->current_group;

    parameters->sum_prof++;
}

int test_params(const struct param *parameters) {
    int res = 1;
    res &= !parameters->is_active_group || parameters->active_group == parameters->current_group;
    res &= parameters->sum_prof < MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT;
    return res;
}

void gen(char *s, unsigned int pos, unsigned int tot, struct param parameters) {
    if (pos == tot) {
        s[pos] = '\0';
        if (n_gen < n_gen_ok
            && verifica_orario_completo(s)) {
            n_gen++;
            printf("            %s\n", s);
        }
        return;
    }

    reset_params(&parameters, pos);

    // prova 0
    s[pos] = '0';
    gen(s, pos + 1, tot, parameters);

    // prova 1
    if (test_params(&parameters)) {
        s[pos] = '1';
        set_params(&parameters);
        gen(s, pos + 1, tot, parameters);
    }
}

void generate_header_rows(char title[], int elems, int space_n, char space_1[], char space_2[]) {
    if (space_n > 0) {
        memset(space_1, ' ', space_n/2);
        memset(space_2, ' ', space_n-space_n / 2-1);
        space_1[space_n / 2] = '\0';
        space_2[space_n - space_n / 2 - 1] = '\0';
    } else {
        space_1[0] = '\0';
        space_2[0] = '\0';
    }
    printf("%s", title);
    for (int i = 0; i < orario_len / elems / space_n;
         i++)
        for (int gs = 0; gs < elems; gs++)
            printf("%s%d%s", space_1, gs, space_2);
    printf("\n");
}

void print_intestazioni() {
    char space_1[orario_len + 1];
    char space_2[orario_len + 1];

    generate_header_rows("Giorno:     ", GIORNO_SETTIMANA_SZ,
                         ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * MATERIA_SZ * AULA_SZ * INSEGNANTE_SZ,
                         space_1, space_2);
    generate_header_rows("Ora:        ", ORA_GIORNALIERA_SZ, CLASSE_SEZIONE_SZ * MATERIA_SZ * AULA_SZ * INSEGNANTE_SZ,
                         space_1, space_2);
    generate_header_rows("Classe:     ", CLASSE_SEZIONE_SZ, AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ, space_1, space_2);
    generate_header_rows("Aula:       ", AULA_SZ, MATERIA_SZ * INSEGNANTE_SZ, space_1, space_2);
    generate_header_rows("Materia:    ", MATERIA_SZ, INSEGNANTE_SZ, space_1, space_2);
    generate_header_rows("Insegnante: ", INSEGNANTE_SZ, 1, space_1, space_2);
}

char *string_allocation(unsigned long max) {
    char *s = malloc(max + 1);
    if (s == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return s;
}

void test(int max) {
    struct timeval t1, t2;
    char *s = string_allocation(max + 1);

    for (int tot = 75; tot < max; tot++) {
        // Start timer
        gettimeofday(&t1, NULL);
        struct param parameters = {0, -1, 0};
        gen(s, 0, tot, parameters);

        // Stop timer
        gettimeofday(&t2, NULL);
        const unsigned long elapsedTime = t2.tv_sec - t1.tv_sec;
        printf("len: %d, %lu s.\n", tot, elapsedTime);
    }
    free(s);
}


int main(void) {
    memset(insegnante_materie, -1, INSEGNANTE_SZ*MAX_MATERIE_PER_INSEGNANTE);
    //test(100);
    print_intestazioni();
    char *s = string_allocation(orario_len + 1);
    struct param parameters = {0, -1, 0};
    gen(s, 0, orario_len, parameters);
    printf("Totale generati: %llu\n", n_gen);

    free(s);
    return 0;
}
