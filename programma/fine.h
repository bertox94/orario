#ifndef FINE_H
#define FINE_H

#include "../solver/ricerca.h"

// Continuare una ricerca: i numeri che servono e che nei passi non ci sono, in un file accanto ai passi (FILE.fine).
// Com'e' fatto il file e' scritto in fine.c.

// Scrive il file alla fine di una ricerca con i passi in `file_passi`.
void scrivi_la_fine(const char *file_passi, const Ricerca *r, double secondi);
// Rimette in r i numeri della ricerca da continuare, e in *secondi quanto era durata. 0 se il file manca, non e'
// quello di quei `nodi` nodi, o la ricerca non era stata fermata dal limite (era finita: niente da continuare).
int leggi_la_fine(const char *file_passi, Ricerca *r, long long nodi, double *secondi);

#endif
