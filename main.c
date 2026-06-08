#include <assert.h>
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

long long int n_gen = 0;
long long int n_gen_ok = 30000000;
int orario_len;
int giorno_ora_classe_sz;
//Associazioni
int insegnante_materie[INSEGNANTE_SZ * MAX_MATERIE_PER_INSEGNANTE] = {};
int ore_classe_materia[CLASSE_SEZIONE_SZ * MATERIA_SZ] = {};

void print_orario_form(const int (*orario)[MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT], const int tot) {
    printf("            ");
    for (int i = 0; i < tot; i++) {
        if (orario[i][0] == -1)
            printf("  -");
        else
            printf("  %d", orario[i][0]);
    }
    printf("\n");
    printf("            ");
    for (int i = 0; i < tot; i++) {
        if (orario[i][1] == -1)
            printf("  -");
        else
            printf("  %d", orario[i][1]);
    }
    printf("\n              ");
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

void set_insegnante_materia(char in[], char mat[]) {
    insegnante_materie[p_insegnante(in) * MATERIA_SZ + p_materia(mat)] = 1;
}

void set_ore_classe_materia(char cla[], char mat[], int ore) {
    ore_classe_materia[p_classe_sezione(cla) * MATERIA_SZ + p_materia(mat)] = ore;
}

void print_intestazioni(const long len) {
    printf("Giorno:     ");
    for (int i = 0; i < len; i++)
        printf("  %d",
               i / actual_ore_giornaliere / actual_classi_sezione / actual_aula / actual_materie %
               actual_giorni_settimana);
    printf("\n");

    printf("Ora:        ");
    for (int i = 0; i < len; i++)
        printf("  %d", i / actual_classi_sezione / actual_aula / actual_materie % actual_ore_giornaliere);
    printf("\n");


    printf("Classe:     ");
    for (int i = 0; i < len; i++)
        printf("  %d", i / actual_aula / actual_materie % actual_classi_sezione);
    printf("\n");

    printf("Aula:       ");
    for (int i = 0; i < len; i++)
        printf("  %d", i / actual_materie % actual_aula);
    printf("\n");

    printf("Materia:    ");
    for (int i = 0; i < len; i++)
        printf("  %d", i % actual_materie);
    printf("\n");
}

int g_materia(const int p) {
    return p % actual_materie;
}

int g_aula(const int p) {
    return p / actual_materie % actual_aula;
}

int g_classe(const int p) {
    return p / actual_aula / actual_materie % actual_classi_sezione;
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

void inizializza_ore_docenti() {
    for (int i = 0; i < INSEGNANTE_SZ; i++) {
        if (strcmp(insegnanti[i], "") != 0)
            ore_settimana_prof[i] = 2;
    }
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

void reset_params(const int pos, int *has_active_group, int *current_group) {
    if (pos % giorno_ora_classe_sz == 0)
        *has_active_group = 0;

    *current_group = pos;
}

int insegnante_materia_adeguata(const int pos, int prof) {
    return prof == -1 || insegnante_materie[prof * MATERIA_SZ + g_materia(pos)];
}

int codocente_deve_essere_in_lab_no_aula(const int pos, int prof) {
    return !codocenti[prof] || laboratori[g_aula(pos)];
}

int se_ce_docente_allora_solo_o_codocente(const int pos, const int prof, const int last_prof) {
    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0) return 1;
    return !is_docente(last_prof) || is_codocente(prof) || is_vuoto(prof);
}

int se_ce_codocente_allora_docente(const int pos, const int prof, const int last_prof) {
    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0) return 1;
    return !is_codocente(last_prof) || is_docente(prof);
}

int se_vuoto_allora_vuoto_o_docente(const int pos, const int prof, const int last_prof) {
    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0) return 1;
    return !is_vuoto(last_prof) || is_vuoto(prof) || is_docente(prof);
}

