#include <assert.h>
#include <iso646.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define GIORNO_SETTIMANA_SZ 6
#define GIORNO_SETTIMANA_ITEM_SZ 4

#define ORA_GIORNALIERA_SZ 6
#define ORA_GIORNALIERA_ITEM_SZ 3

#define CLASSE_SEZIONE_SZ 3
#define CLASSE_SEZIONE_ITEM_SZ 3

#define AULA_SZ 10
#define AULA_ITEM_SZ 15

#define MATERIA_SZ 2
#define MATERIA_ITEM_SZ 15

#define INSEGNANTE_SZ 5
#define INSEGNANTE_ITEM_SZ 15

#define MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT 2
#define MAX_MATERIE_PER_INSEGNANTE 4
#define MAX_INSEGNANTI_PER_INSEGNANTE 4

#define SET_PAIR(arr, pos, x, y) do { arr[pos][0] = (x); arr[pos][1] = (y); } while(0)


char giorni_settimana[GIORNO_SETTIMANA_SZ][GIORNO_SETTIMANA_ITEM_SZ];
char ora_giornaliera[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ];
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ];
char aule[AULA_SZ][AULA_ITEM_SZ];
char materie[MATERIA_SZ][MATERIA_ITEM_SZ];
char insegnanti[INSEGNANTE_SZ][INSEGNANTE_ITEM_SZ];
int laboratori[AULA_SZ];
int codocenti[INSEGNANTE_SZ]; //non possono avere lezione da soli
int giorno_libero_prof[INSEGNANTE_SZ];
int ore_settimana_prof[INSEGNANTE_SZ];

int actual_giorni_settimana;
int actual_ore_giornaliere;
int actual_classi_sezione;
int actual_aula;
int actual_laboratori;
int actual_materie;
int actual_insegnanti;

typedef struct {
    int aula;
    int materia;
    int insegnante1;
    int insegnante2;
} elem;

long long int n_gen = 0;
long long int n_gen_ok = 30;
int orario_len;
int giorno_ora_classi_sz;
//Associazioni
int ore_classe_materia[CLASSE_SEZIONE_SZ * MATERIA_SZ] = {};
int insegnante_classe_materia[CLASSE_SEZIONE_SZ * MATERIA_SZ] = {};

//GIORNI
char Lun[] = "Lun";
char Mar[] = "Mar";
char Mer[] = "Mer";
char Gio[] = "Gio";
char Ven[] = "Ven";
char Sab[] = "Sab";

//ORE
char Ora_08[] = "08";
char Ora_09[] = "09";
char Ora_10[] = "10";
char Ora_11[] = "11";
char Ora_12[] = "12";
char Ora_13[] = "13";

//CLASSI
char Classe_3H[] = "3H";
char Classe_3E[] = "3E";
char Classe_3F[] = "3F";

//AULE
char PS16[] = "PS16";
char INFO1[] = "INFO1";
char INFO0[] = "INFO0";

//MATERIE
char Informatica[] = "Informatica";
char Sistemi_e_Reti[] = "Sistemi e Reti";

//PROF
char Ricciardi[] = "Ricciardi";
char Tiraboschi[] = "Tiraboschi";
char Gatti[] = "Gatti";
char Barosi[] = "Barosi";
char Rizzi[] = "Rizzi";

void print_orario_form(const elem orario[], const int tot) {
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
            printf("  %d", orario[i].materia);
    }
    printf("\n");

    printf("Prof:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].insegnante1 == -1)
            printf("  -");
        else
            printf("  %d", orario[i].insegnante1);
    }
    printf("\n");
    printf("Prof:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].insegnante2 == -1)
            printf("  -");
        else
            printf("  %d", orario[i].insegnante2);
    }
    printf("\n            ");
    for (int i = 0; i < tot; i++) {
        printf("---");
    }
    printf("\n");
}

int p_aula(char a[]) {
    for (int i = 0; i < actual_aula; i++) {
        if (!strcmp(aule[i], a))
            return i;
    }
    assert(0);
    return -1;
}

int p_materia(char mat[]) {
    for (int i = 0; i < actual_materie; i++) {
        if (!strcmp(materie[i], mat))
            return i;
    }
    assert(0);
    return -1;
}

