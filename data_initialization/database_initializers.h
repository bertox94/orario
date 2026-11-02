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
#define MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO 3
#define MAX_ORE_PROF_IN_UNA_CLASSE 3

typedef struct {
    //int aula;
    //int materia;
    int insegnante1;
    int insegnante2;
    // posizione dello slot nell'orario (-1 se lo slot non e' ancora stato assegnato)
    int giorno;
    int ora;
    int classe;
    // valori calcolati una volta sola da (giorno, ora, classe), per non rifare le moltiplicazioni a ogni nodo
    // (vedi precalcola_slot in main.c)
    int base2d_ci;    // classe * INSEGNANTE_SZ
    int base2d_gc;    // giorno * CLASSE_SEZIONE_SZ
    int base3d_gci;   // giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + classe * INSEGNANTE_SZ
    int base3d_goi;   // giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ
    int base4d_queue; // inizio della coda dei docenti di questo slot in queue_mul
} teaching_slot;

extern char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
extern char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
extern char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
extern char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

extern int actual_giorni_settimana;
extern int actual_ore_giornaliere;
extern int actual_classi_sezione;
extern int actual_insegnanti;

// Per ogni coppia (classe, docente), indice classe * INSEGNANTE_SZ + docente.
typedef struct {
    int ore;          // ore totali da fare in questa classe
    int svolte;       // ore svolte fino a _pos_
    int alone;        // di cui da solo (senza codocente)
    int alone_svolte;
} classe_docente_t;

// Per ogni terna (classe, docente, codocente), prof a minore indice prima.
typedef struct {
    int ore;
    int svolte;
} codocenza_t;

typedef struct {
    int ore;    // ore totali della classe
    int svolte; // svolte in totale in una classe fino a _pos_
} classe_t;

typedef struct {
    int ore;         // ore totali del docente
    int ora_entrata; // prima ora in cui puo' fare lezione
} docente_t;

extern classe_docente_t classe_docente[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
extern codocenza_t codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
extern classe_t classe_info[CLASSE_SEZIONE_SZ];
extern docente_t docente_info[INSEGNANTE_SZ];

extern int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

// verifica che un prof, dato un giorno e un ora, stia facendo lezione, perciò nella stesso giorno e ora non può fare altro
extern int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];

// tiene traccia se in un giorno e una classa c'è un ora buca
extern int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];

void set_giorno_settimana(char giorno[]);

void set_ora_giornaliera(char ora[]);

void set_classe_sezione(char cla[]);

void set_insegnante(char in[]);

void set_classe_insegnante_ore(char cla[], char prof[], const int ore);

void set_classe_insegnante_ore_cod(char cla[], char prof[], const int ore, char cod[], const int ore_cod);

int p_insegnante(const char in[]);

int get_ore_classi_insegnante(int classe, int insegnante);

int get_ore_classi_insegnante_svolte(int classe, int insegnante);

void inizializza_calendario();

void inizializza_dati();

void stampa_dati();

#endif //UNTITLED_CONSTANTS_H
