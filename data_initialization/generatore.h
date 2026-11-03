#ifndef UNTITLED_GENERATORE_H
#define UNTITLED_GENERATORE_H

// Vincoli dei dati casuali.
#define GEN_ORE_CLASSE_MIN 29   // ore settimanali di una classe (portate a PRIME_N_ORE_OBBLIGATORIE * giorni se piu' alte)
#define GEN_ORE_CLASSE_MAX 36
#define GEN_ORE_DOCENTE_MIN 5   // ore settimanali di un docente (come docente e come codocente)
#define GEN_ORE_DOCENTE_MAX 24
#define GEN_ORA_INGRESSO_MAX 3  // ora di ingresso piu' tarda (indice dell'ora)

// Con ritardi: circa un docente su cinque entra in ritardo, alla seconda, terza o quarta ora (ora_entrata 1, 2 o 3),
// sempre con ore a settimana che stanno ancora nelle ore che gli restano.
// Riempie classi, docenti, ore e codocenze con dati casuali, riproducibili a partire da `seed`.
// Almeno una lezione in codocenza e almeno una lezione da soli.
// Il calendario (giorni, ore, coda) deve essere gia' inizializzato (inizializza_calendario).
// I dati con e senza ritardi di uno stesso seme sono uguali, tranne le ore di ingresso.
// Se non e' NULL: file con un nome di docente per riga, usati al posto di D001, D002... (vedi generatore.c).
extern const char *file_nomi_docenti;

void genera_dati_casuali(unsigned long long seed, int n_classi, int con_ritardi);

#endif