int p_classe_sezione(char cla[]) {
    for (int i = 0; i < actual_classi_sezione; i++) {
        if (!strcmp(classe_sezione[i], cla))
            return i;
    }
    assert(0);
    return -1;
}

int p_insegnante(char in[]) {
    for (int i = 0; i < actual_insegnanti; i++) {
        if (!strcmp(insegnanti[i], in))
            return i;
    }
    assert(0);
    return -1;
}

void set_giorno_settimana(char giorno[], const int pos) {
    assert(pos < GIORNO_SETTIMANA_SZ);
    assert(strlen(giorno) < GIORNO_SETTIMANA_ITEM_SZ);
    strcpy(giorni_settimana[pos], giorno);
}

void set_ora_giornaliera(char ora[], const int pos) {
    assert(pos < ORA_GIORNALIERA_SZ);
    assert(strlen(ora) < ORA_GIORNALIERA_ITEM_SZ);
    strcpy(ora_giornaliera[pos], ora);
}

void set_classe_sezione(char cla[], const int pos) {
    assert(pos < CLASSE_SEZIONE_SZ);
    assert(strlen(cla) < CLASSE_SEZIONE_ITEM_SZ);
    strcpy(classe_sezione[pos], cla);
}

void set_aula(char aul[], const int pos) {
    assert(pos < AULA_SZ);
    assert(strlen(aul) < AULA_ITEM_SZ);
    strcpy(aule[pos], aul);
}

void set_materia(char mat[], const int pos) {
    assert(pos < MATERIA_SZ);
    assert(strlen(mat) < MATERIA_ITEM_SZ);
    strcpy(materie[pos], mat);
}

void set_insegnante(char in[], const int pos) {
    assert(pos < INSEGNANTE_SZ);
    assert(strlen(in) < INSEGNANTE_ITEM_SZ);
    strcpy(insegnanti[pos], in);
}

void set_laboratorio(char lab[]) {
    laboratori[p_aula(lab)] = 1;
}

void set_codocente(char cod[]) {
    codocenti[p_insegnante(cod)] = 1;
}

void set_classe_materia_ore_prof(char cla[], char mat[], const int ore, const char prof[], const char codocente[]) {
    ore_classe_materia[p_classe_sezione(cla) * MATERIA_SZ + p_materia(mat)] = ore;
    insegnante_classe_materia[p_classe_sezione(cla) * MATERIA_SZ + p_materia(mat)] = p_insegnante(prof);
    //agigungi codocente
}

void print_intestazioni(const long len) {
    printf("Giorno:     ");
    for (int i = 0; i < len; i++)
        printf(" %c%c",
               giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][0],
               giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][1]);
    printf("\n");

    printf("Ora:        ");
    for (int i = 0; i < len; i++)
        printf(" %s", ora_giornaliera[i / actual_classi_sezione % actual_ore_giornaliere]);
    printf("\n");

    printf("Classe:     ");
    for (int i = 0; i < len; i++)
        printf(" %s", classe_sezione[i % actual_classi_sezione]);
    printf("\n");
}

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

int is_vuoto(const int p) {
    return p == -1;
}

int is_docente(const int p) {
    return !is_vuoto(p) && !codocenti[p];
}

int is_codocente(const int p) {
    return !is_vuoto(p) && codocenti[p];
}

int verifica_ore_classe_materia(const int ore_classe_per_materia[]) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ * MATERIA_SZ; i++)
        if (ore_classe_materia[i] != ore_classe_per_materia[i])
            return 0;
    return 1;
}

int verifica_ore_settimana_prof(const int ore_per_settimana_prof[]) {
    for (int i = 0; i < INSEGNANTE_SZ; i++)
        if (ore_per_settimana_prof[i] != ore_settimana_prof[i])
            return 0;
    return 1;
}

int prof_in_classe(const int prof1, const int prof2, int prof_in_classe[]) {
    assert(prof2 != -1);
    return (is_vuoto(prof1) || !prof_in_classe[prof1])
           && !prof_in_classe[prof2];
}

int insegnanti_materia_adeguata(const int mat, int prof1, int prof2) {
    assert(prof2 != -1);
    /*return (prof1 == -1 || insegnante_materie[prof1 * MATERIA_SZ + mat])
           && insegnante_materie[prof2 * MATERIA_SZ + mat];
           */
}


