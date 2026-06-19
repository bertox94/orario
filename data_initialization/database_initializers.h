//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_CONSTANTS_H
#define UNTITLED_CONSTANTS_H

#define GIORNO_SETTIMANA_SZ 6
#define ORA_GIORNALIERA_SZ 6
#define CLASSE_SEZIONE_SZ 120
#define INSEGNANTE_SZ 250

#define MAX_LEN_WORDS 50

#define PRIME_N_ORE_OBBLIGATORIE 0

extern char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
extern char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
extern char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
extern char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

extern int actual_giorni_settimana;
extern int actual_ore_giornaliere;
extern int actual_classi_sezione;
extern int actual_insegnanti;

extern int ore_insegnante_classi[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
extern int ore_classi_settimanali[CLASSE_SEZIONE_SZ];
extern int ore_insegnante_settimanali[INSEGNANTE_SZ];

void set_giorno_settimana(char giorno[]);

void set_ora_giornaliera(char ora[]);

void set_classe_sezione(char cla[]);

void set_insegnante(char in[]);

void set_classe_insegnante_ore(char cla[], char prof[], const int ore);

int p_insegnante(const char in[]);

void inizializza_dati();

void stampa_dati();

#endif //UNTITLED_CONSTANTS_H
