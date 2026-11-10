#ifndef UNTITLED_PASSI_H
#define UNTITLED_PASSI_H

// I passi della ricerca, per lo slider del sito (./untitled caso, vedi caso.h). Solo uscita: la ricerca non cambia.
//
// FILE: un record per nodo (chiamata di print_orario), nell'ordine in cui la ricerca li visita: tre interi a 16 bit
// (nell'ordine dei byte del computer): la cella che la ricerca sta per decidere (pos) e la scelta appena messa nella
// cella prima, pos - 1 (docente e codocente; -1: nessuno). Il nodo k (da 1) e' il record k.
// FILE.ist: ogni PASSI_ISTANTANEA nodi, dopo il record di quel nodo, tutto l'orario (per ogni cella docente e
// codocente, interi a 16 bit). Da li' si ricostruisce un nodo qualunque applicando al massimo PASSI_ISTANTANEA record:
// al nodo k le celle prima di pos sono decise, le altre no.
//
// Ogni cella si scrive subito prima di entrare nella cella dopo (in print_orario non ci sono altri punti in cui
// cambia orario[pos]), quindi il record di un nodo dice tutto quello che e' cambiato dal nodo prima.

#include "data_initialization/database_initializers.h"

#define PASSI_ISTANTANEA 16384

int passi_apri(const char *percorso);
void passi_nodo(long long int nodo, int pos, const teaching_slot *orario, int celle);
void passi_chiudi(void);

#endif //UNTITLED_PASSI_H
