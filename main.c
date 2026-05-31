#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GIORNO_SETTIMANA_SZ 5
#define GIORNO_SETTIMANA_ITEM_SZ 4

#define ORA_GIORNALIERA_SZ 3
#define ORA_GIORNALIERA_ITEM_SZ 2

#define CLASSE_SEZIONE_SZ 2
#define CLASSE_SEZIONE_ITEM_SZ 3

#define MATERIA_SZ 2
#define MATERIA_ITEM_SZ 15

#define INSEGNANTE_TITOLARE_SZ 2
#define INSEGNANTE_CODOC_SZ 3
#define INSEGNANTE_SOSTEGNO_SZ 1
#define INSEGNANTE_ITEM_SZ 15

#define AULA_SZ 4
#define AULA_ITEM_SZ 15

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

char giorni_settimana[GIORNO_SETTIMANA_SZ][GIORNO_SETTIMANA_ITEM_SZ] = {"Lun", "Mar", "Mer", "Gio", "Ven"};
char ora_giornaliera[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ] = {"08", "09", "10", "11", "12", "13"};
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ] = {"3H", "3E", "3F"};
char materia[MATERIA_SZ][MATERIA_ITEM_SZ] = {"Sistemi e Reti", "Informatica"};
char insegnante_titolare[INSEGNANTE_TITOLARE_SZ][INSEGNANTE_ITEM_SZ] = {"Ricciardi", "Tiraboschi"};
char insegnante_codoc[INSEGNANTE_CODOC_SZ][INSEGNANTE_ITEM_SZ] = {"Barosi", "Benedetto", "Rizzi"};
char insegnante_sostegno[INSEGNANTE_SOSTEGNO_SZ][INSEGNANTE_ITEM_SZ] = {"Negrioli"};
char aula[AULA_SZ][AULA_ITEM_SZ] = {"INFO0", "INFO1", "INFO2", "INFO3"};

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


int main(void) {
    char letters[] = {'A', 'B', 'C'};
    int m = 5;

    int idx[m];
    for (int i = 0; i < m; i++)
        idx[i] = 0;

    while (1) {
        /* print current word */
        for (int i = 0; i < m; i++)
            putchar(letters[idx[i]]);
        putchar('\n');

        /* increment the base-3 counter */
        int pos = m - 1;
        while (pos >= 0) {
            idx[pos]++;

            if (idx[pos] < 3)  // no carry
                break;

            idx[pos] = 0;      // carry
            pos--;
        }

        /* overflow: we've generated all words */
        if (pos < 0)
            break;
    }
    return 0;


   int attempt = 0;
    //giorno 0 ora 0:    albero Info 4A ps11; B Sistemi 3E ps12
    //giorno 0 ora 0:    albero Info 4A ps11; B Sistemi 3E ps11
    //giorno 0 ora 0:    albero Info 4A ps11; B Sistemi 3E ps10
    //giorno 0 ora 0:    albero Info 4A ps11; B Sistemi 3F ps12
    //giorno 0 ora 0:    albero Info 4A ps11; B Sistemi 3F ps12
    //giorno 0 ora 0:    albero Info 4A ps11; ...
    //giorno 0 ora 0:    albero Info 4A ps10; B Sistemi 3F ps12

    for (int cs = 0; cs < CLASSE_SEZIONE_SZ; cs++) {
        for (int m = 0; m < MATERIA_SZ; m++) {
            for (int it = 0; it < INSEGNANTE_TITOLARE_SZ; it++) {
                for (int ic = 0; ic < INSEGNANTE_CODOC_SZ; ic++) {
                    for (int is = 0; is < INSEGNANTE_SOSTEGNO_SZ; is++) {
                        for (int a = 0; a < AULA_SZ; a++) {
                            /*orario[attempt] = (struct cell){
                                gs, og,
                                cs, m, it,
                                ic, is, a
                            };
                            if (verifica()) {
                                print();
                            }*/
                            printf("Giorno:\t%d, Ora:\n", cs);
                            attempt++;
                        }
                    }
                }
            }
        }
    }

    verifica();

    return 0;
}
