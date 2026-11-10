#ifndef UNTITLED_CONTROLLI_H
#define UNTITLED_CONTROLLI_H

// I controlli di un nodo, per il sito: ./untitled controlli SEME CLASSI RITARDI PASSI K
//
// Rigenera i dati del caso, ricostruisce dal file PASSI (passi.h) l'orario al nodo K, rimette lo stato della ricerca
// com'era a quel nodo (le ore gia' messe, la coda) e stampa su stdout, in JSON, per la cella che la ricerca sta per
// decidere: il controllo della classe (puo' avere lezione?) con i suoi numeri, la coda di priorita' in ordine, e per
// ogni docente con ore in quella classe il valore di ogni controllo (tutti, non solo fino al primo falso), da solo e
// in coppia con ognuno di quelli dopo di lui nella coda con cui ha ore insieme. Usa le funzioni della ricerca
// (vincoli.h), quindi i valori sono quelli che la ricerca vede.
int stampa_controlli(unsigned long long int seme, int classi, int ritardi, const char *file_passi, long long int k);

#endif //UNTITLED_CONTROLLI_H
