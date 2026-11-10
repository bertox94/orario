//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_CONSTANTS_H
#define UNTITLED_CONSTANTS_H

// I dati della scuola e lo stato della ricerca. Gli array a piu' dimensioni sono piatti: classe, docente ->
// classe * INSEGNANTE_SZ + docente; giorno, ora, docente -> (giorno * ORA_GIORNALIERA_SZ + ora) * INSEGNANTE_SZ + docente;
// giorno, classe, docente -> (giorno * CLASSE_SEZIONE_SZ + classe) * INSEGNANTE_SZ + docente. Gli array "_svolte" sono le
// ore gia' messe nell'orario fino alla cella che la ricerca sta decidendo.

// le dimensioni massime
#define GIORNO_SETTIMANA_SZ 6
#define ORA_GIORNALIERA_SZ 6
#define CLASSE_SEZIONE_SZ 120
#define INSEGNANTE_SZ 250

#define MAX_LEN_WORDS 50

#define PRIME_N_ORE_OBBLIGATORIE 5                      // le prime 5 ore di ogni giorno hanno sempre lezione
#define MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO 3    // un docente insegna al massimo in 3 classi al giorno
#define MAX_ORE_PROF_IN_UNA_CLASSE 3                    // e al massimo 3 ore al giorno nella stessa classe

// una cella dell'orario: i due docenti (-1: nessuno; tutti e due -1: ora buca o cella non decisa). In una coppia
// insegnante1 e' quello che veniva prima nella coda, che puo' essere il docente della lezione o il codocente.
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
// ore di ogni docente in ogni classe (in tutto e da solo, senza codocente), e quante gia' messe
extern int ore_classi_docenti_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
extern int ore_classi_docenti_alone_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

extern int ore_classi[CLASSE_SEZIONE_SZ];
// ore di ogni classe, e quante gia' messe
extern int ore_classe_svolte[CLASSE_SEZIONE_SZ];

// ore di ogni coppia docente-codocente in ogni classe (classe, docente con l'indice minore, l'altro), e quante gia' messe
extern int ore_classi_codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
extern int ore_classi_codocenza_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];

// ore gia' messe di un docente in una classe in un giorno
extern int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

// il docente insegna gia' in quel giorno e a quell'ora (in un'altra classe)
extern int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];

// l'ora (indice) da cui il docente e' a scuola: 0 per tutti, tranne chi entra tardi
extern int ora_di_entrata_prof[INSEGNANTE_SZ];

// ore buche gia' messe in un giorno in una classe (dopo un'ora buca la classe non ha piu' lezione quel giorno)
extern int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];

void set_giorno_settimana(char giorno[]);

void set_ora_giornaliera(char ora[]);

void set_classe_insegnante_ore(char cla[], char prof[], const int ore);

void set_classe_insegnante_ore_cod(char cla[], char prof[], const int ore, char cod[], const int ore_cod);

int p_insegnante(const char in[]);

void increment_ore_classi_codocenza_svolte(int cla, int p1, int p2);

void decrement_ore_classi_codocenza_svolte(int cla, int p1, int p2);

int get_ore_insegnanti_codocenza_svolte_in_una_classe(int cla, int p1, int p2);

int get_ore_insegnanti_codocenza_in_una_classe(int cla, int p1, int p2);

// i dati di prova (dati_di_prova.c): giorni, ore, lezioni e ore di ingresso
void inizializza_dati();


#endif //UNTITLED_CONSTANTS_H
