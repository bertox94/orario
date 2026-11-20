#ifndef VERIFICA_H
#define VERIFICA_H

#include "scuola.h"

// La verifica di un orario finito, indipendente dalla ricerca: legge solo la scuola e le celle (per ogni cella una
// voce, o un numero negativo per l'ora buca) e ricontrolla da capo ogni regola. Se la ricerca sbagliasse, qui si
// vedrebbe.

#define CONTROLLI 8

typedef struct {
    const char *regola;
    int rispettata;
    char dove[160];  // se non e' rispettata: il primo punto in cui non lo e'
} Controllo;

// Riempie i controlli, uno per regola; 1 se l'orario le rispetta tutte.
int verifica_orario(const Scuola *s, const int *orario, Controllo controllo[CONTROLLI]);

#endif
