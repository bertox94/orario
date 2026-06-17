//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_CONSTANTS_H
#define UNTITLED_CONSTANTS_H
#define GIORNO_SETTIMANA_SZ 6

#define ORA_GIORNALIERA_SZ 6
#define ORA_GIORNALIERA_ITEM_SZ 3

#define CLASSE_SEZIONE_SZ 3

#define AULA_SZ 10

#define MATERIA_SZ 20

#define INSEGNANTE_SZ 35

#define MAX_LEN_WORDS 50

#define PRIME_N_ORE_OBBLIGATORIE 5

char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
char ore_giornaliere[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ];
char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
char aule[AULA_SZ][MAX_LEN_WORDS];
char materie[MATERIA_SZ][MAX_LEN_WORDS];
char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

int ore_classe_settimanali[CLASSE_SEZIONE_SZ];
int ore_insegnanti_classe_materia[CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ];

int actual_giorni_settimana;
int actual_ore_giornaliere;
int actual_classi_sezione;
int actual_materie;
int actual_insegnanti;

int giorno_ora_classi_sz;

typedef struct {
    int aula;
    int materia;
    int insegnante;
} teaching_slot;


void set_giorno_settimana(char giorno[]);

void set_ora_giornaliera(char ora[]);

void set_classe_sezione(char cla[]);

void set_aula(char aul[]);

void set_materia(char mat[]);

void set_insegnante(char in[]);

void set_laboratorio(char lab[]);

void set_solo_lez_in_lab(char cod[]);

void inizializza_dati();

void inizializza_vincoli();

void set_classe_materia_ore_prof(char cla[], char mat[], const int ore, const char prof[]);

void set_classe_materia_ore_prof_codocenza(char cla[], char mat[], const int ore, const char prof[], const int ore_cod,
                                           const char cod[]);

int p_aula(char a[]);

int p_materia(char mat[]);

int p_classe_sezione(char cla[]);

int p_insegnante(const char in[]);

#endif //UNTITLED_CONSTANTS_H
