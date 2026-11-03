#ifndef UNTITLED_RICERCA_H
#define UNTITLED_RICERCA_H

// La ricerca ricorsiva (backtracking) dell'orario, slot dopo slot.

#include <stdio.h>

#include "../data_initialization/database_initializers.h"

#define SCHEDULES_PER_GB 55000 // orari completi per GB di orario.txt; il limite di default e' un quinto (circa 200 MB)

extern long long int n_gen;          // orari completi trovati
extern long long int n_gen_max;      // la ricerca si ferma dopo tanti orari completi
extern long long int n_backtrack;    // quante volte non si e' potuto scegliere nessun docente per uno slot
// Cosa scrivere a ogni marcia indietro: niente, una riga compatta (-b) o la griglia parziale intera (--backtrack-griglia,
// molto lento e molto grande: circa 30 KB per marcia).
enum { BACKTRACK_NO, BACKTRACK_COMPATTO, BACKTRACK_GRIGLIA };
extern int stampa_backtrack;
extern long long passi_max;         // con -b: a ogni nodo una riga "P" con l'orario parziale, al massimo passi_max (0 = mai)
extern FILE *file_orario;            // dove si scrivono gli orari trovati (e le marce indietro con stampa_backtrack)

// Il primo orario completo trovato (docenti di ogni slot, -1 = nessuno), valido se n_gen > 0. La ricerca annulla le
// assegnazioni tornando indietro, quindi alla fine orario[] e' vuoto: il primo orario si copia qui appena trovato.
extern int primo_orario_1[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
extern int primo_orario_2[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];

// Cerca gli orari a partire dallo slot pos (la prima chiamata e' con pos = 0).
void cerca_orario(int pos);

// Per il sito: la coda dei docenti dello slot pos, dati i primi pos slot letti da file, e perche' ognuno e' o non e'
// utilizzabile. Scrive una riga JSON su out. Ritorna 0 se il file non va bene.
int analizza_slot(const char *file, FILE *out);

#endif //UNTITLED_RICERCA_H
