#ifndef UNTITLED_PASSO_H
#define UNTITLED_PASSO_H

// Un passo della ricerca, per il sito: ./untitled passo SEME CLASSI RITARDI ORDINE PASSI K
//
// Rigenera i dati del caso, rilegge dal file PASSI (passi.h) l'orario al nodo K, rimette lo stato della ricerca com'era
// a quel nodo (le ore gia' messe, la coda) e stampa su stdout, in JSON:
// - "pos": la cella che la ricerca sta per decidere, "prima": quella del nodo prima (null per il primo; se pos <=
//   prima la ricerca e' tornata indietro), "orario": [[docente, codocente], ...] le celle gia' decise;
// - "potatura": null, oppure la potatura per cui da questo nodo un orario non puo' piu' uscire (potature.h: il numero
//   da 1 a 7, il nome, e i numeri del conto che non torna: docente, classe, ora, servono, possibili; per la 1 anche
//   "oggi" e "oggi_da": quante delle possibili sono oggi e da che ora, indice); allora la ricerca torna indietro
//   senza provare niente in questa cella;
// - "classe": che cosa deve fare la classe ("deve", "può", "non può" fare attivita': vedi stato_della_classe in
//   ricerca.c) e i numeri da cui esce: se l'ora e' una delle prime, obbligatorie; le ore buche che ha gia' avuto
//   oggi; le ore che le restano; quante ne deve per forza fare nei giorni dopo e quanti posti ha in quei giorni;
// - "coda": la coda di priorita' in ordine;
// - "docenti": per ogni docente con ore in quella classe il valore di ogni controllo (tutti, non solo fino al primo
//   falso), da solo e in coppia con ognuno di quelli dopo di lui nella coda con cui ha ore insieme.
// Usa le funzioni della ricerca (vincoli.h), quindi i valori sono quelli che la ricerca vede.
int stampa_passo(unsigned long long int seme, int classi, int ritardi, int ordine, const char *file_passi,
                 long long int k);

#endif //UNTITLED_PASSO_H
