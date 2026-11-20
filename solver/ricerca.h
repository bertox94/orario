#ifndef RICERCA_H
#define RICERCA_H

#include "scuola.h"

// La ricerca dell'orario.
//
// L'orario si riempie un'ora alla volta (lunedi' alla prima ora, poi alla seconda... fino all'ultima ora dell'ultimo
// giorno) e in ogni ora una classe alla volta. In ogni ora si decide per prima la classe che ha meno scelte: se una
// classe non ne ha nessuna lo si scopre subito, e una classe con una scelta sola la prende prima che un'altra gliela
// porti via. Le scelte di una classe sono le sue voci che in quell'ora si possono mettere, e l'ora buca quando e'
// ammessa; si provano mettendo per primi i docenti che hanno meno margine (meno ore libere in piu' di quelle che
// devono ancora fare), poi chi sta continuando le sue ore, poi le voci quasi finite. Se da un punto non esce nessun
// orario si torna indietro e si prova la scelta dopo.
//
// Per non perdersi in rami senza uscita, a ogni passo si guardano dei LIMITI (limiti.c): conti che devono tornare
// perche' un orario possa ancora esistere. Se uno non torna si torna indietro subito.
//
// La ricerca procede a TENTATIVI. Spesso ci si incaglia lontano dalla causa: un errore di lunedi' si paga sabato, e
// tornando indietro una cella alla volta ci vorrebbe troppo per arrivare a correggerlo. Percio' ogni tentativo ha un
// numero di nodi a disposizione: se li finisce si riparte da capo, con l'ordine delle scelte un po' rimescolato. Ogni
// tentativo ne ha la meta' in piu' del precedente, quindi se un orario non c'e' prima o poi un tentativo arriva in
// fondo e lo dimostra. Il rimescolamento dipende solo dal numero del tentativo: lo stesso caso da' sempre la stessa
// ricerca.

#define NODI_DEL_PRIMO_TENTATIVO 1000

#define VUOTA (-2)  // una cella non ancora decisa
#define BUCA (-1)   // un'ora buca

typedef enum { IN_CORSO, TROVATO, IMPOSSIBILE, LIMITE_DI_NODI } Esito;

// Perche' da un nodo non si va avanti: quale conto non torna, per chi, e i suoi due numeri.
typedef enum {
    NESSUNO,
    ORE_DOCENTE,            // al docente restano `servono` ore, ma ha solo `possibili` ore libere
    ORE_DOCENTE_IN_CLASSE,  // al docente restano `servono` ore nella classe, ma ne puo' fare solo `possibili`
    ORE_CLASSE,             // alla classe restano `servono` ore, ma ha solo `possibili` celle
    PRIME_ORE,              // la classe ha `servono` celle da riempire fino all'ora `ora`, le lezioni possibili sono
                            // `possibili`
    DOCENTE_SENZA_POSTO,    // il docente deve insegnare a quest'ora (gli restano `servono` ore e dopo ne ha libere
                            // `possibili`), ma nessuna classe lo puo' prendere
    CLASSE_SENZA_SCELTE,    // per la classe, a quest'ora, non c'e' niente che si possa mettere
    ORARIO_DELLA_CLASSE     // la classe, anche da sola, non ha piu' un orario per le `servono` ore che le restano
} TipoMotivo;

typedef struct {
    TipoMotivo tipo;
    int docente, classe, ora;  // di chi si parla (-1: non c'entra)
    int servono, possibili;
} Motivo;

// Una risposta gia' data sull'orario di una classe da sola (classe_sola.c): le due impronte dello stato della classe
// (64 bit l'una, calcolate in due modi: la risposta vale se coincidono tutte e due) e se un orario c'e'.
typedef struct {
    unsigned long long a, b;
    int ce;
} Ricordo;
#define RICORDI (1 << 18)  // quante se ne tengono: una nuova con lo stesso posto prende il posto della vecchia

// aggiunge un numero alle due impronte di uno stato (si parte da IMPRONTA_A e IMPRONTA_B)
#define IMPRONTA_A 1469598103934665603ULL
#define IMPRONTA_B 0x9E3779B97F4A7C15ULL
static inline void impronta(unsigned long long *a, unsigned long long *b, const int x) {
    *a = (*a ^ (unsigned long long) x) * 1099511628211ULL;
    *b = (*b + (unsigned long long) x + 1) * 0xD6E8FEB86659FD93ULL;
    *b ^= *b >> 32;
}

typedef struct Ricerca Ricerca;
struct Ricerca {
    const Scuola *s;
    int celle;    // giorni * ore * classi
    int *orario;  // per ogni cella una voce, BUCA o VUOTA; la cella di (giorno, ora, classe) e' cella_di()

    // quello che resta da mettere
    int *resto_voce, *resto_classe, *resto_docente;
    int *resto_in_classe;  // del docente t nella classe k: [k * docenti + t]

