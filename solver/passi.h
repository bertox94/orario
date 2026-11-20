#ifndef PASSI_H
#define PASSI_H

#include "ricerca.h"

// I passi della ricerca su file, per rivederla dal sito un nodo alla volta. Solo uscita: la ricerca non cambia.
//
// FILE: un record per nodo, nell'ordine in cui la ricerca li visita: PASSI_CAMPI interi a 16 bit (nell'ordine dei byte
// del computer):
//   profondita (le celle decise), cella e voce (l'ultima scelta: la voce messa in quella cella, -1 l'ora buca; alla
//   radice -1 e -2), poi il motivo per cui da quel nodo non si va avanti: tipo (0: si va avanti; gli altri sono in
//   ricerca.h, TipoMotivo), docente, classe, ora, servono, possibili (-1: non c'entra).
// Il nodo k (da 1) e' il record k. L'orario a un nodo sono le scelte lungo la strada dalla radice: un record con
// profondita d sostituisce la scelta numero d e toglie quelle dopo.
//
// FILE.ist: ogni PASSI_ISTANTANEA nodi, tutta la strada di quel nodo: la profondita, poi per ogni cella dell'orario
// una coppia (cella, voce), le prime `profondita` nell'ordine in cui sono state decise (le altre non contano). Da li'
// un nodo qualunque si ricostruisce applicando al massimo PASSI_ISTANTANEA record.

#define PASSI_CAMPI 9
#define PASSI_ISTANTANEA 16384

typedef struct Passi Passi;

// Apre i due file per un orario di `celle` celle (NULL se non ci riesce); passi_nodo aggiunge il nodo numero
// `nodo` (da 1), con quello che la ricerca dice a ogni nodo (ricerca.h, a_ogni_nodo); passi_chiudi chiude.
Passi *passi_apri(const char *percorso, int celle);
void passi_nodo(Passi *p, long long nodo, int profondita, int cella, int voce, const Motivo *motivo);
void passi_chiudi(Passi *p);

#endif