int codocente_deve_essere_in_lab_no_aula(const int aula, int prof1, int prof2) {
    assert(prof2 != -1);
    if (is_codocente(prof1) || is_codocente(prof2))
        return laboratori[aula];
    return 1;
}

int controllo_prof_insieme(const int prof1, const int prof2) {
    int n_docenti = is_docente(prof1) + is_docente(prof2);
    int n_codocenti = is_codocente(prof1) + is_codocente(prof2);
    // docente e codocente
    return (n_docenti == 1 && n_codocenti == 1)
           // docente solo
           || (n_docenti == 1 && n_codocenti == 0)
           //ora buca
           || (n_docenti == 0 && n_codocenti == 0);
}

void inizializza_dati() {
    int pos = 0;
    set_giorno_settimana("Lun", pos++);
    set_giorno_settimana("Mar", pos++);
    for (int i = pos; i < GIORNO_SETTIMANA_SZ; i++)
        set_giorno_settimana("", i);
    actual_giorni_settimana = pos;

    pos = 0;
    set_ora_giornaliera(Ora_08, pos++);
    set_ora_giornaliera(Ora_09, pos++);
    set_ora_giornaliera(Ora_10, pos++);
    set_ora_giornaliera(Ora_11, pos++);
    set_ora_giornaliera(Ora_12, pos++);
    set_ora_giornaliera(Ora_13, pos++);
    for (int i = pos; i < ORA_GIORNALIERA_SZ; i++)
        set_ora_giornaliera("", i);
    actual_ore_giornaliere = pos;

    pos = 0;
    set_classe_sezione(Classe_3H, pos++);
    set_classe_sezione(Classe_3E, pos++);
    //set_classe_sezione("3F", pos++);
    for (int i = pos; i < CLASSE_SEZIONE_SZ; i++)
        set_classe_sezione("", i);
    actual_classi_sezione = pos;

    pos = 0;
    set_aula("PS16", pos++);
    set_aula("INFO1", pos++);
    //set_aula("INFO0", pos++);
    for (int i = pos; i < AULA_SZ; i++)
        set_aula("", i);
    actual_aula = pos;

    pos = 0;
    set_materia("Informatica", pos++);
    set_materia("Sistemi e Reti", pos++);
    for (int i = pos; i < MATERIA_SZ; i++)
        set_materia("", i);
    actual_materie = pos;

    pos = 0;
    set_insegnante("Ricciardi", pos++);
    set_insegnante("Tiraboschi", pos++);
    set_insegnante("Gatti", pos++);
    set_insegnante("Barosi", pos++);
    set_insegnante("Rizzi", pos++);
    for (int i = pos; i < INSEGNANTE_SZ; i++)
        set_insegnante("", i);
    actual_insegnanti = pos;
}

