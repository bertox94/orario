#ifndef UNTITLED_FATTIBILITA_H
#define UNTITLED_FATTIBILITA_H

#include <stddef.h>

// Verifica preliminare, prima di cercare: controlla condizioni necessarie sui dati. Se una non e' soddisfatta il caso
// non ha soluzioni (qualunque sia l'ordine della ricerca) e la ricerca sarebbe tempo perso.
// Ritorna 1 se nessuna condizione e' violata (non vuol dire che la soluzione esista), altrimenti 0 e scrive in `motivo`
// la prima condizione violata.
int verifica_fattibilita(char *motivo, size_t dimensione);

#endif //UNTITLED_FATTIBILITA_H
