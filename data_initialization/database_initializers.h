//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_CONSTANTS_H
#define UNTITLED_CONSTANTS_H
#define GIORNO_SETTIMANA_SZ 6
#define GIORNO_SETTIMANA_ITEM_SZ 4

#define ORA_GIORNALIERA_SZ 6
#define ORA_GIORNALIERA_ITEM_SZ 3

#define CLASSE_SEZIONE_SZ 3
#define CLASSE_SEZIONE_ITEM_SZ 3

#define AULA_SZ 10
#define AULA_ITEM_SZ 15

#define MATERIA_SZ 20
#define MATERIA_ITEM_SZ 50

#define INSEGNANTE_SZ 25
#define INSEGNANTE_ITEM_SZ 15

#define MAX_PROF_IN_SAME_CLASS_ROOM_SUBJECT 2
#define PRIME_N_ORE_OBBLIGATORIE 4

char giorni_settimana[GIORNO_SETTIMANA_SZ][GIORNO_SETTIMANA_ITEM_SZ];
char ora_giornaliera[ORA_GIORNALIERA_SZ][ORA_GIORNALIERA_ITEM_SZ];
char classe_sezione[CLASSE_SEZIONE_SZ][CLASSE_SEZIONE_ITEM_SZ];
char aule[AULA_SZ][AULA_ITEM_SZ];
char materie[MATERIA_SZ][MATERIA_ITEM_SZ];
char insegnanti[INSEGNANTE_SZ][INSEGNANTE_ITEM_SZ];
int laboratori[AULA_SZ];
int solo_in_lab[INSEGNANTE_SZ];
int giorno_libero_prof[INSEGNANTE_SZ];
int ore_classe_settimanali[CLASSE_SEZIONE_SZ];

int ore_insegnanti_classe_materia[CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ];
int ore_insegnanti_classe_materia_lonely[CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ];

int actual_giorni_settimana;
int actual_ore_giornaliere;
int actual_classi_sezione;
int actual_aula;
int actual_laboratori;
int actual_materie;
int actual_insegnanti;

int giorno_ora_classi_sz;


typedef struct {
    int aula;
    int materia;
    int insegnante1;
    int insegnante2;
} elem;


void set_giorno_settimana(char giorno[], int pos);

void set_ora_giornaliera(char ora[], int pos);

void set_classe_sezione(char cla[]);

void set_aula(char aul[], const int pos);

void set_materia(char mat[]);

void set_insegnante(char in[]);

void set_laboratorio(char lab[], int pos);

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
