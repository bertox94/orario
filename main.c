#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define GIORNO_SETTIMANA_SZ 1
#define GIORNO_SETTIMANA_ITEM_SZ 4

#define ORA_GIORNALIERA_SZ 2
#define ORA_GIORNALIERA_ITEM_SZ 2

#define CLASSE_SEZIONE_SZ 1
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

char giorni_settimana[GIORNO_SETTIMANA_SZ][GIORNO_SETTIMANA_ITEM_SZ] = {"Lun"};
char ora_giornaliera[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ] = {"08", "09"};
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ] = {"3H"};
char aule[AULA_SZ][AULA_ITEM_SZ] = {"P1", "P2"};
char materie[MATERIA_SZ][MATERIA_ITEM_SZ] = {"Sistemi e Reti", "Informatica"};
char insegnanti[INSEGNANTE_SZ][INSEGNANTE_ITEM_SZ] = {"Ricciardi", "Rizzi", "Negrioli"};

struct param {
    int current_group;
    int active_group;
    int sum_prof;
};

unsigned long long int n_gen = 0;

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
        if (!strcmp(materie[i], mat))
            return i;
    }
    return -1;
}

int p_classe_sezione(char cla[CLASSE_SEZIONE_ITEM_SZ]) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ; i++)
        if (!strcmp(classe_sezione[i], cla))
            return i;
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
    if (materia_pos == -1)
        assert(0);
    int classe_pos = p_classe_sezione(cla);
    if (classe_pos == -1)
        assert(0);
    int sum = 0;

    for (int i = 0; s[i] != '\0'; i++) {
        if (g_materia(s, i, materia_pos)
            && g_classe(s, i, classe_pos))
            sum++;
    }
    return sum;
}

int verifica_orario_completo(char *s) {
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

int test_params(struct param *parameters) {
    int res = 1;
    res &= parameters->active_group == -1 || parameters->active_group == parameters->current_group;
    res &= parameters->sum_prof < MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT;
    return res;
}

void set_params(struct param *parameters, int pos) {
    int p = pos % (AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ); // una classe in ogni aula, una materia ogni aula
    // nuovo blocco
    if (p == 0) {
        parameters->active_group = -1;
        parameters->sum_prof = 0;
    }
    parameters->current_group = p / INSEGNANTE_SZ;
}

void update_params_after_1(struct param *parameters) {
    parameters->active_group = parameters->current_group;
    parameters->sum_prof++;
}

void gen(char *s, int pos, int tot, struct param parameters) {
    if (pos == tot) {
        s[pos] = '\0';
        if (verifica_orario_completo(s)) {
            int size_space = strlen("Insegnante: ");
            char str[size_space + 1];
            n_gen++;
            sprintf(str, "%lld", n_gen);
            const unsigned int size = strlen(str);
            str[size] = ')';
            for (unsigned int i = size + 1; i < size_space; i++) {
                str[i] = ' ';
            }
            str[size_space] = '\0';
            printf("%s%s\n", str, s);
        }

        return;
    }

    set_params(&parameters, pos);

    // prova 0
    s[pos] = '0';
    gen(s, pos + 1, tot, parameters);

    // prova 1
    if (test_params(&parameters)) {
        s[pos] = '1';
        update_params_after_1(&parameters);
        gen(s, pos + 1, tot, parameters);
    }
}

void pre_post_spaces(char pre[], char post[], int n) {
    memset(pre, ' ', n/2);
    memset(post, ' ', n-n / 2-1);
    pre[n / 2] = '\0';
    post[n - n / 2 - 1] = '\0';
}

void print_intestazioni() {
    int n = ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ;
    char space_g1[n / 2 + 1];
    char space_g2[n - n / 2];
    pre_post_spaces(space_g1, space_g2, n);
    printf("Giorno:     ");
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        printf("%s%d%s", space_g1, gs, space_g2);
    printf("\n");

    printf("Ora:        ");
    n = CLASSE_SEZIONE_SZ * AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ;
    char space_o1[n / 2 + 1];
    char space_o2[n - n / 2];
    pre_post_spaces(space_o1, space_o2, n);
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            printf("%s%d%s", space_o1, og, space_o2);
    printf("\n");

    printf("Classe:     ");
    n = AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ;
    char space_s1[n / 2 + 1];
    char space_s2[n - n / 2];
    pre_post_spaces(space_s1, space_s2, n);
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++)
                printf("%s%d%s", space_s1, cs, space_s2);
    printf("\n");

    printf("Aula:       ");
    n = MATERIA_SZ * INSEGNANTE_SZ;
    char space_a1[n / 2 + 1];
    char space_a2[n - n / 2];
    pre_post_spaces(space_a1, space_a2, n);
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++)
                for (int a = 0; a < AULA_SZ; a++)
                    printf("%s%d%s", space_a1, a, space_a2);
    printf("\n");

    printf("Materia:    ");
    char space_m1[INSEGNANTE_SZ / 2 + 1];
    char space_m2[INSEGNANTE_SZ - INSEGNANTE_SZ / 2];
    pre_post_spaces(space_m1, space_m2, INSEGNANTE_SZ);
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++)
                for (int a = 0; a < AULA_SZ; a++)
                    for (int m = 0; m < MATERIA_SZ; m++)
                        printf("%s%d%s", space_m1, m, space_m2);
    printf("\n");

    printf("Insegnante: ");
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++)
                for (int a = 0; a < AULA_SZ; a++)
                    for (int m = 0; m < MATERIA_SZ; m++)
                        for (int it = 0; it < INSEGNANTE_SZ; it++)
                            printf("%d", it);
    printf("\n");
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
    int tot = GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * AULA_SZ * MATERIA_SZ *
              INSEGNANTE_SZ;
    char *s = string_allocation(tot + 1);
    struct param parameters = {0, -1, 0};
    gen(s, 0, tot, parameters);

    free(s);
    return 0;
}
