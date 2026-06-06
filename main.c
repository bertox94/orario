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
long long int n_gen_ok = 300000000;
int orario_len;
int giorno_ora_classe_sz;
//Associazioni
int insegnante_materie[INSEGNANTE_SZ][MAX_MATERIE_PER_INSEGNANTE] = {};
int ore_materia_classe[CLASSE_SEZIONE_SZ][MATERIA_SZ] = {};

void print_intestazioni() {
    printf("Giorno:     ");
    for (int i = 0; i < orario_len / actual_ore_giornaliere / actual_classi_sezione / actual_aula / actual_materie /
                    MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; i++)
        for (int j = 0; j < actual_ore_giornaliere * actual_classi_sezione * actual_aula * actual_materie *
                        MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; j++)
            printf("  %d", i % actual_giorni_settimana);
    printf("\n");

    printf("Ora:        ");
    for (int i = 0; i < orario_len / actual_classi_sezione / actual_aula / actual_materie /
                    MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; i++)
        for (int j = 0; j < actual_classi_sezione * actual_aula * actual_materie * MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT;
             j++)
            printf("  %d", i % actual_ore_giornaliere);
    printf("\n");


    printf("Classe:     ");
    for (int i = 0; i < orario_len / actual_aula / actual_materie / MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; i++)
        for (int j = 0; j < actual_aula * actual_materie * MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; j++)
            printf("  %d", i % actual_classi_sezione);
    printf("\n");

    printf("Aula:       ");
    for (int i = 0; i < orario_len / actual_materie / MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; i++)
        for (int j = 0; j < actual_materie * MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; j++)
            printf("  %d", i % actual_aula);
    printf("\n");

    printf("Materia:    ");
    for (int i = 0; i < orario_len / MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; i++)
        for (int j = 0; j < MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT; j++)
            printf("  %d", i % actual_materie);
    printf("\n");

    printf("Insegnante: ");
    for (int i = 0; i < orario_len; i++)
        printf("  %d", i % 2);
    printf("\n");
}

int *int_allocation(const long max) {
    int *arr = malloc(max * (long) sizeof(int));
    if (arr == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return arr;
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
    for (int i = 0; i < actual_insegnanti; i++) {
        if (!strcmp(insegnanti[i], in))
            return i;
    }
    assert(0);
}

int g_materia(const int p) {
    return p / MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT % actual_materie;
}

int g_aula(const int p) {
    return p / actual_materie / MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT % actual_aula;
}

int g_classe(const int p) {
    return p / actual_aula / actual_materie / MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT % actual_classi_sezione;
}

int is_docente(const int p) {
    return p != -1 && !codocenti[p];
}

int is_codocente(const int p) {
    return p != -1 && codocenti[p];
}

int is_vuoto(const int p) {
    return p == -1;
}

void inizializza_giorni() {
    int pos = 0;
    strcpy(giorni_settimana[pos++], "Lun");
    //strcpy(giorni_settimana[pos++], "Mar");
    //strcpy(giorni_settimana[pos++], "Mer");
    //strcpy(giorni_settimana[pos++], "Gio");
    //strcpy(giorni_settimana[pos++], "Ven");
    //strcpy(giorni_settimana[pos++], "Sab");
    actual_giorni_settimana = pos;
}

void inizializza_ore() {
    int pos = 0;
    strcpy(ora_giornaliera[pos++], "08");
    strcpy(ora_giornaliera[pos++], "09");
    strcpy(ora_giornaliera[pos++], "10");
    strcpy(ora_giornaliera[pos++], "11");
    strcpy(ora_giornaliera[pos++], "12");
    strcpy(ora_giornaliera[pos++], "13");

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
    strcpy(materie[pos++], "Informatica"); //0
    strcpy(materie[pos++], "Sistemi e Reti"); //1
    //strcpy(materie[pos++], "TPS");
    actual_materie = pos;
}

void inizializza_insegnanti() {
    int pos = 0;
    strcpy(insegnanti[pos++], "Ricciardi"); //0
    strcpy(insegnanti[pos++], "Tiraboschi"); //1
    strcpy(insegnanti[pos++], "Gatti"); //2
    strcpy(insegnanti[pos++], "Barosi"); //3
    strcpy(insegnanti[pos++], "Rizzi"); //4
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
    memset(codocenti, 0, actual_insegnanti);
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
    memset(insegnante_materie, 0, actual_insegnanti*actual_materie);
    insegnante_materie[p_insegnante("Ricciardi")][p_materia("Sistemi e Reti")] = 1;
    insegnante_materie[p_insegnante("Tiraboschi")][p_materia("Sistemi e Reti")] = 1;
    insegnante_materie[p_insegnante("Tiraboschi")][p_materia("Informatica")] = 1;
    insegnante_materie[p_insegnante("Gatti")][p_materia("Informatica")] = 1;
    insegnante_materie[p_insegnante("Barosi")][p_materia("Informatica")] = 1;
    insegnante_materie[p_insegnante("Rizzi")][p_materia("Sistemi e Reti")] = 1;
}

void inizializza_ore_materie_classe() {
    memset(ore_materia_classe, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
    ore_materia_classe[p_classe_sezione("3H")][p_materia("Sistemi e Reti")] = 1;
    ore_materia_classe[p_classe_sezione("3H")][p_materia("Informatica")] = 1;
}

int verifica_ore_materia_classe(const int pos, int ore_materie_per_classe[][MATERIA_SZ]) {
    /*int ore_materia_classe_curr[CLASSE_SEZIONE_SZ][MATERIA_SZ];
    memset(ore_materia_classe_curr, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
    for (int i = 0; i < pos - 1; i += 2) {
        if (!is_vuoto(orario[i]) || !is_vuoto(orario[i + 1]))
            ore_materia_classe_curr[g_classe(i)][g_materia(i)]++;
    }
    */
    // verifica con le materie e le classi impostate
    for (int i = 0; i < CLASSE_SEZIONE_SZ; i++) {
        for (int j = 0; j < MATERIA_SZ; j++) {
            if (ore_materia_classe[i][j] != ore_materie_per_classe[i][j])
                return 0;
        }
    }
    return 1;
}

int insegnante_materia_adeguata(const int pos, int prof) {
    return insegnante_materie[prof][g_materia(pos)];
}

int codocente_deve_essere_in_lab_no_aula(const int pos, int prof) {
    if (codocenti[prof])
        return laboratori[g_aula(pos)];
    return 1;
}

int se_ce_docente_allora_solo_o_codocente(const int pos, const int prof, const int last_prof) {
    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0) return 1;
    if (is_docente(last_prof)) {
        return is_codocente(prof) || is_vuoto(prof);
    }
    return 1;
}

int se_ce_codocente_allora_docente(const int pos, const int prof, const int last_prof) {
    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0) return 1;
    if (is_codocente(last_prof)) {
        return is_docente(prof);
    }
    return 1;
}

