#include "passi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

struct Passi {
    FILE *passi, *istantanee, *profilo;
    int16_t bassa, alta;  // la profondita piu' bassa e la piu' alta del blocco di nodi che si sta riempiendo
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
    // il profilo: il primo nodo di un blocco comincia il conto, l'ultimo lo scrive
    if (nodo % PASSI_BLOCCO == 1 || profondita < p->bassa) p->bassa = (int16_t) profondita;
    if (nodo % PASSI_BLOCCO == 1 || profondita > p->alta) p->alta = (int16_t) profondita;
    if (nodo % PASSI_BLOCCO == 0) {
        const int16_t blocco[2] = {p->bassa, p->alta};
        fwrite(blocco, sizeof blocco, 1, p->profilo);
    }
    if (nodo % PASSI_ISTANTANEA) return;  // non e' il momento dell'istantanea
    fwrite(p->strada, sizeof(int16_t), 1 + 2 * p->celle, p->istantanee);
    fflush(p->passi);  // il sito legge mentre la ricerca va avanti: fino a qui tutto scritto
    fflush(p->istantanee);
    fflush(p->profilo);
}

// Apre il file dei passi e, accanto, quelli delle istantanee e del profilo (lo stesso nome con .ist e .prof in
// fondo).
Passi *passi_apri(const char *percorso, const int celle) {
    char istantanee[1024], profilo[1024];
    snprintf(istantanee, sizeof istantanee, "%s.ist", percorso);
    snprintf(profilo, sizeof profilo, "%s.prof", percorso);
    Passi *p = calloc(1, sizeof(Passi));
    if (!p) return NULL;
    p->passi = fopen(percorso, "wb");
    p->istantanee = fopen(istantanee, "wb");
    p->profilo = fopen(profilo, "wb");
    p->strada = calloc(1 + 2 * celle, sizeof(int16_t));
    p->celle = celle;
    if (!p->passi || !p->istantanee || !p->profilo || !p->strada) {
        passi_chiudi(p);
        return NULL;
    }
    return p;
}

// Riapre i tre file per continuare. Bisogna rimettere com'erano le due cose che passi_nodo tiene in memoria: la
// strada dell'ultimo nodo (dall'ultima istantanea, applicando i record dopo: lo stesso conto che fa il sito in
// passi.py) e la profondita piu' bassa e piu' alta del blocco di profilo rimasto a meta'.
Passi *passi_riapri(const char *percorso, const int celle, long long *nodi, int *strada, int *profondita) {
    char istantanee[1024], profilo[1024];
    snprintf(istantanee, sizeof istantanee, "%s.ist", percorso);
    snprintf(profilo, sizeof profilo, "%s.prof", percorso);
    const long lungo_record = PASSI_CAMPI * sizeof(int16_t), lunga_istantanea = (1 + 2 * celle) * sizeof(int16_t);
    struct stat st;
    if (stat(percorso, &st) || st.st_size % lungo_record) return NULL;
    const long long n = st.st_size / lungo_record;      // i nodi che ci sono gia'
    const long long j = n / PASSI_ISTANTANEA;           // le istantanee intere: l'ultima e' del nodo j * ISTANTANEA
    const long long blocchi = n / PASSI_BLOCCO;         // i blocchi di profilo interi
    Passi *p = calloc(1, sizeof(Passi));
    if (!p) return NULL;
    p->strada = calloc(1 + 2 * celle, sizeof(int16_t));
    p->celle = celle;
    FILE *f = fopen(percorso, "rb"), *g = fopen(istantanee, "rb");
    int ok = p->strada && f && g;
    // la strada dell'ultima istantanea (prima della prima istantanea si parte dalla strada vuota)...
    if (ok && j)
        ok = !fseek(g, (j - 1) * lunga_istantanea, SEEK_SET) && fread(p->strada, lunga_istantanea, 1, g) == 1;
    // ...e i record dopo, uno per uno: ognuno mette la sua scelta al posto della sua profondita
    ok = ok && !fseek(f, j * PASSI_ISTANTANEA * lungo_record, SEEK_SET);
    for (long long nodo = j * PASSI_ISTANTANEA + 1; ok && nodo <= n; nodo++) {
        int16_t r[PASSI_CAMPI];
        if (!(ok = fread(r, lungo_record, 1, f) == 1 && r[0] >= 0 && r[0] <= celle)) break;
        p->strada[0] = r[0];
        if (r[0]) p->strada[2 * r[0] - 1] = r[1], p->strada[2 * r[0]] = r[2];
        if (nodo <= blocchi * PASSI_BLOCCO) continue;  // il suo blocco di profilo e' gia' scritto
        if (nodo % PASSI_BLOCCO == 1 || r[0] < p->bassa) p->bassa = r[0];
        if (nodo % PASSI_BLOCCO == 1 || r[0] > p->alta) p->alta = r[0];
    }
    if (f) fclose(f);
    if (g) fclose(g);
    // istantanee e profilo esattamente lunghi quanto i nodi che ci sono, poi tutti e tre si riaprono per aggiungere
    ok = ok && !truncate(istantanee, j * lunga_istantanea) && !truncate(profilo, blocchi * 2 * sizeof(int16_t));
    if (ok) {
        p->passi = fopen(percorso, "ab");
        p->istantanee = fopen(istantanee, "ab");
        p->profilo = fopen(profilo, "ab");
    }
    if (!ok || !p->passi || !p->istantanee || !p->profilo) {
        passi_chiudi(p);
        return NULL;
    }
    *nodi = n;
    *profondita = p->strada[0];
    for (int i = 0; i < 2 * p->strada[0]; i++) strada[i] = p->strada[1 + i];
    return p;
}

void passi_chiudi(Passi *p) {
    if (!p) return;
    if (p->passi) fclose(p->passi);
    if (p->istantanee) fclose(p->istantanee);
    if (p->profilo) fclose(p->profilo);
    free(p->strada);
    free(p);
}
