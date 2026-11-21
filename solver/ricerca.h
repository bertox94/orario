#ifndef RICERCA_H
#define RICERCA_H

#include <stdio.h>

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
// Le parole che tornano in tutto il codice:
//   cella       un posto dell'orario: una classe in un'ora di un giorno. Le celle sono numerate un'ora dopo l'altra,
//               e dentro l'ora una classe dopo l'altra: (giorno * ore + ora) * classi + classe (cella_di).
//   voce        una lezione di una classe: un docente da solo, o una coppia, con le sue ore (scuola.h)
//   nodo        un punto della ricerca: un orario riempito in parte. La radice e' l'orario vuoto; ogni scelta scende
//               di un nodo, ogni ritorno indietro risale.
//   profondita  quante celle sono decise in un nodo
//   resto       le ore ancora da mettere: di una voce, di una classe, di un docente
//   t, k, v     dove non c'e' spazio per un nome: un docente, una classe, una voce (i loro indici nella Scuola)

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
    int servono, possibili;    // i due numeri del conto: quello che serve e' piu' di quello che e' possibile
} Motivo;

// I segni di un conto che non torna: per ogni cella di cui il conto parla, che parte ha (per il sito).
enum {
    SEGNO_FATTA = 1,  // il docente del conto qui insegna gia'
    SEGNO_PUO,        // una cella che il conto conta tra le possibili
    SEGNO_NO,         // una cella che non conta: il docente non c'e', o non puo', o la classe ha finito
    SEGNO_SERVE       // una cella che va riempita per forza (le prime ore)
};

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
    long long *visto;     // l'ultima volta (il timbro) che il docente e' comparso tra le scelte di una classe: [t]
    long long timbro;     // cresce di uno a ogni scelta della classe: distingue una volta dall'altra

    Esito esito;
    long long nodi, limite;  // i nodi visitati; dopo `limite` nodi la ricerca si ferma (0: mai)
    int profondita_massima;  // il massimo di celle decise insieme
    // I ritorni indietro: i nodi da cui non si va avanti perche' un conto non torna. Quanti in tutto, e quanti per
    // cella: [cella] conta quelli arrivati subito dopo una scelta messa in quella cella. Solo numeri da mostrare.
    long long ritorni, *ritorni_in;
    // Gli stessi, ma solo dall'ultimo passo avanti: si azzerano tutti ogni volta che la ricerca arriva piu' avanti
    // di quanto sia mai arrivata (una profondita massima nuova). Una cella che ha fatto tanti ritorni e poi e' stata
    // superata non e' piu' il problema: qui torna a zero, e restano i ritorni del punto in cui si e' fermi adesso.
    long long *ritorni_recenti_in;
    Motivo motivo_radice;    // se non c'e' nessun orario gia' in partenza: il conto che non torna

    // se c'e', si chiama a ogni nodo: `profondita` celle decise, l'ultima e' `cella` con dentro `voce` (alla radice
    // -1 e VUOTA); motivo->tipo dice se da qui non si va avanti, e perche'
    void (*a_ogni_nodo)(const Ricerca *r, int profondita, int cella, int voce, const Motivo *motivo);
    void *di_chi;
};

// Prepara una ricerca per la scuola: l'orario vuoto e tutte le ore ancora da mettere. NULL se manca la memoria.
// La scuola deve restare dov'e' finche' la ricerca si usa. Si libera con libera_ricerca.
Ricerca *nuova_ricerca(const Scuola *s);
void libera_ricerca(Ricerca *r);

// Cerca un orario: alla fine r->esito e' TROVATO (l'orario e' in r->orario), IMPOSSIBILE o LIMITE_DI_NODI.
void cerca_orario(Ricerca *r);

// Riprende una ricerca che il limite di nodi aveva fermato, dal punto esatto in cui era, e va avanti fino al limite
// nuovo: il risultato e' lo stesso di una ricerca sola con quel limite. `strada` e' la strada dell'ultimo nodo
// visitato: `profondita` coppie (cella, voce) nell'ordine in cui le celle sono state decise. Prima di chiamarla r
// deve essere nuova, con r->nodi, r->ritorni, r->ritorni_in, r->ritorni_recenti_in e r->profondita_massima rimessi
// com'erano e r->limite nuovo. Ritorna 0 (senza fare niente di utile) se la strada non e' di questa ricerca.
int riprendi_la_ricerca(Ricerca *r, const int *strada, int profondita);

// Perche' un docente non puo' insegnare adesso in una classe (ricerca.c, perche_non_puo).
typedef enum {
    PUO,                // puo'
    NON_ENTRATO,        // la sua ora di ingresso e' dopo
    ALTROVE,            // in quest'ora e' gia' in un'altra classe
    TROPPE_CLASSI,      // oggi qui non ha ancora insegnato, ed e' gia' nel massimo di classi al giorno
    MASSIMO_RAGGIUNTO,  // oggi qui ha gia' fatto il massimo di ore
    INTERROTTO          // oggi qui ha gia' insegnato, ma nell'ora prima non c'era: le ore devono essere di seguito
} PercheNo;
PercheNo perche_non_puo(const Ricerca *r, int giorno, int ora, int k, int t);

// Mette nella cella la voce v o l'ora buca (verso = 1), oppure la toglie (verso = -1), e tiene tutti i conti. La usa
// la ricerca; serve anche a rimettere l'orario com'era a un nodo, una scelta dopo l'altra (main.c, i segni).
void segna(Ricerca *r, int cella, int v, int verso);

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
// Le celle di cui parla il motivo per cui dal nodo non si va avanti (limiti.c, in fondo): in `segni`, lungo r->celle,
// per ogni cella 0 o un SEGNO_. L'orario di r deve essere quello del nodo: `profondita` celle decise, l'ultima e'
// `cella` (-1 alla radice). Ritorna l'ora della settimana (giorno * ore + ora) a cui il conto e' stato fatto.
int segni_del_nodo(const Ricerca *r, int profondita, int cella, const Motivo *motivo, signed char *segni);
// I conti di ogni docente a un nodo, in JSON su `f` (limiti.c, in fondo): le ore in tutto e in ogni sua classe, che
// tornino o no. Per il sito, che li mostra docente per docente.
void scrivi_i_docenti(FILE *f, const Ricerca *r, int ora_della_settimana);
// Il conto del motivo in dettaglio, in JSON su `f` (limiti.c, in fondo): i pezzi da cui vengono i suoi due numeri.
// Come per i segni, l'orario di r deve essere quello del nodo, e l'ora e' quella che segni_del_nodo ritorna.
void scrivi_il_conto(FILE *f, const Ricerca *r, int ora_della_settimana, const Motivo *motivo);
// La classe k, guardata da sola, ha ancora un orario per le ore che le restano (classe_sola.c): 1 se c'e' o non si
// riesce a dirlo in poco tempo, 0 se non c'e'.
int la_classe_ha_un_orario(const Ricerca *r, int giorno, int ora, int k);

#endif
