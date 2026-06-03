#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define GIORNO_SETTIMANA_SZ 1
#define GIORNO_SETTIMANA_ITEM_SZ 4

#define ORA_GIORNALIERA_SZ 1
#define ORA_GIORNALIERA_ITEM_SZ 2

#define CLASSE_SEZIONE_SZ 3
#define CLASSE_SEZIONE_ITEM_SZ 3

#define AULA_SZ 3
#define AULA_ITEM_SZ 15

#define MATERIA_SZ 2
#define MATERIA_ITEM_SZ 15

#define INSEGNANTE_SZ 5
#define INSEGNANTE_ITEM_SZ 15

#define MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT 2
#define MAX_MATERIE_PER_INSEGNANTE 4
#define MAX_INSEGNANTI_PER_INSEGNANTE 4


char giorni_settimana[GIORNO_SETTIMANA_SZ][GIORNO_SETTIMANA_ITEM_SZ];
int actual_giorni_settimana;
char ora_giornaliera[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ];
int actual_ore_giornaliere;
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ];
int actual_classi_sezione;
char aule[AULA_SZ][AULA_ITEM_SZ];
int actual_aula;
int laboratori[AULA_SZ];
int actual_laboratori;
char materie[MATERIA_SZ][MATERIA_ITEM_SZ];
int actual_materie;
char insegnanti[INSEGNANTE_SZ][INSEGNANTE_ITEM_SZ];
int actual_insegnanti;
int codocenti[INSEGNANTE_SZ]; //non possono avere lezione da soli

struct param {
    unsigned int current_group;
    int is_active_group;
    unsigned int active_group;
    int sum_prof;

    // to be set with all zero at beginning
    int ore_materia_classe[CLASSE_SEZIONE_SZ][MATERIA_SZ];
};

unsigned long long int n_gen = 0;
unsigned long long int n_gen_ok = -1;
unsigned int orario_len = GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * AULA_SZ * MATERIA_SZ *
                          INSEGNANTE_SZ;

//Associazioni
char insegnanti_insegnanti[INSEGNANTE_SZ][INSEGNANTE_SZ];
/* for example if 1 works with 3,5 and 3 works with 1 and 4
 * then we have:
 * i_i[1] = [3,5,-1,-1]
 * i_i[3] = [1, 4, -1, -1]
 * */
char insegnante_materie[INSEGNANTE_SZ][MAX_MATERIE_PER_INSEGNANTE] = {};
int ore_materia_classe[CLASSE_SEZIONE_SZ][MATERIA_SZ] = {};


