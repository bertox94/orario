#include "passi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Passi {
    FILE *passi, *istantanee;
    int16_t *strada;  // la strada del nodo dove si e': strada[0] la profondita, poi le coppie (cella, voce)
    int celle;
};

// Aggiunge un nodo: il suo record nel file dei passi e, ogni PASSI_ISTANTANEA nodi, tutta la sua strada nel file
// delle istantanee. La strada si tiene aggiornata a ogni nodo: la scelta di questo nodo va al posto numero
// `profondita`, e quelle dopo non contano piu' (si e' tornati indietro, o non sono ancora state fatte).
void passi_nodo(Passi *p, const long long nodo, const int profondita, const int cella, const int voce,
                const Motivo *m) {
    const int16_t record[PASSI_CAMPI] = {(int16_t) profondita, (int16_t) cella, (int16_t) voce, (int16_t) m->tipo,
                                         (int16_t) m->docente, (int16_t) m->classe, (int16_t) m->ora,
                                         (int16_t) m->servono, (int16_t) m->possibili};
    fwrite(record, sizeof record, 1, p->passi);
    p->strada[0] = (int16_t) profondita;
    if (profondita) p->strada[2 * profondita - 1] = (int16_t) cella, p->strada[2 * profondita] = (int16_t) voce;
    if (nodo % PASSI_ISTANTANEA) return;  // non e' il momento dell'istantanea
    fwrite(p->strada, sizeof(int16_t), 1 + 2 * p->celle, p->istantanee);
    fflush(p->passi);  // il sito legge mentre la ricerca va avanti: fino a qui tutto scritto
    fflush(p->istantanee);
}

// Apre il file dei passi e, accanto, quello delle istantanee (lo stesso nome con .ist in fondo).
Passi *passi_apri(const char *percorso, const int celle) {
    char istantanee[1024];
    snprintf(istantanee, sizeof istantanee, "%s.ist", percorso);
    Passi *p = calloc(1, sizeof(Passi));
    if (!p) return NULL;
    p->passi = fopen(percorso, "wb");
    p->istantanee = fopen(istantanee, "wb");
    p->strada = calloc(1 + 2 * celle, sizeof(int16_t));
    p->celle = celle;
    if (!p->passi || !p->istantanee || !p->strada) {
        passi_chiudi(p);
        return NULL;
    }
    return p;
}

void passi_chiudi(Passi *p) {
    if (!p) return;
    if (p->passi) fclose(p->passi);
    if (p->istantanee) fclose(p->istantanee);
    free(p->strada);
    free(p);
}
