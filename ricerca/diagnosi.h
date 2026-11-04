#ifndef UNTITLED_DIAGNOSI_H
#define UNTITLED_DIAGNOSI_H

// Diagnosi della ricerca: non influisce sulla ricerca, serve a capire dove si blocca (marce indietro per slot,
// posizione piu' avanzata, motivi per cui i docenti vengono scartati) e a fermarla dopo un tempo dato.

#include <stdio.h>

#include "../data_initialization/database_initializers.h"
#include "../orario/orario.h"
#include "vincoli.h"

#define N_MOTIVI 8

// Contatori. Sono qui (e non nascosti nel .c) perche' i piccoli aggiornamenti per nodo sono static inline.
extern long long int nodi;                // chiamate della ricerca
extern long long int blocchi_per_pos[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ + 1];
extern long long int visite_per_pos[];    // quante volte la ricerca e' arrivata a ogni slot
extern int pos_max;                       // posizione piu' avanzata raggiunta
extern long long int nodi_a_pos_max;      // a quale nodo e' stata raggiunta
extern double limite_secondi;             // se > 0 la ricerca si ferma dopo tanti secondi
extern double inizio_ricerca;
extern int interrotto_per_tempo;
extern int interrotto_su_richiesta;       // arrivato SIGTERM (il pulsante «Ferma» del sito) o raggiunto il limite di passi (--ferma-ai-passi)
extern int errore_di_scrittura;           // non si e' piu' potuto scrivere il file degli orari (disco pieno, limite
                                          // del sistema alla dimensione dei file, gzip terminato)

double secondi_reali(void);

// Imposta il limite di tempo (0 = nessuno) e se registrare anche i motivi di scarto dei docenti.
void diagnosi_imposta(double secondi, int con_motivi);

// Con --progresso: circa ogni secondo una riga "PROGRESSO {json}" su stdout (vedi diagnosi.c).
void diagnosi_imposta_progresso(int attivo);

// Gestore di SIGTERM: chiede alla ricerca di fermarsi al prossimo controllo periodico.
void diagnosi_richiesta_di_arresto(int segnale);

// Controlli che costano troppo per farli a ogni nodo (tempo, errori di scrittura, arresto richiesto,
// progresso): si fanno ogni 65536 nodi. pos e' lo slot corrente. Ritorna 1 se la ricerca va interrotta.
int diagnosi_controllo_periodico(int pos);

// Da chiamare appena prima di cominciare la ricerca.
void diagnosi_avvia(void);

// Da chiamare all'ingresso di ogni nodo. Ritorna 1 se la ricerca va interrotta (tempo scaduto, fermata, errore di scrittura).
static inline int diagnosi_nuovo_nodo(const int pos) {
    nodi++;
    visite_per_pos[pos]++;
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

// Lo stato della ricerca all'ultimo blocco in uno slot (marcia indietro o ramo scartato), per la diagnosi nel JSON.
#define MAX_DOCENTI_BLOCCO 40

typedef struct {
    int ore_classe_fatte;   // ore della classe dello slot gia' in orario in quel momento
    int docenti_con_ore;    // marce indietro: docenti che avevano ancora ore da fare in quella classe
    int scelta[2];          // rami scartati: docente e codocente (-1) messi nello slot, cioe' la scelta scartata
    // marce indietro, con --motivi: ogni docente con ore nella classe, il suo motivo (vedi nome_motivo) e, se e' gia'
    // in un'altra classe alla stessa ora, quale (altrimenti -1)
    int n_docenti;
    int docenti[MAX_DOCENTI_BLOCCO][3];
} stato_blocco_t;

long long int diagnosi_visite_nello_slot(int pos);
// L'ultima marcia indietro nello slot (ore_classe_fatte < 0 se non ce ne sono state).
stato_blocco_t diagnosi_ultima_marcia(int pos);
// L'ultimo ramo scartato dopo lo slot per quel motivo, con i numeri della verifica (vincoli.h).
stato_blocco_t diagnosi_ultimo_scarto(int pos, int motivo_scarto, dettaglio_scarto_t *dettaglio);

// Un ramo scartato dalla potatura; pos e' l'ultimo slot deciso, quello la cui scelta ha reso il ramo senza soluzioni
// (o che per primo se ne accorge).
void diagnosi_ramo_scartato(int pos, int motivo);

// Un nodo in cui non si e' potuto scegliere nessun docente. base4d_queue/queue_size delimitano la coda dei docenti.
// `obbligato`: il docente che la potatura ha reso obbligatorio in questo slot (-1 se nessuno): gli altri non sono
// stati provati, e il motivo del loro scarto e' quello.
void diagnosi_marcia_indietro(const teaching_slot *slot, int pos, int base4d_queue, int queue_size, int obbligato);

// I contatori per slot (pos = posizione nell'orario) e per docente, per chi li vuole esportare (risultati_json.c).
// I motivi (per slot e per docente) si contano solo con --motivi: diagnosi_con_motivi() dice se sono validi.
long long int diagnosi_marce_nello_slot(int pos);
// I vicoli ciechi di uno slot, per la mappa: quante volte la ricerca ci e' passata senza riuscire a mettere nessun
// docente E senza poter lasciare l'ora buca, cioe' i punti in cui si e' davvero bloccata. Non conta i rami scartati
// dalla potatura (scelte errate gia' rilevate dal codice), le ore buche (li' la ricerca prosegue) ne' i ripassi in
// cui un docente c'era. Il passo passo del sito usa la stessa regola.
long long int diagnosi_blocchi_nello_slot(int pos);
long long int diagnosi_scartati_nello_slot(int pos, int motivo_scarto);
long long int diagnosi_motivo_nello_slot(int pos, int motivo);
long long int diagnosi_motivo_del_docente(int prof, int motivo);
int diagnosi_con_motivi(void);
const char *diagnosi_nome_motivo(int motivo);

// Scrive su f (una riga JSON) la coda dei docenti dello slot pos e, per ognuno, se e' utilizzabile o perche' no.
// `scarto`, se non NULL, e' un oggetto JSON aggiunto come campo "potatura": il nodo era un ramo scartato dalla potatura
// prima di provare i docenti, quindi la coda dello slot e' solo quella che ci sarebbe stata.
void diagnosi_analizza_slot(const teaching_slot *slot, int pos, int base4d_queue, int queue_size, FILE *f,
                            const char *scarto, int obbligato);

// Scrive su f i campi JSON (con la virgola davanti) del dettaglio di un ramo scartato per il motivo m (SCARTATO_*):
// le lezioni della classe (fino_ora, da_riempire, possibili, lezioni...) o le ore di un docente.
void diagnosi_scrivi_dettaglio_scarto(FILE *f, int motivo, const dettaglio_scarto_t *d);

// Con --motivi: scrive su f una riga "M d:m d:m ..." con, per l'ultima marcia indietro, il motivo m (indice di
// diagnosi_nome_motivo) per cui ogni docente d, che nella classe aveva ancora ore da fare, non si e' potuto mettere
// nello slot. Senza motivi scrive la sola "M".
void diagnosi_scrivi_motivi_ultima_marcia(FILE *f);
const char *diagnosi_nome_motivo_scarto(int motivo_scarto);

// Scrive la diagnosi su stdout.
void stampa_diagnosi(long long int n_backtrack);

#endif //UNTITLED_DIAGNOSI_H
