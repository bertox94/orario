#include "ricerca.h"

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

#define TENTATIVI 2000
#define MAI (-9)  // l'ultima ora di un docente che oggi non ha ancora insegnato

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

// passa al giorno dopo (i docenti ripartono da zero); se di li' non si riesce, il giorno torna com'era
static int giorno_dopo(ClasseSola *c, const int giorno) {
    int fatte[c->n_docenti], ultima[c->n_docenti];
    memcpy(fatte, c->fatte, sizeof fatte);
    memcpy(ultima, c->ultima, sizeof ultima);
    for (int i = 0; i < c->n_docenti; i++) c->fatte[i] = 0, c->ultima[i] = MAI;
    if (riempi(c, giorno + 1, 0)) return 1;
    memcpy(c->fatte, fatte, sizeof fatte);
    memcpy(c->ultima, ultima, sizeof ultima);
    return 0;
}

// il docente al posto i puo' insegnare in quest'ora
static int puo(const ClasseSola *c, const int i, const int ora) {
    const int t = c->r->docente_di[c->k * c->r->s->docenti + i];
    return c->r->s->ingresso[t] <= ora && c->fatte[i] < MAX_ORE_DI_SEGUITO && (!c->fatte[i] || c->ultima[i] == ora - 1);
}

// Riempie le celle della classe da (giorno, ora) in poi: 1 se ci riesce, o se i tentativi sono finiti.
static int riempi(ClasseSola *c, const int giorno, const int ora) {
    const Scuola *s = c->r->s;
    if (giorno == s->giorni) return c->resto_classe == 0;
    if (ora == s->ore) return giorno_dopo(c, giorno);
    if (--c->tentativi < 0) return 1;
    const int dopo = s->giorni - 1 - giorno, obbligatoria = ora < s->obbligatorie;
    if (obbligatoria || c->resto_classe > dopo * s->obbligatorie)
        for (int v = 0; v < c->n_voci; v++) {
            const int a = c->posto[2 * v], b = c->posto[2 * v + 1];
            if (!c->resto[v] || !puo(c, a, ora) || (b >= 0 && !puo(c, b, ora))) continue;
            const int prima_a = c->ultima[a], prima_b = b >= 0 ? c->ultima[b] : MAI;
            c->resto[v]--, c->resto_classe--, c->fatte[a]++, c->ultima[a] = ora;
            if (b >= 0) c->fatte[b]++, c->ultima[b] = ora;
            if (riempi(c, giorno, ora + 1)) return 1;
            c->resto[v]++, c->resto_classe++, c->fatte[a]--, c->ultima[a] = prima_a;
            if (b >= 0) c->fatte[b]--, c->ultima[b] = prima_b;
        }
    return !obbligatoria && c->resto_classe <= dopo * s->ore && giorno_dopo(c, giorno);
}

int la_classe_ha_un_orario(const Ricerca *r, const int giorno, const int ora, const int k) {
    const Scuola *s = r->s;
    const int n_voci = s->prima_voce[k + 1] - s->prima_voce[k], n_docenti = r->n_docenti_di[k];
    int resto[n_voci], fatte[n_docenti], ultima[n_docenti];
    ClasseSola c = {r, k, n_voci, n_docenti, &s->voce[s->prima_voce[k]], &r->posto[2 * s->prima_voce[k]], resto,
                    fatte, ultima, r->resto_classe[k], TENTATIVI};
    for (int v = 0; v < n_voci; v++) resto[v] = r->resto_voce[s->prima_voce[k] + v];
    // da dove riparte: oggi dalla sua prima ora non decisa, con le ore che i docenti hanno gia' fatto; oppure, se la
    // sua giornata e' finita, dal giorno dopo
    int g = giorno, h = ora + (r->orario[cella_di(r, giorno, ora, k)] != VUOTA);
    const int finita = h >= s->ore || r->chiusa[giorno * s->classi + k];
    if (finita) g++, h = 0;
    for (int i = 0; i < n_docenti; i++) {
        const int t = r->docente_di[k * s->docenti + i];
        fatte[i] = finita ? 0 : r->ore_oggi[(giorno * s->classi + k) * s->docenti + t];
        ultima[i] = !finita && h > 0 && presente(r, giorno, h - 1, k, t) ? h - 1 : MAI;
    }
    // lo stato in due impronte
    unsigned long long a = IMPRONTA_A, b = IMPRONTA_B;
    impronta(&a, &b, k), impronta(&a, &b, g * s->ore + h);
    for (int v = 0; v < n_voci; v++) impronta(&a, &b, resto[v]);
    for (int i = 0; i < n_docenti; i++) impronta(&a, &b, fatte[i] << 1 | (ultima[i] != MAI));
    Ricordo *ricordo = &r->ricordo[a & (RICORDI - 1)];
    if (ricordo->a != a || ricordo->b != b) *ricordo = (Ricordo){a, b, riempi(&c, g, h)};
    return ricordo->ce;
}
