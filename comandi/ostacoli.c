#include "ostacoli.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// un ostacolo, aperto o chiuso
typedef struct {
    int cella;              // la cella da cui la ricerca torna indietro
    int fondo;              // la cella piu' bassa che ha cambiato per superarla
    long long int dal, al;  // il nodo del primo ritorno e quello in cui la cella e' superata (0: non ancora)
    long long int ritorni;  // quante volte e' tornata indietro da quella cella
} Ostacolo;

#define PEZZO 65536  // quanti nodi si leggono per volta

static long long int durata(const Ostacolo *o, const long long int nodi) {
    return (o->al ? o->al : nodi) - o->dal;
}

// tiene in `lunghi` (al massimo `quanti`, dal piu' lungo) gli ostacoli che hanno richiesto piu' nodi
static void tieni(Ostacolo *lunghi, int *n, const int quanti, const Ostacolo *o, const long long int nodi) {
    if (!quanti) return;
    int i = *n;
    if (i == quanti) {
        if (durata(o, nodi) <= durata(&lunghi[i - 1], nodi)) return;
        i--;
    } else (*n)++;
    for (; i > 0 && durata(&lunghi[i - 1], nodi) < durata(o, nodi); i--) lunghi[i] = lunghi[i - 1];
    lunghi[i] = *o;
}

int stampa_ostacoli(const char *file_passi, int fasce, const int quanti) {
    FILE *f = fopen(file_passi, "rb");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    const long long int nodi = ftell(f) / (3 * (long long int) sizeof(int16_t));
    rewind(f);
    if (fasce > nodi) fasce = (int) nodi;
    if (fasce < 0) fasce = 0;
    // gli ostacoli aperti stanno uno dentro l'altro: una pila, con in cima il piu' interno. Sono al massimo uno per
    // cella, e le celle sono meno di 32768 (interi a 16 bit)
    Ostacolo *pila = malloc(32768 * sizeof(Ostacolo)), *lunghi = malloc((quanti > 0 ? quanti : 1) * sizeof(Ostacolo));
    int (*fascia)[2] = malloc((fasce > 0 ? fasce : 1) * sizeof(int[2]));
    int16_t *pezzo = malloc(PEZZO * 3 * sizeof(int16_t));
    if (!pila || !lunghi || !fascia || !pezzo) {
        fclose(f);
        return 1;
    }
    int aperti = 0, n_lunghi = 0, prima = -1, massima = -1;
    long long int k = 0, ritorni = 0, ostacoli = 0;
    size_t letti;
    while ((letti = fread(pezzo, 3 * sizeof(int16_t), PEZZO, f)) > 0) {
        for (size_t i = 0; i < letti; i++) {
            const int pos = pezzo[3 * i];
            k++;  // il nodo k decide la cella pos
            if (pos > massima) massima = pos;
            if (fasce) {
                const int j = (int) ((k - 1) * fasce / nodi);
                if ((k - 1) * fasce / nodi != (k - 2) * fasce / nodi || k == 1) fascia[j][0] = fascia[j][1] = pos;
                if (pos < fascia[j][0]) fascia[j][0] = pos;
                if (pos > fascia[j][1]) fascia[j][1] = pos;
            }
            if (prima >= 0 && pos <= prima) {
                // il nodo prima, che decideva `prima`, e' un ritorno: la ricerca ha cambiato la cella pos - 1
                ritorni++;
                if (aperti && pila[aperti - 1].cella == prima) pila[aperti - 1].ritorni++;
                else {
                    pila[aperti++] = (Ostacolo) {prima, pos - 1, k - 1, 0, 1};
                    ostacoli++;
                }
                if (pos - 1 < pila[aperti - 1].fondo) pila[aperti - 1].fondo = pos - 1;
            } else {
                // la ricerca va avanti: sono superati gli ostacoli delle celle prima di pos
                while (aperti && pila[aperti - 1].cella < pos) {
                    Ostacolo *o = &pila[--aperti];
                    o->al = k;
                    tieni(lunghi, &n_lunghi, quanti, o, nodi);
                    // quanto e' scesa per un ostacolo vale anche per quello che lo contiene
                    if (aperti && o->fondo < pila[aperti - 1].fondo) pila[aperti - 1].fondo = o->fondo;
                }
            }
            prima = pos;
        }
    }
    fclose(f);
    // alla fine del file: gli ostacoli ancora aperti non sono superati, dal piu' interno
    while (aperti) {
        const Ostacolo *o = &pila[--aperti];
        tieni(lunghi, &n_lunghi, quanti, o, k);
        if (aperti && o->fondo < pila[aperti - 1].fondo) pila[aperti - 1].fondo = o->fondo;
    }
    printf("{\"passi\":%lld,\"ritorni\":%lld,\"ostacoli\":%lld,\"cella_massima\":%d,\"fasce\":[", k, ritorni, ostacoli,
           massima);
    for (int j = 0; j < fasce; j++) printf("%s[%d,%d]", j ? "," : "", fascia[j][0], fascia[j][1]);
    printf("],\"lunghi\":[");
    for (int i = 0; i < n_lunghi; i++) {
        const Ostacolo *o = &lunghi[i];
        printf("%s{\"cella\":%d,\"profondita\":%d,\"dal\":%lld,\"al\":", i ? "," : "", o->cella, o->cella - o->fondo,
               o->dal);
        if (o->al) printf("%lld", o->al);
        else printf("null");
        printf(",\"passi\":%lld,\"ritorni\":%lld}", durata(o, k), o->ritorni);
    }
    printf("]}\n");
    free(pila);
    free(lunghi);
    free(fascia);
    free(pezzo);
    return 0;
}
