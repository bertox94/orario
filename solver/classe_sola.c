#include "limiti.h"

#include <string.h>

// L'orario di una classe, da sola.
//
// Si guarda una classe come se le altre non ci fossero (i suoi docenti sempre liberi) e si prova a riempire le celle
// che le restano con le ore che restano alle sue voci, con le regole che riguardano solo lei: ogni docente dal suo
// ingresso, al massimo le ore di seguito consentite; lezione nelle prime ore; l'ora buca solo quando le ore che
// restano stanno nei giorni dopo, e da li' la giornata e' finita; alla fine nessuna ora avanzata. Se un orario cosi'
// non c'e', non c'e' nemmeno quello di tutta la scuola, che per questa classe sarebbe uno di quelli.
//
// E' una ricerca piccola dentro la ricerca. Se non finisce in TENTATIVI passi non si sa, e il limite passa. La
// risposta dipende solo dallo stato della classe (da dove riparte, le ore che restano alle voci, e per oggi le ore
// gia' fatte da ogni docente e se c'era nell'ora prima), che torna uguale moltissime volte mentre la ricerca cambia
// le altre classi: le risposte si ricordano (ricerca.h, Ricordo).

// Come e' fatta: `riempi` decide una cella della classe alla volta (una voce, o l'ora buca) e chiama se stessa per
// la cella dopo; `giorno_dopo` passa da un giorno al successivo. E' la stessa idea della ricerca grande, in piccolo:
// una classe sola, e solo la domanda «c'e' o non c'e'».

#define TENTATIVI 2000  // quante celle si provano al massimo per una risposta: oltre, non si sa e si lascia passare
#define MAI (-9)        // l'ultima ora di un docente che oggi non ha ancora insegnato (lontana da ogni ora vera)

// Lo stato della piccola ricerca. I docenti qui sono contati da 0 dentro la classe (il loro «posto»: r->posto), non
// con il loro indice nella scuola: cosi' le tabelle sono lunghe quanto i docenti della classe.

typedef struct {
    const Ricerca *r;
    int k, n_voci, n_docenti;
    const Voce *voce;    // le voci della classe
    const int *posto;    // per ogni voce, il posto dei suoi due docenti tra quelli della classe (-1: non c'e')
    int *resto;          // le ore che restano a ogni voce
    int *fatte, *ultima; // nel giorno che si sta riempiendo, per ogni docente: le ore fatte e l'ultima ora
    int resto_classe, tentativi;
} ClasseSola;

static int riempi(ClasseSola *c, int giorno, int ora);

// Passa al giorno dopo (i docenti ripartono da zero: nessuna ora fatta); se di li' non si riesce, il giorno torna
// com'era, perche' chi ha chiamato provera' un'altra scelta per oggi.
static int giorno_dopo(ClasseSola *c, const int giorno) {
    int fatte[c->n_docenti], ultima[c->n_docenti];  // la giornata di oggi, messa da parte
    memcpy(fatte, c->fatte, sizeof fatte);
    memcpy(ultima, c->ultima, sizeof ultima);
    for (int i = 0; i < c->n_docenti; i++) c->fatte[i] = 0, c->ultima[i] = MAI;
    if (riempi(c, giorno + 1, 0)) return 1;
    memcpy(c->fatte, fatte, sizeof fatte);
    memcpy(c->ultima, ultima, sizeof ultima);
    return 0;
}

// Il docente al posto i puo' insegnare in quest'ora: e' entrato, non ha gia' fatto il massimo, e se oggi ha gia'
// insegnato qui deve continuare di seguito (la sua ultima ora e' quella prima). Non si guarda se e' in un'altra
// classe: le altre classi qui non ci sono.
static int puo(const ClasseSola *c, const int i, const int ora) {
    const int t = c->r->docente_di[c->k * c->r->s->docenti + i];
    return c->r->s->ingresso[t] <= ora && c->fatte[i] < MAX_ORE_DI_SEGUITO && (!c->fatte[i] || c->ultima[i] == ora - 1);
}

