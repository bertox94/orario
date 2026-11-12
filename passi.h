#ifndef UNTITLED_PASSI_H
#define UNTITLED_PASSI_H

// I passi della ricerca, per lo slider del sito (./untitled caso, vedi caso.h). Solo uscita: la ricerca non cambia.
//
// FILE: un record per nodo (chiamata di cerca_orario), nell'ordine in cui la ricerca li visita: tre interi a 16 bit
// (nell'ordine dei byte del computer): la cella che la ricerca sta per decidere (pos) e la scelta appena messa nella
// cella prima, pos - 1 (docente e codocente; -1: nessuno). Il nodo k (da 1) e' il record k.
// FILE.ist: ogni PASSI_ISTANTANEA nodi, dopo il record di quel nodo, tutto l'orario (per ogni cella docente e
// codocente, interi a 16 bit). Da li' si ricostruisce un nodo qualunque applicando al massimo PASSI_ISTANTANEA record:
// al nodo k le celle prima di pos sono decise, le altre no.
//
// Ogni cella si scrive subito prima di entrare nella cella dopo (in cerca_orario non ci sono altri punti in cui
// cambia orario[pos]), quindi il record di un nodo dice tutto quello che e' cambiato dal nodo prima.

#include <stdio.h>

#include "data_initialization/database_initializers.h"

#define PASSI_ISTANTANEA 16384

// Scrivere: passi_apri apre i due file (NULL se non ci riesce), passi_nodo aggiunge un nodo, passi_chiudi chiude.
typedef struct Passi Passi;
Passi *passi_apri(const char *percorso);
void passi_nodo(Passi *x, long long int nodo, int pos, const teaching_slot *orario, int celle);
void passi_chiudi(Passi *x);

// Legge il prossimo nodo da FILE aperto in lettura (fopen(percorso, "rb")): in *pos la cella che il nodo sta per
// decidere, in *prima la scelta appena messa nella cella pos - 1. Ritorna 0 quando i nodi sono finiti.
int passi_prossimo(FILE *f, int *pos, teaching_slot *prima);

// Rilegge il nodo k (da 1) dal file: riempie orario[] fino alla cella che il nodo sta per decidere e la ritorna
// (-1: il nodo non c'e'); in *pos_prima la cella che decideva il nodo prima (-1 per il primo nodo). celle: quante celle
// ha l'orario.
int passi_leggi(const char *percorso, long long int k, int celle, teaching_slot *orario, int *pos_prima);

#endif //UNTITLED_PASSI_H
