#ifndef UNTITLED_DIAGNOSI_H
#define UNTITLED_DIAGNOSI_H

// Diagnosi della ricerca: non influisce sulla ricerca, serve a capire dove si blocca (marce indietro per slot,
// posizione piu' avanzata, motivi per cui i docenti vengono scartati) e a fermarla dopo un tempo dato.

#include "../data_initialization/database_initializers.h"
#include "../orario/orario.h"

#define N_MOTIVI 7

// Contatori. Sono qui (e non nascosti nel .c) perche' i piccoli aggiornamenti per nodo sono static inline.
extern long long int nodi;                // chiamate della ricerca
extern int pos_max;                       // posizione piu' avanzata raggiunta
extern long long int nodi_a_pos_max;      // a quale nodo e' stata raggiunta
extern double limite_secondi;             // se > 0 la ricerca si ferma dopo tanti secondi
extern double inizio_ricerca;
extern int interrotto_per_tempo;

double secondi_reali(void);

// Imposta il limite di tempo (0 = nessuno) e se registrare anche i motivi di scarto dei docenti.
void diagnosi_imposta(double secondi, int con_motivi);

// Da chiamare appena prima di cominciare la ricerca.
void diagnosi_avvia(void);

// Da chiamare all'ingresso di ogni nodo. Ritorna 1 se la ricerca va interrotta per il tempo scaduto.
static inline int diagnosi_nuovo_nodo(const int pos) {
    nodi++;
    if (pos > pos_max) {
        pos_max = pos;
        nodi_a_pos_max = nodi;
    }
    if (limite_secondi > 0 && (nodi & 0xFFFF) == 0 && secondi_reali() - inizio_ricerca > limite_secondi) {
        interrotto_per_tempo = 1;
        return 1;
    }
    return 0;
}

// Perche' un ramo viene scartato (potatura) prima di arrivare a una marcia indietro.
enum {
    SCARTATO_LEZIONI_DELLA_CLASSE, // alla classe non bastano piu' le lezioni per gli slot da riempire
    SCARTATO_ORE_DEI_DOCENTI,      // le ore che mancano a un docente non stanno piu' nel tempo rimasto
    N_MOTIVI_SCARTO
};

// Un ramo scartato dalla potatura; pos e' l'ultimo slot deciso, quello la cui scelta ha reso il ramo senza soluzioni
// (o che per primo se ne accorge).
void diagnosi_ramo_scartato(int pos, int motivo);

// Un nodo in cui non si e' potuto scegliere nessun docente. base4d_queue/queue_size delimitano la coda dei docenti.
void diagnosi_marcia_indietro(const teaching_slot *slot, int pos, int base4d_queue, int queue_size);

// Scrive la diagnosi su stdout.
void stampa_diagnosi(long long int n_backtrack);

#endif //UNTITLED_DIAGNOSI_H
