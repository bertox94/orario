#ifndef UNTITLED_RICERCA_H
#define UNTITLED_RICERCA_H

// La ricerca ricorsiva (backtracking) dell'orario, slot dopo slot.

#include <stdio.h>

#define SCHEDULES_PER_GB 55000 // orari completi per GB di orario.txt; il limite di default e' un quinto (circa 200 MB)

extern long long int n_gen;          // orari completi trovati
extern long long int n_gen_max;      // la ricerca si ferma dopo tanti orari completi
extern long long int n_backtrack;    // quante volte non si e' potuto scegliere nessun docente per uno slot
extern int stampa_backtrack;         // se 1, ad ogni marcia indietro scrive la riga di avviso e la griglia parziale (lento)
extern FILE *file_orario;            // dove si scrivono gli orari trovati (e le marce indietro con stampa_backtrack)

// Cerca gli orari a partire dallo slot pos (la prima chiamata e' con pos = 0).
void cerca_orario(int pos);

#endif //UNTITLED_RICERCA_H
