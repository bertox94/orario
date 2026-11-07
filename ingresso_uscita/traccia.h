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

#include "../potature/potature.h"
#include "../priority_queue/prio_queue.h"

extern long long int traccia_nodi;        // nodi visitati (chiamate di print_orario)
extern long long int traccia_ferma_nodi;  // --ferma-nodi: fermarsi dopo tanti nodi (0: mai)
extern long long int traccia_controlli_nodo;  // --controlli-nodo N: al nodo N i controlli dei docenti della cella, poi fine
extern long long int traccia_prossimo;    // a questo nodo la ricerca chiama traccia_progresso
extern long long int traccia_backtrack_n; // backtrack incontrati
extern int traccia_pos_max;               // la posizione piu' avanti raggiunta dalla ricerca
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
// Il passo passo del sito, ai nodi che non sono backtrack: al nodo traccia_controlli_nodo (entrando nella cella, dopo le
// potature e con la coda gia' ordinata) un record come un backtrack ma con "tipo":"controlli", poi la traccia si chiude e
// il programma esce. Lo stato all'ingresso della cella e' lo stesso di quando la ricerca la lascia (le scelte sono
// tolte), quindi i controlli sono quelli che la ricerca fa in quella cella; il percorso fino a li' non cambia.
void traccia_controlli(int pos, int giorno, int ora, int cla, int base2d_ci, int base3d_goi, int base4d_queue);
void traccia_backtrack(int pos, int giorno, int ora, int cla, int base2d_ci, int base3d_goi, int base4d_queue,
                       int buca_dopo);

// Lo slot non puo' avere lezione (activities_for_this_class_and_hour falso) e siamo nelle ore obbligatorie: la ricerca
// torna indietro senza provare nessun docente.
void traccia_slot_chiuso(int pos, int giorno, int ora, int cla);
// la scelta nella cella prima lascia un resto impossibile (una potatura, vedi potature/potature.h)
void traccia_potatura(int pos, int giorno, int ora, int cla, const Potatura *p);

// ---- per i solver di solutori/ (la stessa traccia, con due aggiunte) ----
// Un solver che non decide le celle in ordine di indice (per-classe): cella_di[s] e' la cella decisa al passo s,
// indice_di[cella] il passo. Da chiamare prima della ricerca. Nella traccia "pos" e' il passo s, i dati hanno
// "ordine" e gli orari sono in quell'ordine; le funzioni traccia_* ricevono s.
void traccia_imposta_ordine(const int *cella_di, const int *indice_di);
// Un controllo in piu' per ogni coppia (jt) e per ogni docente da solo (jt = NULL), dopo quelli di main: scrive i suoi
// ingressi nel file della traccia tra traccia_inizia_controllo e traccia_fine_controllo e ritorna la sua uscita.
typedef int (*traccia_controllo_fn)(int pos, int giorno, int ora, int cla, int base2d_ci, int base3d_goi,
                                    QueueItem *it, QueueItem *jt, int *fermato_a, int k);
extern traccia_controllo_fn traccia_controllo_extra;
void traccia_inizia_controllo(const char *nome);
void traccia_fine_controllo(int uscita, int *fermato_a, int k);
FILE *traccia_file(void);
// Un solver che riempie i giorni in un ordine che cambia durante la ricerca (giorni-liberi): traccia_giorni[r] e' il
// giorno riempito per r-esimo, traccia_posti[g] il posto del giorno g (NULL: calendario). Ogni backtrack ha in piu'
// "giro", "fissati" (quanti giorni all'inizio dell'ordine sono fissati) e "giorni" (l'ordine): "pos" e' il passo in
// quell'ordine. La regola dell'ultimo giorno e le ore obbligatorie contano i giorni in quell'ordine; gli orari trovati
// sono in ordine di calendario (come in main).
extern const int *traccia_giorni, *traccia_posti;
// Un solver che dentro ogni ora decide le classi in un ordine che cambia durante la ricerca (ordini-classi):
// traccia_classi[k] e' la classe decisa per k-esima in ogni ora (NULL: in ordine di indice). Va con traccia_giorni:
// ogni backtrack ha in piu' "classi" (l'ordine), e "pos" e' il passo in quell'ordine.
extern const int *traccia_classi;
extern int traccia_giro, traccia_fissati;
// All'inizio di ogni giro: si registrano di nuovo fino a max_backtrack backtrack (per ogni giro i primi).
void traccia_nuovo_giro(void);

void traccia_orario_trovato(void);

// Ogni tanto (dalla ricerca): scrive il progresso su stderr, se richiesto.
void traccia_progresso(int pos);

void traccia_fine(double secondi);

void traccia_errore(const char *messaggio);

void traccia_chiudi(void);

#endif