int controllo_prof_insieme(const int orario[], const int pos) {
    int n_docenti = 0, n_codocenti = 0;
    if (pos == 0) return 1;
    for (int i = pos - 2; i < pos; i++) {
        if (orario[i] == -1) continue;
        codocenti[orario[i]] ? n_codocenti++ : n_docenti++;
    }
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
    for (int i = pos; i < GIORNO_SETTIMANA_SZ; i++)
        set_giorno_settimana("", i);
    actual_giorni_settimana = pos;

    pos = 0;
    set_ora_giornaliera("08", pos++);
    set_giorno_settimana("09", pos++);
    //set_giorno_settimana("10", pos++);
    //set_giorno_settimana("11", pos++);
    //set_giorno_settimana("12", pos++);
    //set_giorno_settimana("13", pos++);
    for (int i = pos; i < ORA_GIORNALIERA_SZ; i++)
        set_ora_giornaliera("", i);
    actual_ore_giornaliere = pos;

    pos = 0;
    set_classe_sezione("3H", pos++);
    //set_classe_sezione("3E", pos++);
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
    set_ore_classe_materia("3H", "Sistemi e Reti", 1);
    set_ore_classe_materia("3H", "Informatica", 1);

    memset(ore_settimana_prof, 0, INSEGNANTE_SZ*sizeof(int));
    ore_settimana_prof[p_insegnante("Ricciardi")] = 1;
    ore_settimana_prof[p_insegnante("Tiraboschi")] = 1;
    ore_settimana_prof[p_insegnante("Barosi")] = 1;
}

int controlli_finali(const int ore_classe_materia_curr[], const int ore_per_settimana_prof[]) {
    return verifica_ore_classe_materia(ore_classe_materia_curr)
           && verifica_ore_settimana_prof(ore_per_settimana_prof)
           && 1;
}

int can_place(const int pos, const int prof, const int last_prof, const int ore_classe_per_materia[],
              const int ore_per_settimana_prof[]) {
    return //insegnante insegna la propria materia
            insegnante_materia_adeguata(pos, prof)
            // il codocente può essere assegnato soltanto a un lab
            && codocente_deve_essere_in_lab_no_aula(pos, prof)
            //se è già presente un docente nel gruppo corrente allora può essere solo o con un codocente
            && se_ce_docente_allora_solo_o_codocente(pos, prof, last_prof)
            //se è già presente un codocente allora può esserci solo un docente
            && se_ce_codocente_allora_docente(pos, prof, last_prof)
            //se vuoto allora slot vuoto oppure docente
            && se_vuoto_allora_vuoto_o_docente(pos, prof, last_prof)
            // le ore totali (controllate sul docente di teoria) non hanno superato le ore classe e materia per la settimana
            && (codocenti[prof] || ore_classe_per_materia[g_classe(pos) * MATERIA_SZ + g_materia(pos)] <
                ore_classe_materia[g_classe(pos) * MATERIA_SZ + g_materia(pos)])
            // le ore totali di un docente doveno essere inferiori alle ore settimanale
            && ore_per_settimana_prof[prof] < ore_settimana_prof[prof]
            && 1;
}