// Riempie le celle della classe da (giorno, ora) in poi: 1 se ci riesce, o se i tentativi sono finiti.
static int riempi(ClasseSola *c, const int giorno, const int ora) {
    const Scuola *s = c->r->s;
    if (giorno == s->giorni) return c->resto_classe == 0;  // finita la settimana: ci si e' riusciti se non avanza niente
    if (ora == s->ore) return giorno_dopo(c, giorno);      // finita la giornata
    if (--c->tentativi < 0) return 1;                      // troppo lungo: non si sa, e nel dubbio si dice di si'
    const int dopo = s->giorni - 1 - giorno, obbligatoria = ora < s->obbligatorie;
    // Una lezione (le stesse condizioni della ricerca grande: scelte_della_classe): si prova ogni voce che ha ancora
    // ore e i cui docenti possono.
    if (obbligatoria || c->resto_classe > dopo * s->obbligatorie)
        for (int v = 0; v < c->n_voci; v++) {
            const int a = c->posto[2 * v], b = c->posto[2 * v + 1];  // i posti dei suoi docenti (b < 0: da solo)
            if (!c->resto[v] || !puo(c, a, ora) || (b >= 0 && !puo(c, b, ora))) continue;
            const int prima_a = c->ultima[a], prima_b = b >= 0 ? c->ultima[b] : MAI;  // per rimettere com'era
            // si mette la voce in quest'ora...
            c->resto[v]--, c->resto_classe--, c->fatte[a]++, c->ultima[a] = ora;
            if (b >= 0) c->fatte[b]++, c->ultima[b] = ora;
            // ...si va avanti, e se da li' si arriva in fondo un orario c'e'...
            if (riempi(c, giorno, ora + 1)) return 1;
            // ...se no la si toglie e si prova la voce dopo
            c->resto[v]++, c->resto_classe++, c->fatte[a]--, c->ultima[a] = prima_a;
            if (b >= 0) c->fatte[b]--, c->ultima[b] = prima_b;
        }
    // L'ora buca: non nelle prime ore, e solo se le ore che restano stanno nei giorni dopo. Dopo un'ora buca la
    // giornata e' finita, quindi si passa direttamente al giorno dopo.
    return !obbligatoria && c->resto_classe <= dopo * s->ore && giorno_dopo(c, giorno);
}

// La domanda, per la classe k al punto in cui e' la ricerca: prepara lo stato della classe, guarda se la risposta
// per quello stato e' gia' stata data, e se no la cerca con `riempi` e la ricorda.
int la_classe_ha_un_orario(const Ricerca *r, const int giorno, const int ora, const int k) {
    const Scuola *s = r->s;
    const int n_voci = s->prima_voce[k + 1] - s->prima_voce[k], n_docenti = r->n_docenti_di[k];
    int resto[n_voci], fatte[n_docenti], ultima[n_docenti];
    // lo stato della piccola ricerca: le sue tabelle sono qui sopra, sullo stack, lunghe quanto serve a questa classe
    ClasseSola c = {r, k, n_voci, n_docenti, &s->voce[s->prima_voce[k]], &r->posto[2 * s->prima_voce[k]], resto,
                    fatte, ultima, r->resto_classe[k], TENTATIVI};
    // le ore che restano alle sue voci (una copia: `riempi` le cambia mentre prova)
    for (int v = 0; v < n_voci; v++) resto[v] = r->resto_voce[s->prima_voce[k] + v];
    // da dove riparte: oggi dalla sua prima ora non decisa, con le ore che i docenti hanno gia' fatto; oppure, se la
    // sua giornata e' finita, dal giorno dopo
    int g = giorno, h = ora + (r->orario[cella_di(r, giorno, ora, k)] != VUOTA);
    // la classe oggi ha finito se non ha piu' ore libere, o se ha gia' avuto un'ora buca
    const int finita = h >= s->ore || r->chiusa[giorno * s->classi + k];
    if (finita) g++, h = 0;
    for (int i = 0; i < n_docenti; i++) {
        const int t = r->docente_di[k * s->docenti + i];
        // per ogni suo docente: le ore fatte oggi qui, e se c'era nell'ora prima della prima libera
        fatte[i] = finita ? 0 : r->ore_oggi[(giorno * s->classi + k) * s->docenti + t];
        ultima[i] = !finita && h > 0 && presente(r, giorno, h - 1, k, t) ? h - 1 : MAI;
    }
    // Lo stato in due impronte: tutto quello da cui la risposta dipende (la classe, da dove riparte, le ore che
    // restano a ogni voce, e per ogni docente le ore fatte oggi e se c'era nell'ora prima) passa in due numeri di 64
    // bit, calcolati in due modi diversi.
    unsigned long long a = IMPRONTA_A, b = IMPRONTA_B;
    impronta(&a, &b, k), impronta(&a, &b, g * s->ore + h);
    for (int v = 0; v < n_voci; v++) impronta(&a, &b, resto[v]);
    for (int i = 0; i < n_docenti; i++) impronta(&a, &b, fatte[i] << 1 | (ultima[i] != MAI));
    // Il posto della risposta nella tabella lo danno gli ultimi bit della prima impronta. Se li' c'e' la risposta
    // per questo stato (tutte e due le impronte uguali) e' quella; se no si cerca adesso e la si scrive li', al posto
    // di quella che c'era.
    Ricordo *ricordo = &r->ricordo[a & (RICORDI - 1)];
    if (ricordo->a != a || ricordo->b != b) *ricordo = (Ricordo){a, b, riempi(&c, g, h)};
    return ricordo->ce;
}
