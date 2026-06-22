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

#define PRIME_N_ORE_OBBLIGATORIE 5
#define MAX_ORE_STESSA_MATERIA_GIORNO 2

extern char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
extern char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
extern char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
extern char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

extern int actual_giorni_settimana;
extern int actual_ore_giornaliere;
extern int actual_classi_sezione;
extern int actual_insegnanti;

extern int ore_classi_docenti[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
//associa ad ogni classe e insegnante le ore totali svolte fino a _pos_
extern int ore_classi_docenti_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

extern int ore_classi[CLASSE_SEZIONE_SZ];
// tiene traccia delle svolte in totale in una classe fino a _pos_
extern int ore_classe_svolte[CLASSE_SEZIONE_SZ];

extern int ore_docenti[INSEGNANTE_SZ];
// tiene traccia del numero di ore svolte da un docente fino a _pos_
extern int ore_docenti_svolte[INSEGNANTE_SZ];

// verifica che un prof, dato un giorno e un ora, stia facendo lezione, perciò nella stesso giorno e ora non può fare altro
extern int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];

// tiene traccia se in un giorno e una classa c'è un ora buca
extern int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];
// tiene traccia delle ore svolte da un insegnante in un giorno e una classe (serve per non avere lezioni di tre ore con lo stesso insegnante)
extern int ore_prof_giorno_classe[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
//TODO: un prof vede una classe una sola volta al giorno

void set_giorno_settimana(char giorno[]);

void set_ora_giornaliera(char ora[]);

void set_classe_sezione(char cla[]);

void set_insegnante(char in[]);

void set_classe_insegnante_ore(char cla[], char prof[], const int ore);

int p_insegnante(const char in[]);

int get_ore_classi_insegnante(int classe, int insegnante);

void inizializza_dati();

void stampa_dati();

#endif //UNTITLED_CONSTANTS_H
