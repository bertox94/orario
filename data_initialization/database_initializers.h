//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_CONSTANTS_H
#define UNTITLED_CONSTANTS_H

// I dati della scuola: giorni, ore, classi, docenti, le ore di ogni docente in ogni classe (da solo e in coppia) e le
// ore di ingresso. Stanno tutti in una Scuola, che chi lancia il programma crea (nuova_scuola) e passa a chi la
// riempie (inizializza_dati, genera_dati_casuali) e a chi la legge (la verifica dei dati, la ricerca, la verifica
// dell'orario). Gli array a due dimensioni sono piatti: classe, docente -> classe * INSEGNANTE_SZ + docente.

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

typedef struct {
    // i nomi, nell'ordine in cui sono stati registrati (l'ordine da' gli indici)
    char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
    char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
    char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
    char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];
    // quanti giorni, ore, classi e docenti ci sono davvero (gli array sono grandi per il massimo)
    int actual_giorni_settimana;
    int actual_ore_giornaliere;
    int actual_classi_sezione;
    int actual_insegnanti;
    // ore di ogni docente in ogni classe: in tutto (da solo, in coppia, e quelle che fa come codocente) e da solo
    int ore_classi_docenti[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
    int ore_classi_docenti_alone[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
    // ore di ogni classe
    int ore_classi[CLASSE_SEZIONE_SZ];
    // ore di ogni coppia docente-codocente in ogni classe (classe, docente con l'indice minore, l'altro): si leggono
    // con get_ore_insegnanti_codocenza_in_una_classe
    int ore_classi_codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
    // l'ora (indice) da cui il docente e' a scuola: 0 per tutti, tranne chi entra tardi
    int ora_di_entrata_prof[INSEGNANTE_SZ];
} Scuola;

// Una scuola vuota (e' grande: sta nella memoria dinamica). libera_scuola quando non serve piu'.
Scuola *nuova_scuola(void);
void libera_scuola(Scuola *s);

// Registrare giorni e ore; una lezione registra anche la classe e i docenti, se non ci sono ancora.
void set_giorno_settimana(Scuola *s, const char giorno[]);
void set_ora_giornaliera(Scuola *s, const char ora[]);
// il docente prof fa `ore` ore nella classe cla, tutte da solo
void set_classe_insegnante_ore(Scuola *s, const char cla[], const char prof[], int ore);
// prof fa `ore` ore nella classe, di cui ore_cod insieme a cod (e ore - ore_cod da solo)
void set_classe_insegnante_ore_cod(Scuola *s, const char cla[], const char prof[], int ore, const char cod[],
                                   int ore_cod);

// l'indice del docente con questo nome (-1 se non c'e')
int p_insegnante(const Scuola *s, const char in[]);
// le ore del docente p nella classe: in tutto (da solo, in coppia, e quelle che fa come codocente) e da solo
int get_ore_insegnante_in_una_classe(const Scuola *s, int cla, int p);
int get_ore_insegnante_da_solo_in_una_classe(const Scuola *s, int cla, int p);
// le ore che p1 e p2 hanno insieme nella classe (in qualunque ordine)
int get_ore_insegnanti_codocenza_in_una_classe(const Scuola *s, int cla, int p1, int p2);
// una coppia di docenti sempre nello stesso ordine: prima l'indice minore
void swap_min_first(int *p1, int *p2);

// i dati di prova (dati_di_prova.c): giorni, ore, lezioni e ore di ingresso
void inizializza_dati(Scuola *s);

#endif //UNTITLED_CONSTANTS_H