void symmetric_closure_insegnanti() {
    for (int i = 0; i < INSEGNANTE_SZ; i++) {
        for (int j = 0; j < INSEGNANTE_SZ; j++) {
            if (insegnanti_insegnanti[i][j])
                insegnanti_insegnanti[j][i] = insegnanti_insegnanti[i][j];
        }
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

int p_aula(char a[]) {
    for (int i = 0; i < actual_aula; i++) {
        if (!strcmp(aule[i], a))
            return i;
    }
    assert(0);
}

int p_materia(char mat[]) {
    for (int i = 0; i < actual_materie; i++) {
        if (!strcmp(materie[i], mat))
            return i;
    }
    assert(0);
}

int p_classe_sezione(char cla[]) {
    for (int i = 0; i < actual_classi_sezione; i++) {
        if (!strcmp(classe_sezione[i], cla))
            return i;
    }
    assert(0);
}

int p_insegnante(char in[]) {
    for (int i = 0; i < INSEGNANTE_SZ; i++) {
        if (!strcmp(insegnanti[i], in))
            return i;
    }
    assert(0);
}


unsigned int g_prof(const unsigned int p) {
    return p % INSEGNANTE_SZ;
}

unsigned int g_materia(const unsigned int p) {
    return (p / INSEGNANTE_SZ) % MATERIA_SZ;
}

unsigned int g_aula(const unsigned int p) {
    return (p / MATERIA_SZ / INSEGNANTE_SZ) % AULA_SZ;
}

unsigned int g_classe(const unsigned int p) {
    return (p / AULA_SZ / MATERIA_SZ / INSEGNANTE_SZ) % CLASSE_SEZIONE_SZ;
}


void set_insegnante_codocente(char in[], char co[]) {
    insegnanti_insegnanti[p_insegnante(in)][p_insegnante(co)] =
            insegnanti_insegnanti[p_insegnante(in)][p_insegnante(co)] = 1;
}

void inizializza_giorni() {
    int pos = 0;
    strcpy(giorni_settimana[pos++], "Lun");
    /*
    strcpy(giorni_settimana[pos++], "Mar");
    strcpy(giorni_settimana[pos++], "Mer");
    strcpy(giorni_settimana[pos++], "Gio");
    strcpy(giorni_settimana[pos++], "Ven");
    strcpy(giorni_settimana[pos++], "Sab");
    */
    actual_giorni_settimana = pos;
}

void inizializza_ore() {
    int pos = 0;
    strcpy(ora_giornaliera[pos++], "08");
    /*
   strcpy(ora_giornaliera[pos++], "09")
   strcpy(ora_giornaliera[pos++], "10");
   strcpy(ora_giornaliera[pos++], "11");
   strcpy(ora_giornaliera[pos++], "12");
   strcpy(ora_giornaliera[pos++], "13");
   */
    actual_ore_giornaliere = pos;
}

void inizializza_classi() {
    int pos = 0;
    strcpy(classe_sezione[pos++], "3H");
    strcpy(classe_sezione[pos++], "3E");
    strcpy(classe_sezione[pos++], "3F");
    actual_classi_sezione = pos;
}

void inizializza_aule() {
    int pos = 0;
    strcpy(aule[pos++], "PS16"); //0
    strcpy(aule[pos++], "INFO0"); //1
    strcpy(aule[pos++], "INFO1"); //2
    /*
       strcpy(aule[pos++], "INFO2");
       strcpy(aule[pos++], "INFO3");
       strcpy(aule[pos++], "PS15");
    */
    actual_aula = pos;
}

void inizializza_laboratori() {
    int pos = 0;
    laboratori[p_aula("INFO0")] = 1;
    laboratori[p_aula("INFO1")] = 1;
    /*
     strcpy(laboratori[pos++], "INFO2");
     strcpy(laboratori[pos++], "INFO3");
     */
    actual_laboratori = pos;
}

void inizializza_materie() {
    int pos = 0;
    strcpy(materie[pos++], "Sistemi e Reti");
    strcpy(materie[pos++], "Informatica");
    //strcpy(materie[pos++], "TPS");
    actual_materie = pos;
}

void inizializza_insegnanti() {
    int pos = 0;
    strcpy(insegnanti[pos++], "Ricciardi"); //1
    strcpy(insegnanti[pos++], "Tiraboschi"); //2
    strcpy(insegnanti[pos++], "Gatti"); //3
    strcpy(insegnanti[pos++], "Barosi"); //4
    strcpy(insegnanti[pos++], "Rizzi"); //5
    /*
       strcpy(insegnanti[pos++], "Gagliano");
       strcpy(insegnanti[pos++], "Paghera");
       strcpy(insegnanti[pos++], "Manganelli");
       strcpy(insegnanti[pos++], "Di Chio");
       strcpy(insegnanti[pos++], "Benedetto");
       strcpy(insegnanti[pos++], "Moretti");
       strcpy(insegnanti[pos++], "Masetti");
       strcpy(insegnanti[pos++], "Sciambarella");
       */
    actual_insegnanti = pos;
}

void initializza_codocenti() {
    memset(codocenti, 0, INSEGNANTE_SZ);
    codocenti[p_insegnante("Rizzi")] = 1;
    codocenti[p_insegnante("Barosi")] = 1;
    /*
     codocenti[p_insegnante("Gagliano")] = 1;
     codocenti[p_insegnante("Paghera")] = 1;
     codocenti[p_insegnante("Moretti")] = 1;
     codocenti[p_insegnante("Benedetto")] = 1;
     */
}

void inizializza_insegnanti_materie() {
    memset(insegnante_materie, 0, INSEGNANTE_SZ*MATERIA_SZ);
    insegnante_materie[p_insegnante("Ricciardi")][p_materia("Sistemi e Reti")] = 1;
    insegnante_materie[p_insegnante("Tiraboschi")][p_materia("Sistemi e Reti")] = 1;
    insegnante_materie[p_insegnante("Tiraboschi")][p_materia("Informatica")] = 1;
    insegnante_materie[p_insegnante("Gatti")][p_materia("Informatica")] = 1;
    insegnante_materie[p_insegnante("Barosi")][p_materia("Informatica")] = 1;
    insegnante_materie[p_insegnante("Rizzi")][p_materia("Sistemi e Reti")] = 1;
}

void inizializza_ore_materie_classe() {
    memset(ore_materia_classe, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ);
    ore_materia_classe[p_classe_sezione("3H")][p_materia("Sistemi e Reti")] = 2;
}

int verifica_ore_materia_classe(const unsigned int pos, const struct param *param) {
    for (int i = 0; i < actual_classi_sezione; i++) {
        for (int j = 0; j < actual_materie; j++) {
            if (ore_materia_classe[i][j] != param->ore_materia_classe[i][j])
                return 0;
        }
    }
    return 1;
}

int insegnante_materia_adeguata(const unsigned int pos) {
    return insegnante_materie[g_prof(pos)][g_materia(pos)];
}

int codocente_deve_essere_in_lab_no_aula(const unsigned int pos) {
    if (codocenti[g_prof(pos)])
        return laboratori[g_aula(pos)];
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

void set_params(const unsigned int pos, struct param *parameters) {
    parameters->is_active_group = 1;
    parameters->active_group = parameters->current_group;
    parameters->sum_prof++;

    //update the total hours of a subject for a class
    parameters->ore_materia_classe[g_classe(pos)][g_materia(pos)]++;
}

int can_place_one(unsigned int pos, const struct param *parameters) {
    /*Each day and hour, teachers can be only in a class, room and subject*/
    return (!parameters->is_active_group || parameters->active_group == parameters->current_group)
           /*In the same classroom and subject there can be at most MAX_PROF... at the same time*/
           && parameters->sum_prof < MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT
           // un insegnante può insegnare solo un range di materie
           && insegnante_materia_adeguata(pos)
           // il codocente può essere assegnato soltanto a un lab
           && codocente_deve_essere_in_lab_no_aula(pos)
           //le ore totali non hanno superato le ore classe e materia per la settimana
           && parameters->ore_materia_classe[g_classe(pos)][g_materia(pos)] <= ore_materia_classe[g_classe(pos)][
               g_materia(pos)];
}

// Do not pass s as parameter, the last part might form the previous calls.
// Draw the recursion tree to analyze the problem
void gen(char *s, const unsigned int pos, const unsigned int tot, struct param parameters) {
    if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        if (verifica_ore_materia_classe(pos, &parameters)) {
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
    if (can_place_one(pos, &parameters)) {
        s[pos] = '1';
        set_params(pos, &parameters);
        gen(s, pos + 1, tot, parameters);
    }
}


int main(void) {
    inizializza_giorni();
    inizializza_ore();
    inizializza_classi();
    inizializza_aule();
    inizializza_laboratori();
    inizializza_materie();
    inizializza_insegnanti();
    initializza_codocenti();
    inizializza_insegnanti_materie();
    inizializza_ore_materie_classe();

    //test(100);
    print_intestazioni();
    char *s = string_allocation(orario_len + 1);
    s[orario_len] = '\0';
    struct param parameters = {0, -1, 0};
    memset(parameters.ore_materia_classe, 0, actual_classi_sezione*actual_materie*sizeof(int));
    gen(s, 0, orario_len, parameters);
    printf("Totale generati: %llu\n", n_gen);
    fflush(stdout);

    free(s);
    return 0;
}
