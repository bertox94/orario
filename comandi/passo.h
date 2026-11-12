#ifndef UNTITLED_PASSO_H
#define UNTITLED_PASSO_H

// Un passo della ricerca, per il sito: ./untitled passo SEME CLASSI RITARDI ORDINE PASSI K
//
// Rigenera i dati del caso, rilegge dal file PASSI (passi.h) l'orario al nodo K, rimette lo stato della ricerca com'era
// a quel nodo (le ore gia' messe, la coda) e stampa su stdout, in JSON:
// - "pos": la cella che la ricerca sta per decidere, "prima": quella del nodo prima (null per il primo; se pos <=
//   prima la ricerca e' tornata indietro), "orario": [[docente, codocente], ...] le celle gia' decise;
// - "classe": lo stato della classe ("deve", "può", "non può" fare attivita': vedi stato_della_classe in
//   ricerca.c), il controllo «puo' avere lezione» con i suoi numeri, e se puo' avere un'ora buca;
// - "coda": la coda di priorita' in ordine;
// - "docenti": per ogni docente con ore in quella classe il valore di ogni controllo (tutti, non solo fino al primo
//   falso), da solo e in coppia con ognuno di quelli dopo di lui nella coda con cui ha ore insieme.
// Usa le funzioni della ricerca (vincoli.h), quindi i valori sono quelli che la ricerca vede.
int stampa_passo(unsigned long long int seme, int classi, int ritardi, int ordine, const char *file_passi,
                 long long int k);

#endif //UNTITLED_PASSO_H
