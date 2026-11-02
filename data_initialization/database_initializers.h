//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_CONSTANTS_H
#define UNTITLED_CONSTANTS_H

// I dati del problema: giorni, ore, classi, docenti, quante ore fa ogni docente in ogni classe (da solo o in
// codocenza) e lo stato che la ricerca aggiorna man mano che assegna gli slot.
// Tutto e' in vettori globali di dimensione fissa (le *_SZ sono i massimi); le actual_* dicono quanti elementi sono
// davvero usati. Le tabelle a piu' dimensioni sono vettori piatti: l'indice si calcola a mano (vedi i commenti).

// Dimensioni massime (capienza dei vettori, non i valori effettivi).
#define GIORNO_SETTIMANA_SZ 6
#define ORA_GIORNALIERA_SZ 6
#define CLASSE_SEZIONE_SZ 120
#define INSEGNANTE_SZ 250

#define MAX_LEN_WORDS 50 // lunghezza massima di un nome (giorno, ora, classe, docente), terminatore compreso

// Regole dell'orario.
#define PRIME_N_ORE_OBBLIGATORIE 5                  // le prime 5 ore di ogni giorno ogni classe ha sempre lezione
#define MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO 3 // un docente insegna in al massimo 3 classi diverse al giorno
#define MAX_ORE_PROF_IN_UNA_CLASSE 3                // e al massimo 3 ore al giorno nella stessa classe (contigue)

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

// Nomi; l'indice in questi vettori e' l'identificativo usato ovunque (giorno, ora, classe, docente sono interi).
extern char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
extern char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
extern char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
extern char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

// Quanti giorni, ore, classi e docenti ci sono davvero.
extern int actual_giorni_settimana;
extern int actual_ore_giornaliere;
extern int actual_classi_sezione;
extern int actual_insegnanti;

// Per ogni coppia (classe, docente), indice classe * INSEGNANTE_SZ + docente.
// ore = alone + ore in codocenza (con qualunque codocente). I campi *svolte li aggiorna la ricerca.
typedef struct {
    int ore;          // ore totali da fare in questa classe
    int svolte;       // ore svolte fino a _pos_
    int alone;        // di cui da solo (senza codocente)
    int alone_svolte;
} classe_docente_t;

// Per ogni terna (classe, docente, codocente), prof a minore indice prima:
// indice classe * INSEGNANTE_SZ * INSEGNANTE_SZ + min(p, q) * INSEGNANTE_SZ + max(p, q) (vedi idx_codocenza).
// ore = ore in cui i due docenti sono insieme nella classe (contate una volta sola, sono un solo slot).
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

// I vettori dei dati, con gli indici descritti sopra.
extern classe_docente_t classe_docente[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
extern codocenza_t codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
extern classe_t classe_info[CLASSE_SEZIONE_SZ];
extern docente_t docente_info[INSEGNANTE_SZ];

// Stato della ricerca. Ore fatte da un docente in una classe in un giorno;
// indice giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + classe * INSEGNANTE_SZ + docente (base3d_gci dello slot + docente).
extern int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

// verifica che un prof, dato un giorno e un ora, stia facendo lezione, perciò nella stesso giorno e ora non può fare altro
// (1 = impegnato); indice giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ + docente (base3d_goi + docente)
extern int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];

// tiene traccia se in un giorno e una classa c'è un ora buca (indice giorno * CLASSE_SEZIONE_SZ + classe):
// dopo un'ora buca la classe quel giorno non fa piu' lezione
extern int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];

// Aggiungono un nome al rispettivo elenco (se non c'e' gia').
void set_giorno_settimana(char giorno[]);

void set_ora_giornaliera(char ora[]);

void set_classe_sezione(char cla[]);

void set_insegnante(char in[]);

// Il docente prof fa `ore` ore da solo nella classe cla.
void set_classe_insegnante_ore(char cla[], char prof[], const int ore);

// Il docente prof fa `ore` ore nella classe cla; in `ore_cod` di queste c'e' anche il codocente cod
// (le altre ore - ore_cod le fa da solo). Le ore della classe aumentano di `ore`: le ore in coppia sono uno slot solo.
void set_classe_insegnante_ore_cod(char cla[], char prof[], const int ore, char cod[], const int ore_cod);

// Dal nome all'indice del docente (-1 se non c'e').
int p_insegnante(const char in[]);

// Ore totali, e svolte finora, del docente nella classe.
int get_ore_classi_insegnante(int classe, int insegnante);

int get_ore_classi_insegnante_svolte(int classe, int insegnante);

// La coda dei docenti (va preparata prima di aggiungere lezioni; inizializza_calendario la prepara gia').
void prepara_coda(void);

// Dal nome all'indice della classe (-1 se non c'e').
int p_classe_sezione(char cla[]);

// Giorni, ore e coda dei docenti (va chiamata prima di aggiungere i dati).
void inizializza_calendario();

// Dati reali del programma (classi e docenti scritti a mano), calendario compreso.
void inizializza_dati();

// Riepilogo dei dati su stdout (in printer.c).
void stampa_dati();

#endif //UNTITLED_CONSTANTS_H
