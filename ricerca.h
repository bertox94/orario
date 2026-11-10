#ifndef UNTITLED_RICERCA_H
#define UNTITLED_RICERCA_H

// La ricerca (ricerca.c), per chi la lancia: main.c con i dati di prova, prove.c con i dati casuali.

#include <stdio.h>

#include "data_initialization/database_initializers.h"

extern teaching_slot orario[];
extern int orario_len;
extern int trovato;                 // l'orario e' completo

// Il conto dei nodi (le chiamate di print_orario) e i limiti: non cambiano la ricerca, la fermano soltanto.
extern long long int nodi;          // nodi visitati
extern long long int limite_nodi;   // dopo tanti nodi la ricerca si ferma (0: nessun limite)
extern int fermato;                 // si e' fermata al limite
extern int pos_max;                 // la cella piu' avanti raggiunta
// se c'e', si chiama a ogni nodo, dopo averlo contato: la ricerca sta per decidere la cella pos e l'orario e' deciso
// fino a pos - 1 (per seguirla dal sito, vedi caso.c). Non cambia la ricerca.
extern void (*ogni_nodo)(int pos);

// Decide la cella pos e scende alle celle dopo (print_orario(0): tutto l'orario).
void print_orario(int pos);

#endif //UNTITLED_RICERCA_H
