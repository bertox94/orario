#ifndef SPIEGA_H
#define SPIEGA_H

#include <stdio.h>

#include "../solver/ricerca.h"

// Le spiegazioni: quello che c'e' da dire di un nodo della ricerca a chi lo guarda (il sito). La ricerca non le usa.
// Stanno a parte dai limiti, ma dicono la stessa cosa: i conti li chiedono alle stesse funzioni (solver/limiti.h).

// I segni di un conto che non torna: per ogni cella di cui il conto parla, che parte ha (per il sito).
enum {
    SEGNO_FATTA = 1,  // il docente del conto qui insegna gia'
    SEGNO_PUO,        // una cella che il conto conta tra le possibili
    SEGNO_NO,         // una cella che non conta: il docente non c'e', o non puo', o la classe ha finito
    SEGNO_SERVE       // una cella che va riempita per forza (le prime ore)
};

// Le celle di cui parla il motivo per cui dal nodo non si va avanti : in `segni`, lungo r->celle,
// per ogni cella 0 o un SEGNO_. L'orario di r deve essere quello del nodo: `profondita` celle decise, l'ultima e'
// `cella` (-1 alla radice). Ritorna l'ora della settimana (giorno * ore + ora) a cui il conto e' stato fatto.
int segni_del_nodo(const Ricerca *r, int profondita, int cella, const Motivo *motivo, signed char *segni);

// Il conto del motivo in dettaglio, in JSON su `f` : i pezzi da cui vengono i suoi due numeri.
// Come per i segni, l'orario di r deve essere quello del nodo, e l'ora e' quella che segni_del_nodo ritorna.
void scrivi_il_conto(FILE *f, const Ricerca *r, int ora_della_settimana, const Motivo *motivo);

// I conti di ogni docente a un nodo, in JSON su `f` : le ore in tutto e in ogni sua classe, che
// tornino o no. Per il sito, che li mostra docente per docente.
void scrivi_i_docenti(FILE *f, const Ricerca *r, int ora_della_settimana);

#endif