    // quello che e' gia' nell'orario, come serve alle regole
    unsigned char *occupato;     // il docente insegna in quell'ora: [(giorno * ore + ora) * docenti + t]
    unsigned char *ore_oggi;     // le ore del docente nella classe in un giorno: [(giorno * classi + k) * docenti + t]
    unsigned char *classi_oggi;  // in quante classi insegna in un giorno: [giorno * docenti + t]
    unsigned char *chiusa;       // la classe quel giorno ha gia' avuto un'ora buca: [giorno * classi + k]

    // per ogni classe i suoi docenti, per ogni docente le sue classi (non cambiano)
    int *docente_di, *n_docenti_di;  // [k * docenti + i], [k]
    int *classe_di, *n_classi_di;    // [t * classi + i], [t]
    int max_scelte;                  // il massimo di scelte di una classe: le sue voci, piu' l'ora buca
    int *posto;                      // il posto dei due docenti di una voce tra quelli della sua classe: [2 * v + i]
    Ricordo *ricordo;                // le risposte gia' date sull'orario delle classi da sole

    // calcolati all'inizio di ogni ora, per i nodi di quell'ora: [(giorno * ore + ora) * docenti + t]
    unsigned char *deve;  // il docente deve insegnare in quest'ora, o le sue ore non ci stanno piu'
    int *margine;         // le ore libere che ha in piu' di quelle che deve ancora fare
    long long *visto;     // a quale nodo il docente e' comparso tra le scelte di una classe: [t]

    Esito esito;
    long long nodi, limite;  // i nodi visitati (in tutti i tentativi); dopo `limite` nodi la ricerca si ferma (0: mai)
    int tentativo;           // quale tentativo e' (da 0)
    long long fine_del_tentativo;  // a quanti nodi il tentativo finisce
    int riparti;             // il tentativo e' finito: si esce da tutti i nodi togliendo le scelte, e si riparte
    int profondita_massima;  // il massimo di celle decise insieme
    Motivo motivo_radice;    // se non c'e' nessun orario gia' in partenza: il conto che non torna

    // se c'e', si chiama a ogni nodo: `profondita` celle decise, l'ultima e' `cella` con dentro `voce` (alla radice,
    // cioe' all'inizio di ogni tentativo, -1 e VUOTA); motivo->tipo dice se da qui non si va avanti, e perche'
    void (*a_ogni_nodo)(const Ricerca *r, int profondita, int cella, int voce, const Motivo *motivo);
    void *di_chi;
};

Ricerca *nuova_ricerca(const Scuola *s);
void libera_ricerca(Ricerca *r);

// Cerca un orario: alla fine r->esito e' TROVATO (l'orario e' in r->orario), IMPOSSIBILE o LIMITE_DI_NODI.
void cerca_orario(Ricerca *r);

// ---- le celle e le regole di un docente, per ricerca.c e limiti.c ----

static inline int cella_di(const Ricerca *r, const int giorno, const int ora, const int classe) {
    return (giorno * r->s->ore + ora) * r->s->classi + classe;
}

// il docente t e' nella classe k in quell'ora
static inline int presente(const Ricerca *r, const int giorno, const int ora, const int k, const int t) {
    const int v = r->orario[cella_di(r, giorno, ora, k)];
    return v >= 0 && nella_voce(&r->s->voce[v], t);
}

// Quante ore il docente t puo' ancora fare oggi nella classe k tra l'ora `da` (la prima della classe non ancora
// decisa) e l'ora `fino_a`, comprese. Niente se la classe ha gia' avuto un'ora buca. Se oggi ci ha gia' insegnato,
// solo continuando subito (c'era nell'ora prima, e adesso non e' altrove) fino al massimo di ore di seguito. Se no,
// dal suo ingresso (dall'ora dopo, se adesso e' in un'altra classe), e solo se oggi non e' gia' in troppe classi.
int ancora_oggi(const Ricerca *r, int giorno, int da, int fino_a, int k, int t);

// I limiti (limiti.c): 1 se i conti tornano; se no 0, con il perche' in *motivo.
// All'inizio di un'ora, per tutti i docenti e tutte le classi; prepara anche r->deve e r->margine per quell'ora.
int limiti_a_inizio_ora(Ricerca *r, int giorno, int ora, Motivo *motivo);
// Dopo una scelta, per quello che la scelta ha cambiato: la classe della cella e le altre classi dei docenti messi.
int limiti_dopo_la_scelta(const Ricerca *r, int cella, Motivo *motivo);
// le ore in cui il docente t e' a scuola dall'ora `da` di quel giorno alla fine della settimana
int ore_libere_da(const Ricerca *r, int giorno, int da, int t);
// La classe k, guardata da sola, ha ancora un orario per le ore che le restano (classe_sola.c): 1 se c'e' o non si
// riesce a dirlo in poco tempo, 0 se non c'e'.
int la_classe_ha_un_orario(const Ricerca *r, int giorno, int ora, int k);

#endif
