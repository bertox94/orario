#include "passi.h"

#include <stdint.h>
#include <stdio.h>

static FILE *passi = NULL, *istantanee = NULL;

int passi_apri(const char *percorso) {
    char ist[1024];
    snprintf(ist, sizeof ist, "%s.ist", percorso);
    passi = fopen(percorso, "wb");
    istantanee = fopen(ist, "wb");
    return passi && istantanee;
}

void passi_nodo(const long long int nodo, const int pos, const teaching_slot *orario, const int celle) {
    if (!passi) return;
    const int16_t r[3] = {(int16_t) pos, (int16_t) (pos > 0 ? orario[pos - 1].insegnante1 : -1),
                          (int16_t) (pos > 0 ? orario[pos - 1].insegnante2 : -1)};
    fwrite(r, sizeof r, 1, passi);
    if (nodo % PASSI_ISTANTANEA) return;
    for (int i = 0; i < celle; i++) {
        const int16_t c[2] = {(int16_t) orario[i].insegnante1, (int16_t) orario[i].insegnante2};
        fwrite(c, sizeof c, 1, istantanee);
    }
    // il sito legge mentre la ricerca va avanti: fino a qui tutto scritto
    fflush(passi);
    fflush(istantanee);
}

void passi_chiudi(void) {
    if (passi) fclose(passi);
    if (istantanee) fclose(istantanee);
    passi = istantanee = NULL;
}
