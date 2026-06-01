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

struct cell {
    //int giorno_settimana;
    //int ora_giornaliera;
    int classe_sezione;
    int materia;
    int insegnante_titolare;
    int insegnante_codoc;
    int insegnante_sostegno;
    int aula;
};

char giorni_settimana[GIORNO_SETTIMANA_SZ][GIORNO_SETTIMANA_ITEM_SZ] = {"Lun"};
char ora_giornaliera[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ] = {"08", "09"};
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ] = {"3H"};
char aula[AULA_SZ][AULA_ITEM_SZ] = {"P1", "P2"};
char materia[MATERIA_SZ][MATERIA_ITEM_SZ] = {"Sistemi e Reti", "Informatica"};
char insegnante_titolare[INSEGNANTE_SZ][INSEGNANTE_ITEM_SZ] = {"Ricciardi", "Rizzi", "Negrioli"};

struct cell orario[36];

int p_materia(char mat[MATERIA_ITEM_SZ]) {
    for (int i = 0; i < MATERIA_SZ; i++) {
        if (!strcmp(materia[i], mat))
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

unsigned int ore_materia_classe(char mat[], char cla[]) {
    int materia_pos = p_materia(mat);
    if (materia_pos == -1)
        assert(0);
    int classe_pos = p_classe_sezione(cla);
    if (classe_pos == -1)
        assert(0);
    unsigned int sum = 0;
    for (int i = 0; i < 36; i++) {
        if (orario[i].materia == materia_pos
            && orario[i].classe_sezione == classe_pos)
            sum++;
    }
    return sum;
}

int verifica() {
    int res = 1;
    res &= ore_materia_classe("Informatica", "3H") == 1;
    //...
    return res;
}

void gen(char *s, int pos, int tot, int active_group, int sum_prof) {
    if (pos == tot) {
        s[pos] = '\0';
        if (verifica(s))
            printf("            %s\n", s);
        return;
    }

    int p = pos % (AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ); // una classe in ogni aula, una materia ogni aula
    // nuovo blocco da 27
    if (p == 0)
        active_group = -1;
    int group = p / INSEGNANTE_SZ;

    if (pos % INSEGNANTE_SZ == 0)
        sum_prof = 0;


    // prova 0
    s[pos] = '0';
    gen(s, pos + 1, tot, active_group, sum_prof);

    // prova 1
    if ((active_group == -1 || active_group == group)
        && sum_prof < MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT) {
        s[pos] = '1';
        sum_prof++;
        gen(s, pos + 1, tot, group, sum_prof);
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


void test(int max) {
    struct timeval t1, t2;
    double elapsedTime;

    int tot = 70;

    char *s = malloc(max + 1);
    if (s == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (; tot < max; tot++) {
        // Start timer
        gettimeofday(&t1, NULL);

        gen(s, 0, tot, -1, 0);

        // Stop timer
        gettimeofday(&t2, NULL);

        // Compute and print the elapsed time in milliseconds:

        elapsedTime = t2.tv_sec - t1.tv_sec;

        printf("len: %d, %.2f s.\n", tot, elapsedTime);
    }
}


int main(void) {
    print_intestazioni();

    int tot = GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * AULA_SZ * MATERIA_SZ *
              INSEGNANTE_SZ;
    //printf("String length: %u\n", tot);

    char *s = malloc(tot + 1);
    if (s == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    gen(s, 0, tot, -1, 0);

    free(s);
    return 0;
}
