#ifndef UNTITLED_VERIFICA_DATI_H
#define UNTITLED_VERIFICA_DATI_H

#include <stddef.h>

// La verifica dei dati, prima della ricerca: condizioni necessarie che si vedono dai dati senza cercare. Se una non
// vale l'orario non esiste e la ricerca non parte («errore nei dati»); se valgono tutte non e' garantito niente.
// Non tocca la ricerca: legge soltanto i dati.
//
// 1 se i dati passano tutti i controlli; se no 0, con il primo che non vale spiegato in errore.
int verifica_dati(char *errore, size_t dimensione);

#endif //UNTITLED_VERIFICA_DATI_H
