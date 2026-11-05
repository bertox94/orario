#ifndef UNTITLED_TRACCIA_H
#define UNTITLED_TRACCIA_H

// Traccia della ricerca (opzione --traccia FILE), per il sito: un oggetto JSON per riga.
//
//   {"tipo":"dati", ...}        i nomi di giorni, ore, classi e docenti, le ore di ingresso, le lezioni
//   {"tipo":"backtrack", ...}   uno slot in cui la ricerca non ha potuto mettere nessun docente: l'orario fino a quel
//                               punto e, per ogni docente della coda, i controlli di main con ingressi e uscita
//   {"tipo":"orario", ...}      un orario completo trovato
//   {"tipo":"fine", ...}        la ricerca e' finita (esito, orari trovati, nodi, backtrack)
//
// La traccia osserva soltanto: i controlli vengono richiamati sullo stesso stato in cui la ricerca li ha appena
// valutati (sono funzioni di sola lettura), quindi le uscite sono quelle che ha visto la ricerca. Il percorso della
// ricerca non cambia.

#include <stdio.h>

extern long long int traccia_nodi;        // nodi visitati (chiamate di print_orario)
extern long long int traccia_backtrack_n; // backtrack incontrati

// Apre la traccia; max_backtrack = quanti backtrack registrare per esteso (gli altri si contano soltanto).
int traccia_apri(const char *percorso, long long int max_backtrack, int progresso);

void traccia_dati(void);

// Nessun docente utilizzabile nello slot (il "Backtrack" che main scrive in orario.txt). buca_dopo: dopo, la ricerca
// prova comunque l'ora buca (siamo oltre le ore obbligatorie), quindi non e' un vicolo cieco.
void traccia_backtrack(int pos, int giorno, int ora, int cla, int base2d_ci, int base3d_goi, int base4d_queue,
                       int buca_dopo);

// Lo slot non puo' avere lezione (activities_for_this_class_and_hour falso) e siamo nelle ore obbligatorie: la ricerca
// torna indietro senza provare nessun docente.
void traccia_slot_chiuso(int pos, int giorno, int ora, int cla);

void traccia_orario_trovato(void);

// Ogni tanto (dalla ricerca): scrive il progresso su stderr, se richiesto.
void traccia_progresso(int pos);

void traccia_fine(double secondi);

void traccia_errore(const char *messaggio);

void traccia_chiudi(void);

#endif
