#ifndef UNTITLED_TRACCIA_H
#define UNTITLED_TRACCIA_H

// Traccia della ricerca (opzione --traccia FILE), per il sito: un oggetto JSON per riga.
//
//   {"tipo":"dati", ...}        i nomi di giorni, ore, classi e docenti, le ore di ingresso, le lezioni
//   {"tipo":"backtrack", ...}   uno slot in cui la ricerca non ha potuto mettere nessun docente: l'orario fino a quel
//                               punto e, per ogni docente della coda, i controlli di main con ingressi e uscita
//   {"tipo":"nodi","da":N,"passi":[[pos,p1,p2],...]}
//                               i nodi della ricerca da N in poi (con --max-nodi): per ogni nodo la posizione a cui
//                               arriva e la coppia appena messa nella posizione prima (-1 = nessuno, -1,-1 = ora buca;
//                               al nodo con pos 0 non c'e' coppia). Le righe "nodi" arrivano prima del backtrack o
//                               dell'orario dello stesso nodo, quindi la pagina ricostruisce ogni passo della ricerca
//   {"tipo":"orario", ...}      un orario completo trovato
//   {"tipo":"fine", ...}        la ricerca e' finita (esito, orari trovati, nodi, backtrack)
//
// La traccia osserva soltanto: i controlli vengono richiamati sullo stesso stato in cui la ricerca li ha appena
// valutati (sono funzioni di sola lettura), quindi le uscite sono quelle che ha visto la ricerca. Il percorso della
// ricerca non cambia.

#include <stdio.h>

extern long long int traccia_nodi;        // nodi visitati (chiamate di print_orario)
extern long long int traccia_backtrack_n; // backtrack incontrati
extern int traccia_con_nodi;              // la ricerca chiama traccia_nodo (finche' non si supera un limite)

// Apre la traccia; max_backtrack = quanti backtrack registrare per esteso (gli altri si contano soltanto);
// max_nodi = quanti nodi registrare al massimo (0 = nessuno). I nodi si registrano finche' i backtrack restano entro
// max_backtrack, cosi' arrivano fino alla risoluzione dell'ultimo gruppo di backtrack registrati.
int traccia_apri(const char *percorso, long long int max_backtrack, long long int max_nodi, int progresso);

// Un nodo della ricerca (una chiamata di print_orario(pos)): chiamata solo se traccia_con_nodi.
void traccia_nodo(int pos);

void traccia_dati(void);

// Nessun docente utilizzabile nello slot (il "Backtrack" che main scrive in orario.txt). buca_dopo: dopo, la ricerca
// prova comunque l'ora buca (siamo oltre le ore obbligatorie), quindi non e' un vicolo cieco.
void traccia_backtrack(int pos, int giorno, int ora, int cla, int base2d_ci, int base3d_goi, int base4d_queue,
                       int buca_dopo);

// Lo slot non puo' avere lezione (activities_for_this_class_and_hour falso) e siamo nelle ore obbligatorie: la ricerca
// torna indietro senza provare nessun docente.
void traccia_slot_chiuso(int pos, int giorno, int ora, int cla);
// la scelta nella cella prima lascia un resto impossibile (una potatura, vedi potature/potature.h)
void traccia_potatura(int pos, int giorno, int ora, int cla);

void traccia_orario_trovato(void);

// Ogni tanto (dalla ricerca): scrive il progresso su stderr, se richiesto.
void traccia_progresso(int pos);

void traccia_fine(double secondi);

void traccia_errore(const char *messaggio);

void traccia_chiudi(void);

#endif