int se_vuoto_allora_vuoto_o_docente(const int pos, const int prof, const int last_prof) {
    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0) return 1;
    if (is_vuoto(last_prof)) {
        return is_vuoto(prof) || is_docente(prof);
    }
    return 1;
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

int can_place(const int pos, const int prof, const int has_active_group, const int active_group,
              const int current_group, const int last_prof, int ore_materie_per_classe[][MATERIA_SZ]) {
    /*Each day and hour, teachers can be only in a class, room and subject*/
    return (!has_active_group || active_group == current_group)
           //insegnante insegna la propria materia
           && insegnante_materia_adeguata(pos, prof)
           // il codocente può essere assegnato soltanto a un lab
           && codocente_deve_essere_in_lab_no_aula(pos, prof)
           //se è già presente un docente nel gruppo corrente allora può essere solo o con un codocente
           && se_ce_docente_allora_solo_o_codocente(pos, prof, last_prof)
           //se è già presente un codocente allora può esserci solo un docente
           && se_ce_codocente_allora_docente(pos, prof, last_prof)
           //se vuoto allora slot vuoto oppure docente
           && se_vuoto_allora_vuoto_o_docente(pos, prof, last_prof)
           // le ore totali non hanno superato le ore classe e materia per la settimana
           && ore_materie_per_classe[g_classe(pos)][g_materia(pos)] < ore_materia_classe[g_classe(pos)][
               g_materia(pos)]
           && 1;
}

// Do not pass s as parameter, the last part might form the previous calls.
// Draw the recursion tree to analyze the problem
void gen(int *orario, const int pos, int has_active_group, int active_group,
         int current_group, const int tot, const int last_prof, int ore_materie_per_classe[][MATERIA_SZ]) {
    if (n_gen_ok <= n_gen) return;

    // Controlli a fine blocco giorno ora classe
    if (pos % giorno_ora_classe_sz == 0) {
    }

    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0)
        if (!controllo_prof_insieme(orario, pos))
            return;

    if (pos == tot) {
        if (!verifica_ore_materia_classe(pos, ore_materie_per_classe)) {
            return;
        }
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
        return;
    }

    // reset params every giorno_ora_classe
    if (pos % giorno_ora_classe_sz == 0)
        has_active_group = 0;

    if (pos % MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT == 0)
        current_group = pos / MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT;

    orario[pos] = -1;
    gen(orario, pos + 1, has_active_group, active_group, current_group, tot, -1,ore_materie_per_classe);
    for (int i = last_prof + 1; i < actual_insegnanti; i++) {
        if (can_place(pos, i, has_active_group, active_group, current_group, last_prof, ore_materie_per_classe)) {
            int ore_materie_per_classe_new[CLASSE_SEZIONE_SZ][MATERIA_SZ];
            orario[pos] = i;
            memcpy(ore_materie_per_classe_new, ore_materie_per_classe, CLASSE_SEZIONE_SZ*MATERIA_SZ);
            ore_materie_per_classe_new[g_classe(pos)][g_materia(pos)]++;
            gen(orario, pos + 1, 1, current_group, current_group, tot, i,ore_materie_per_classe);
        }
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

    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione * actual_aula * actual_materie
                 * MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT;
    giorno_ora_classe_sz = actual_aula * actual_materie * MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT;
    //test(100);
    print_intestazioni();
    // total number of profs should be below 255
    int *orario = int_allocation(orario_len);
     int ore_materie_per_classe[CLASSE_SEZIONE_SZ][MATERIA_SZ];
    memset(orario, 0, orario_len*sizeof(int)); // the first descent set the array eith all zeros
    memset(ore_materie_per_classe,0,CLASSE_SEZIONE_SZ*MATERIA_SZ);
    gen(orario, 0, 0, 0, 0, orario_len, -1,ore_materie_per_classe);
    printf("Totale generati: %llu\n", n_gen);
    fflush(stdout);
    
    free(orario);
    return 0;
    
    }