void inizializza_vincoli() {
    memset(laboratori, 0, AULA_SZ*sizeof(int));
    //set_laboratorio("INFO0");
    set_laboratorio("INFO1");

    memset(codocenti, 0, INSEGNANTE_SZ*sizeof(int));
    set_codocente("Rizzi");
    set_codocente("Barosi");

    memset(insegnante_materie, 0, INSEGNANTE_SZ*MATERIA_SZ*sizeof(int));
    set_insegnante_materia("Ricciardi", "Sistemi e Reti");
    set_insegnante_materia("Tiraboschi", "Sistemi e Reti");
    set_insegnante_materia("Tiraboschi", "Informatica");
    set_insegnante_materia("Gatti", "Informatica");
    set_insegnante_materia("Barosi", "Informatica");
    set_insegnante_materia("Rizzi", "Sistemi e Reti");

    memset(ore_classe_materia, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
    set_classe_materia_ore_prof("3H", "Sistemi e Reti", 2, Ricciardi);
    set_classe_materia_ore_prof("3H", "Informatica", 1, Gatti);
    set_classe_materia_ore_prof("3E", "Sistemi e Reti", 2, Ricciardi);
    set_classe_materia_ore_prof("3E", "Informatica", 1, Gatti);

    memset(ore_settimana_prof, 0, INSEGNANTE_SZ*sizeof(int));
    ore_settimana_prof[p_insegnante("Ricciardi")] = 2;
    ore_settimana_prof[p_insegnante("Gatti")] = 2;
    ore_settimana_prof[p_insegnante("Tiraboschi")] = 2;
    ore_settimana_prof[p_insegnante("Barosi")] = 1;

    //vincolo classi materia insegnanti
}

int controlli_finali(const int ore_classe_materia_curr[], const int ore_per_settimana_prof[]) {
    return verifica_ore_classe_materia(ore_classe_materia_curr)
           && verifica_ore_settimana_prof(ore_per_settimana_prof)
           && 1;
}

void print_orario(elem orario[], const int pos, const int tot, int prof_gia_in_classe_curr[],
                  int ore_classe_materia_curr[],
                  const int ore_per_settimana_prof_curr[]) {
    if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        if (controlli_finali(ore_classe_materia_curr, ore_per_settimana_prof_curr)) {
            n_gen++;
            print_orario_form(orario, tot);
        }
        return;
    }

    pos % giorno_ora_classi_sz != 0 ? : memset(prof_gia_in_classe_curr, 0, INSEGNANTE_SZ*sizeof(int));

    orario[pos] = (elem){-1, -1, -1, -1};
    print_orario(orario, pos + 1, tot, prof_gia_in_classe_curr, ore_classe_materia_curr, ore_per_settimana_prof_curr);

    for (int h = 0; h < actual_aula; h++) {
        for (int m = 0; m < actual_materie; m++) {
            //ci può essere un solo insegnante perciò parte da -1
            for (int i = -1; i < actual_insegnanti; i++) {
                for (int j = i + 1; j < actual_insegnanti; j++) {
                    if (prof_in_classe(i, j, prof_gia_in_classe_curr)
                        && insegnanti_materia_adeguata(m, i, j)
                        && codocente_deve_essere_in_lab_no_aula(h, i, j)
                        && controllo_prof_insieme(i, j)
                        && ore_classe_materia_curr[g_classe(pos) * MATERIA_SZ + m] <
                        ore_classe_materia[g_classe(pos) * MATERIA_SZ + m]
                        && ore_per_settimana_prof_curr[i] < ore_settimana_prof[i]
                        && 1) {
                        orario[pos] = (elem){h, m, i, j};

                        int ore_classe_materia_new[CLASSE_SEZIONE_SZ * MATERIA_SZ];
                        int ore_per_settimana_prof_new[INSEGNANTE_SZ];
                        int prof_gia_in_classe_new[INSEGNANTE_SZ];
                        memcpy(ore_classe_materia_new, ore_classe_materia_curr,
                               CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
                        memcpy(ore_per_settimana_prof_new, ore_per_settimana_prof_curr, INSEGNANTE_SZ*sizeof(int));
                        memcpy(prof_gia_in_classe_new, prof_gia_in_classe_curr, INSEGNANTE_SZ*sizeof(int));

                        if (!is_vuoto(i)) {
                            ore_per_settimana_prof_new[i]++;
                            prof_gia_in_classe_new[i] = 1;
                        }
                        ore_classe_materia_new[g_classe(pos) * MATERIA_SZ + m]++;
                        ore_per_settimana_prof_new[j]++;
                        prof_gia_in_classe_new[j] = 1;

                        print_orario(orario, pos + 1, tot, prof_gia_in_classe_new, ore_classe_materia_new,
                                     ore_per_settimana_prof_new);
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
    int ore_materie_per_classe[CLASSE_SEZIONE_SZ * MATERIA_SZ];
    int ore_per_settimana_prof[INSEGNANTE_SZ];
    int prof_gia_in_classe[INSEGNANTE_SZ];
    memset(orario, 0, orario_len*sizeof(int));
    memset(ore_materie_per_classe, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
    memset(ore_per_settimana_prof, 0, INSEGNANTE_SZ*sizeof(int));
    memset(prof_gia_in_classe, 0, INSEGNANTE_SZ*sizeof(int));
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(orario, 0, orario_len, prof_gia_in_classe, ore_materie_per_classe, ore_per_settimana_prof);

    printf("Totale generati: %llu\n", n_gen);
    fflush(stdout);

    return 0;
}
