// Lo stato del solver: i nomi, le ore da fare e quelle svolte (vedi database_initializers.c).
#ifndef UNTITLED_CONSTANTS_H
#define UNTITLED_CONSTANTS_H

#define GIORNO_SETTIMANA_SZ 6
#define ORA_GIORNALIERA_SZ 6
#define CLASSE_SEZIONE_SZ 120
#define INSEGNANTE_SZ 250

#define MAX_LEN_WORDS 50

#define PRIME_N_ORE_OBBLIGATORIE 5
#define MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO 3
#define MAX_ORE_PROF_IN_UNA_CLASSE 3

// una cella dell'orario: i due docenti (-1: nessuno; insegnante2 >= 0 solo in coppia)
typedef struct {
    int insegnante1;
    int insegnante2;
} teaching_slot;

extern char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
extern char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
extern char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
extern char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

extern int actual_giorni_settimana;
extern int actual_ore_giornaliere;
extern int actual_classi_sezione;
extern int actual_insegnanti;

extern int ore_classi_docenti[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
extern int ore_classi_docenti_alone[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
// le ore svolte (classe, docente) fino alla cella che la ricerca sta decidendo
extern int ore_classi_docenti_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
extern int ore_classi_docenti_alone_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

extern int ore_classi[CLASSE_SEZIONE_SZ];
extern int ore_classe_svolte[CLASSE_SEZIONE_SZ];

extern int ore_docenti[INSEGNANTE_SZ];
extern int ore_docenti_svolte[INSEGNANTE_SZ];
// le ore in coppia (classe, docente, codocente), con il docente a indice minore prima
extern int ore_classi_codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
extern int ore_classi_codocenza_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];

extern int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

// il docente e' gia' in una classe in questo (giorno, ora)
extern int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];

extern int ora_di_entrata_prof[INSEGNANTE_SZ];

// la classe ha gia' avuto un'ora buca in questo giorno
extern int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];

// registrare i dati (nell'ordine di arrivo: decide gli indici dei docenti e quindi la coda)
void set_giorno_settimana(char giorno[]);
void set_ora_giornaliera(char ora[]);
void set_classe_insegnante_ore(char cla[], char prof[], int ore);
void set_classe_insegnante_ore_cod(char cla[], char prof[], int ore, char cod[], int ore_cod);
int p_classe_sezione(char cla[]);
int p_insegnante(const char in[]);

// le ore (classe, docente) e le ore in coppia (classe, docente, codocente)
int get_ore_classi_insegnante(int classe, int insegnante);
int get_ore_classi_insegnante_svolte(int classe, int insegnante);
int get_ore_insegnanti_codocenza_in_una_classe(int cla, int p1, int p2);
int get_ore_insegnanti_codocenza_svolte_in_una_classe(int cla, int p1, int p2);
void increment_ore_classi_codocenza_svolte(int cla, int p1, int p2);
void decrement_ore_classi_codocenza_svolte(int cla, int p1, int p2);

// i dati di prova, senza --input
void inizializza_dati(void);

#endif //UNTITLED_CONSTANTS_H
