#ifndef UNTITLED_DIAGNOSI_H
#define UNTITLED_DIAGNOSI_H

// Diagnosi della ricerca: non influisce sulla ricerca, serve a capire dove si blocca (marce indietro per slot,
// posizione piu' avanzata, motivi per cui i docenti vengono scartati) e a fermarla dopo un tempo dato.

#include <stdio.h>

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
extern int interrotto_per_spazio;         // il file degli orari ha superato il limite di --max-mb
extern int interrotto_su_richiesta;       // arrivato SIGTERM (il pulsante "Ferma" del sito)
extern int errore_di_scrittura;           // non si e' piu' potuto scrivere il file degli orari (disco pieno, limite
                                          // del sistema alla dimensione dei file, gzip terminato)

double secondi_reali(void);

// Imposta il limite di tempo (0 = nessuno) e se registrare anche i motivi di scarto dei docenti.
void diagnosi_imposta(double secondi, int con_motivi);

// Imposta il limite di spazio: la ricerca si ferma quando il file `file` supera `mb` MB sul disco (0 = nessun limite).
// Per un file .gz conta la dimensione compressa, cioe' quella che occupa davvero.
void diagnosi_imposta_spazio(const char *file, double mb);

// Con --progresso: circa ogni secondo una riga "PROGRESSO {json}" su stdout (vedi diagnosi.c).
void diagnosi_imposta_progresso(int attivo);

// Gestore di SIGTERM: chiede alla ricerca di fermarsi al prossimo controllo periodico.
void diagnosi_richiesta_di_arresto(int segnale);

// Controlli che costano troppo per farli a ogni nodo (tempo, spazio, errori di scrittura, arresto richiesto,
// progresso): si fanno ogni 65536 nodi. pos e' lo slot corrente. Ritorna 1 se la ricerca va interrotta.
int diagnosi_controllo_periodico(int pos);

// Da chiamare appena prima di cominciare la ricerca.
void diagnosi_avvia(void);

// Da chiamare all'ingresso di ogni nodo. Ritorna 1 se la ricerca va interrotta (tempo scaduto o spazio finito).
static inline int diagnosi_nuovo_nodo(const int pos) {
    nodi++;
    if (pos > pos_max) {
        pos_max = pos;
        nodi_a_pos_max = nodi;
    }
    if ((nodi & 0xFFFF) == 0) return diagnosi_controllo_periodico(pos);
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

// I contatori per slot (pos = posizione nell'orario) e per docente, per chi li vuole esportare (risultati_json.c).
// I motivi (per slot e per docente) si contano solo con --motivi: diagnosi_con_motivi() dice se sono validi.
long long int diagnosi_marce_nello_slot(int pos);
long long int diagnosi_scartati_nello_slot(int pos, int motivo_scarto);
long long int diagnosi_motivo_nello_slot(int pos, int motivo);
long long int diagnosi_motivo_del_docente(int prof, int motivo);
int diagnosi_con_motivi(void);
const char *diagnosi_nome_motivo(int motivo);

// Con --motivi: scrive su f una riga "M d:m d:m ..." con, per l'ultima marcia indietro, il motivo m (indice di
// diagnosi_nome_motivo) per cui ogni docente d, che nella classe aveva ancora ore da fare, non si e' potuto mettere
// nello slot. Senza motivi scrive la sola "M".
void diagnosi_scrivi_motivi_ultima_marcia(FILE *f);
const char *diagnosi_nome_motivo_scarto(int motivo_scarto);

// Scrive la diagnosi su stdout.
void stampa_diagnosi(long long int n_backtrack);

#endif //UNTITLED_DIAGNOSI_H
