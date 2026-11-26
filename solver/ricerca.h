#ifndef RICERCA_H
#define RICERCA_H

#include "../scuola/scuola.h"

// La ricerca dell'orario.
//
// L'orario si riempie un'ora alla volta (lunedi' alla prima ora, poi alla seconda... fino all'ultima ora dell'ultimo
// giorno) e in ogni ora una classe alla volta. In ogni ora si decide per prima la classe che ha meno scelte: se una
// classe non ne ha nessuna lo si scopre subito, e una classe con una scelta sola la prende prima che un'altra gliela
// porti via. Le scelte di una classe sono le sue voci che in quell'ora si possono mettere, e l'ora buca quando e'
// ammessa (la Buca: piu' sotto, resto_buca); si provano mettendo per primi i docenti che hanno meno margine (meno ore libere in piu' di quelle che
// devono ancora fare), poi chi sta continuando le sue ore, poi le voci quasi finite. L'ora buca e' in coda con il
// suo margine, come un docente. Se da un punto non esce nessun
// orario si torna indietro e si prova la scelta dopo.
//
// Per non perdersi in rami senza uscita, a ogni passo si guardano dei LIMITI (limiti.h): conti che devono tornare
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

    // La Buca. Le ore buche di una classe si trattano come le ore di un docente tutto suo, la Buca di quella classe
    // (richiesta di Alb, 26 novembre 2026): ha le ore che deve fare in una settimana (le celle della classe meno le
    // sue lezioni), «entra» dopo le ore obbligatorie, e quando in un giorno comincia va avanti fino all'ultima ora.
    // Cosi' per le ore buche valgono gli stessi conti dei docenti: i posti dove puo' ancora andare, il margine, se
    // deve cominciare adesso (qui sotto: posti_della_buca, la_buca_deve, la_buca_puo).
    int *resto_buca;  // le ore buche che la classe deve ancora avere: [k]

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
    // E per colpa di chi: i ritorni contati per controllo e per il docente (o, se il controllo non parla di un
    // docente, la classe) che il motivo nomina, in tutto e dall'ultimo passo avanti. Il posto e' posto_del_motivo().
    // Un ritorno si conta sulla cella dell'ultima scelta, che spesso non e' il problema: il motivo dice chi lo e'.
    long long *ritorni_per, *ritorni_recenti_per;
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
// deve essere nuova, con r->nodi, r->profondita_massima e tutti i conti dei ritorni rimessi com'erano, e r->limite
// nuovo. Ritorna 0 (senza fare niente di utile) se la strada non e' di questa ricerca.
int riprendi_la_ricerca(Ricerca *r, const int *strada, int profondita);

// Una ricerca che parte da una strada scelta a mano: `profondita` coppie (cella, voce), almeno una. Percorre la
// strada (ogni scelta deve essere una di quelle che la ricerca avrebbe avuto in quel punto) e da li' cerca come
// sempre; quando sotto l'ultima scelta della strada non c'e' niente, va avanti con le scelte che la ricerca avrebbe
// provato DOPO quelle della strada, risalendo. Quelle che avrebbe provato PRIMA non le prova: per questo qui
// IMPOSSIBILE vuol dire solo «nessun orario nella parte cercata», non che un orario non esiste. r deve essere nuova.
// Ritorna 0 se la strada non e' una strada possibile.
int cerca_da_una_strada(Ricerca *r, const int *strada, int profondita);

// Le scelte che la ricerca ha nel nodo in fondo a una strada (`profondita` coppie, anche nessuna): in `scelte`
// (lungo r->max_scelte) le voci, o BUCA, nell'ordine in cui le proverebbe, e in *cella la cella che quel nodo
// decide. Ritorna quante sono (0 se da li' non si va avanti), o -1 se la strada non e' possibile. r deve essere
// nuova, e dopo non serve piu' ad altro.
int scelte_dopo_la_strada(Ricerca *r, const int *strada, int profondita, int *cella, int *scelte);

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

// Quanti posti hanno le tabelle dei ritorni per motivo, e il posto di un motivo: una riga per tipo, e in ogni riga
// prima i docenti e poi le classi.
static inline int posti_dei_motivi(const Scuola *s) { return (ORARIO_DELLA_CLASSE + 1) * (s->docenti + s->classi); }
static inline int posto_del_motivo(const Scuola *s, const Motivo *m) {
    return m->tipo * (s->docenti + s->classi) + (m->docente >= 0 ? m->docente : s->docenti + MAX(m->classe, 0));
}

// ---- la Buca di una classe (vedi sopra, resto_buca) ----

// I posti dove la Buca puo' ancora andare dall'ora `da` di quel giorno alla fine della settimana: le ore dopo le
// obbligatorie. E' lo stesso conto delle ore libere di un docente (ore_libere_da), con le obbligatorie come ingresso.
static inline int posti_della_buca(const Ricerca *r, const int giorno, const int da) {
    const Scuola *s = r->s;
    return (s->giorni - 1 - giorno) * (s->ore - s->obbligatorie) + MAX(0, s->ore - MAX(da, s->obbligatorie));
}
// La Buca della classe k deve cominciare in quest'ora: se la classe fa lezione anche adesso, le ore buche che le
// restano non stanno piu' nei posti dall'ora dopo in poi.
static inline int la_buca_deve(const Ricerca *r, const int giorno, const int ora, const int k) {
    return r->resto_buca[k] > posti_della_buca(r, giorno, ora + 1);
}
// La Buca della classe k puo' stare in quest'ora: e' «entrata» (non e' un'ora obbligatoria) e ha abbastanza ore per
// arrivare alla fine della giornata, perche' dopo un'ora buca la classe quel giorno non ha piu' lezione.
static inline int la_buca_puo(const Ricerca *r, const int ora, const int k) {
    return ora >= r->s->obbligatorie && r->resto_buca[k] >= r->s->ore - ora;
}

// Mette nella cella la voce v o l'ora buca (verso = 1), oppure la toglie (verso = -1), e tiene tutti i conti. La usa
// la ricerca; serve anche a rimettere l'orario com'era a un nodo, una scelta dopo l'altra (main.c, i segni).
void segna(Ricerca *r, int cella, int v, int verso);

// ---- le celle e le regole di un docente, per la ricerca, i limiti e le spiegazioni ----

static inline int cella_di(const Ricerca *r, const int giorno, const int ora, const int classe) {
    return (giorno * r->s->ore + ora) * r->s->classi + classe;
}

// il docente t e' nella classe k in quell'ora
static inline int presente(const Ricerca *r, const int giorno, const int ora, const int k, const int t) {
    const int v = r->orario[cella_di(r, giorno, ora, k)];
    return v >= 0 && nella_voce(&r->s->voce[v], t);
}

#endif
