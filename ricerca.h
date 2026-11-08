#ifndef UNTITLED_RICERCA_H
#define UNTITLED_RICERCA_H

// La ricerca (main.c), per chi la lancia: main() con i dati di prova, prove.c con i dati casuali.

#include <stdio.h>

#include "data_initialization/database_initializers.h"

extern teaching_slot orario[];
extern int orario_len;
extern int trovato;                 // l'orario e' completo
extern FILE *f;                     // dove stampare l'orario trovato (NULL: da nessuna parte)

// Il conto dei nodi (le chiamate di print_orario) e i limiti: non cambiano la ricerca, la fermano soltanto.
extern long long int nodi;          // nodi visitati
extern long long int limite_nodi;   // dopo tanti nodi la ricerca si ferma (0: nessun limite)
extern int fermato;                 // si e' fermata al limite
extern int pos_max;                 // la cella piu' avanti raggiunta
extern FILE *passi;                 // ogni nodo, una riga (NULL: no), fino a passi_max nodi
extern long long int passi_max;
// l'ultimo backtrack (NULL: no): dal nodo dove la ricerca si blocca, ogni nodo finche' non va oltre quella cella; il
// blocco dopo ricomincia il file. Alla fine chiudi_ultimo_backtrack() scrive come finisce e taglia il file li'.
extern FILE *ultimo_backtrack;
void chiudi_ultimo_backtrack(void);

// Decide la cella pos e scende alle celle dopo (print_orario(0): tutto l'orario).
void print_orario(int pos);

#endif //UNTITLED_RICERCA_H