// Do not pass s as parameter, the last part might form the previous calls.
// Draw the recursion tree to analyze the problem
void gen(int orario[], const int pos, int has_active_group, const int active_group, int current_group, const int tot,
         const int last_prof, int ore_materie_per_classe[], const int ore_per_settimana_prof[]) {
    if (n_gen_ok <= n_gen) return;

    // Controlli a fine blocco giorno ora classe
    if (0 &&
        pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0
        && !controllo_prof_insieme(orario, pos))
        return;

    if (pos == tot) {
        if (1
            || controlli_finali(ore_materie_per_classe, ore_per_settimana_prof)) {
            n_gen++;
            int rowlen = tot / actual_giorni_settimana;
            for (int j = 0; j < actual_giorni_settimana; j++) {
                printf("Giorno: %d   ", j + 1);
                for (int i = j * rowlen; i < (j + 1) * rowlen; i++)
                    if (orario[i] == -1)
                        printf("  -");
                    else
                        printf("  %d", orario[i]);
                printf("\n");
            }
            printf("\n");
        }
        return;
    }

    // reset params every giorno_ora_classe
    reset_params(pos, &has_active_group, &current_group);

    orario[pos] = -1;
    gen(orario, pos + 1, has_active_group, active_group, current_group, tot, -1, ore_materie_per_classe,
        ore_per_settimana_prof);
    for (int i = last_prof + 1; i < actual_insegnanti; i++) {
        if ((!has_active_group || active_group == current_group)
            && (1 || can_place(pos, i, last_prof, ore_materie_per_classe, ore_per_settimana_prof))) {
            orario[pos] = i;
            int ore_classe_materia_new[CLASSE_SEZIONE_SZ * MATERIA_SZ];
            memcpy(ore_classe_materia_new, ore_materie_per_classe, CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
            if (!codocenti[i]) // le ore si aggiornano solo sul docente di teoria
                // accesso alla riga g_classe(pos) e colonna g_materia(pos)
                ore_classe_materia_new[g_classe(pos) * MATERIA_SZ + g_materia(pos)]++;
            int ore_per_settimana_prof_new[INSEGNANTE_SZ];
            memcpy(ore_per_settimana_prof_new, ore_per_settimana_prof, INSEGNANTE_SZ*sizeof(int));
            ore_per_settimana_prof_new[i]++;
            gen(orario, pos + 1, 1, current_group, current_group, tot, i, ore_classe_materia_new,
                ore_per_settimana_prof_new);
        }
    }
}

int check_active_group_condition(int has_active_group, const int active_group, int current_group) {
    return !has_active_group || active_group == current_group;
}

void print_orario(int orario[][MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT], const int pos, int has_active_group,
                  const int active_group, int current_group, const int tot, int ore_materie_per_classe[],
                  const int ore_per_settimana_prof[]) {
    if (n_gen_ok <= n_gen) return;

    if (pos == tot) {
        n_gen++;
        print_orario_form(orario, tot);
        return;
    }

    reset_params(pos, &has_active_group, &current_group);

    orario[pos][0] = -1;
    orario[pos][1] = -1;
    print_orario(orario, pos + 1, has_active_group, active_group, current_group, tot, ore_materie_per_classe,
                 ore_per_settimana_prof);

    for (int i = -1; i < actual_insegnanti; i++) {
        if (check_active_group_condition(has_active_group, active_group, current_group)
            && insegnante_materia_adeguata(pos, i)) {
            orario[pos][0] = i;
            for (int j = i + 1; j < actual_insegnanti; j++) {
                if (insegnante_materia_adeguata(pos, j)) {
                    orario[pos][1] = j;
                    print_orario(orario, pos + 1, 1, current_group, current_group, tot, ore_materie_per_classe,
                                 ore_per_settimana_prof);
                }
            }
        }
    }
}

int main(void) {
    inizializza_dati();
    inizializza_vincoli();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione * actual_aula *
                 actual_materie;
    giorno_ora_classe_sz = actual_aula * actual_materie;
    //test(100);

    print_intestazioni(orario_len);
    int orario[orario_len][MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT];
    int ore_materie_per_classe[CLASSE_SEZIONE_SZ * MATERIA_SZ];
    int ore_per_settimana_prof[INSEGNANTE_SZ];
    memset(orario, 0, orario_len*sizeof(int));
    memset(ore_materie_per_classe, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
    memset(ore_per_settimana_prof, 0, INSEGNANTE_SZ*sizeof(int));

    print_orario(orario, 0, 0, 0, 0, orario_len, ore_materie_per_classe, ore_per_settimana_prof);

    printf("Totale generati: %llu\n", n_gen);
    fflush(stdout);

    return 0;
}
