#include "passi.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Passi {
    FILE *passi, *istantanee;
};

Passi *passi_apri(const char *percorso) {
    char ist[1024];
    snprintf(ist, sizeof ist, "%s.ist", percorso);
    Passi *x = malloc(sizeof(Passi));
    if (!x) return NULL;
    x->passi = fopen(percorso, "wb");
    x->istantanee = fopen(ist, "wb");
    if (x->passi && x->istantanee) return x;
    passi_chiudi(x);
    return NULL;
}

void passi_nodo(Passi *x, const long long int nodo, const int pos, const teaching_slot *orario, const int celle) {
    const int16_t r[3] = {(int16_t) pos, (int16_t) (pos > 0 ? orario[pos - 1].insegnante1 : -1),
                          (int16_t) (pos > 0 ? orario[pos - 1].insegnante2 : -1)};
    fwrite(r, sizeof r, 1, x->passi);
    if (nodo % PASSI_ISTANTANEA) return;
    for (int i = 0; i < celle; i++) {
        const int16_t c[2] = {(int16_t) orario[i].insegnante1, (int16_t) orario[i].insegnante2};
        fwrite(c, sizeof c, 1, x->istantanee);
    }
    // il sito legge mentre la ricerca va avanti: fino a qui tutto scritto
    fflush(x->passi);
    fflush(x->istantanee);
}

void passi_chiudi(Passi *x) {
    if (!x) return;
    if (x->passi) fclose(x->passi);
    if (x->istantanee) fclose(x->istantanee);
    free(x);
}

// ---- rileggere un nodo ----
static long dimensione(FILE *x) {
    fseek(x, 0, SEEK_END);
    return ftell(x);
}

int passi_leggi(const char *percorso, const long long int k, const int celle, teaching_slot *orario, int *pos_prima) {
    char ist[1024];
    snprintf(ist, sizeof ist, "%s.ist", percorso);
    FILE *p = fopen(percorso, "rb"), *i = fopen(ist, "rb");
    int pos = -1;
    if (!p || !i || k < 1 || k > dimensione(p) / 6) goto fine;
    // l'ultima istantanea prima del nodo (la j e' l'orario dopo il nodo j * PASSI_ISTANTANEA), o l'orario vuoto
    const long per_istantanea = 2L * sizeof(int16_t) * celle;
    long long int j = k / PASSI_ISTANTANEA;
    if (j > dimensione(i) / per_istantanea) j = dimensione(i) / per_istantanea;
    int16_t c[2 * GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
    for (int n = 0; n < 2 * celle; n++) c[n] = -1;
    if (j) {
        fseek(i, (j - 1) * per_istantanea, SEEK_SET);
        if (fread(c, 1, per_istantanea, i) != (size_t) per_istantanea) goto fine;
    }
    // poi i record fino a k: ognuno e' la scelta messa nella cella prima di quella che il nodo decide
    int16_t r[3] = {0, -1, -1};
    *pos_prima = -1;
    long long int n = j * PASSI_ISTANTANEA;  // l'ultimo nodo gia' dentro l'istantanea
    if (n == k) n--;                         // k e' proprio quello: si rilegge, per la sua posizione
    if (n >= 1) {                            // il nodo prima del primo da applicare: serve solo la sua posizione
        fseek(p, (n - 1) * 6, SEEK_SET);
        if (fread(r, sizeof r, 1, p) != 1) goto fine;
    } else fseek(p, 0, SEEK_SET);
    for (n++; n <= k; n++) {
        *pos_prima = n > 1 ? r[0] : -1;
        if (fread(r, sizeof r, 1, p) != 1) goto fine;
        if (r[0] > 0) {
            c[2 * (r[0] - 1)] = r[1];
            c[2 * (r[0] - 1) + 1] = r[2];
        }
    }
    pos = r[0];
    for (int n2 = 0; n2 < pos; n2++) orario[n2] = (teaching_slot){c[2 * n2], c[2 * n2 + 1]};
fine:
    if (p) fclose(p);
    if (i) fclose(i);
    return pos;
}
