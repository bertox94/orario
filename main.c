#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GIORNO_SETTIMANA_SZ 2
#define GIORNO_SETTIMANA_ITEM_SZ 4

#define ORA_GIORNALIERA_SZ 3
#define ORA_GIORNALIERA_ITEM_SZ 2

#define CLASSE_SEZIONE_SZ 3
#define CLASSE_SEZIONE_ITEM_SZ 3

#define AULA_SZ 3
#define AULA_ITEM_SZ 15

#define MATERIA_SZ 3
#define MATERIA_ITEM_SZ 15

#define INSEGNANTE_SZ 3
#define INSEGNANTE_ITEM_SZ 15


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
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ] = {"3H", "3E", "3F"};
char aula[AULA_SZ][AULA_ITEM_SZ] = {"P1", "P2", "L2"};
char materia[MATERIA_SZ][MATERIA_ITEM_SZ] = {"Sistemi e Reti", "Informatica", "Italiano"};
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

void print() {
    printf("Found one! --------------------\n");
    for (int i = 0; i < 36; i++) {
        printf("%d) %d %d %d %d %d %d \n", i, orario[i].classe_sezione,
               orario[i].materia, orario[i].insegnante_titolare,
               orario[i].insegnante_codoc, orario[i].insegnante_sostegno,
               orario[i].aula);
    }
}

void gen(char *s, int pos, int tot, int active_group) {
    if (pos == tot) {
        s[pos] = '\0';
        printf("%s\n", s);
        return;
    }

    int p = pos % (AULA_SZ * MATERIA_SZ * INSEGNANTE_SZ); // una classe in ogni aula

    // nuovo blocco da 27
    if (p == 0)
        active_group = -1;

    int group = p / (MATERIA_SZ * INSEGNANTE_SZ); // 0, 1, 2

    // prova 0
    s[pos] = '0';
    gen(s, pos + 1, tot, active_group);

    // prova 1
    if (active_group == -1 || active_group == group) {
        s[pos] = '1';
        gen(s, pos + 1, tot, group);
    }
}

void print_intestazioni() {
        printf("Giorno:     ");
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        printf(
            "                                                                                                                         %d"
            "                                                                                                                         ",
            gs);
    printf("\n");

    printf("Ora:\t\t");
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            printf("                                        %d                                        ", og);
    printf("\n");

    printf("Classe:\t\t");
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++)
                printf("             %d             ", cs);
    printf("\n");

    printf("Aula:\t\t");
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++)
                for (int a = 0; a < AULA_SZ; a++)
                    printf("    %d    ", a);
    printf("\n");

    printf("Materia:    ");
    char space_materia[]
    for (int gs = 0; gs < GIORNO_SETTIMANA_SZ; gs++)
        for (int og = 0; og < ORA_GIORNALIERA_SZ; og++)
            for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++)
                for (int a = 0; a < AULA_SZ; a++)
                    for (int m = 0; m < MATERIA_SZ; m++)
                        printf(" %d ", m);
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

int main(void) {


    int tot = GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * AULA_SZ * MATERIA_SZ *
              INSEGNANTE_SZ;
    tot = 28;
    //printf("String length: %u\n", tot);

    char *s = malloc(tot + 1);
    if (s == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    gen(s, 0, tot, -1);

    free(s);
    return 0;
}
