#ifndef UNTITLED_GENERATORE_H
#define UNTITLED_GENERATORE_H

// Vincoli dei dati casuali.
#define GEN_ORE_CLASSE_MIN 29   // ore settimanali di una classe (portate a PRIME_N_ORE_OBBLIGATORIE * giorni se piu' alte)
#define GEN_ORE_CLASSE_MAX 36
#define GEN_ORE_DOCENTE_MIN 5   // ore settimanali di un docente (come docente e come codocente)
#define GEN_ORE_DOCENTE_MAX 24

// Riempie classi, docenti, ore e codocenze con dati casuali, riproducibili a partire da `seed`.
// Almeno una lezione in codocenza e almeno una lezione da soli.
// Il calendario (giorni, ore, coda) deve essere gia' inizializzato (inizializza_calendario).
void genera_dati_casuali(unsigned long long seed, int n_classi);

#endif
