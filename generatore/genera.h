#ifndef GENERA_H
#define GENERA_H

// Casi casuali, riproducibili: lo stesso seme, lo stesso numero di classi e la stessa scelta sui ritardi danno sempre
// lo stesso caso.
//
// Sei giorni di sei ore, le prime cinque con lezione. Ogni classe ha da 30 a 36 ore, divise in lezioni da 1 a 6 ore;
// una parte delle lezioni ha anche un codocente, per alcune delle sue ore. I docenti sono tanti da avere in media tra
// 10 e 20 ore l'uno, e ogni lezione va al meno carico fra tre presi a caso. Con i ritardi alcuni docenti entrano alla
// seconda, alla terza o alla quarta ora: un caso con i ritardi puo' non avere nessun orario.
//
// Scrive il caso su `uscita`, nel formato che leggi_scuola legge. 0 se non riesce a generarlo.
#include <stdio.h>

int genera_caso(FILE *uscita, unsigned long long seme, int classi, int con_ritardi);

#endif
