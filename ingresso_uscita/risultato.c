#include "risultato.h"

#include <stdio.h>

// una stringa JSON (i nomi vengono dai dati: basta togliere virgolette, barre e caratteri di controllo)
static void stringa(FILE *r, const char *s) {
    fputc('"', r);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') fprintf(r, "\\%c", *s);
        else if ((unsigned char) *s < 0x20) fprintf(r, "\\u%04x", *s);
        else fputc(*s, r);
    }
    fputc('"', r);
}

static void nomi(FILE *r, const char *campo, char elenco[][MAX_LEN_WORDS], const int n) {
    fprintf(r, ",\"%s\":[", campo);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', r);
        stringa(r, elenco[i]);
    }
    fputc(']', r);
}

static void docente(FILE *r, const int p) {
    if (p < 0) fprintf(r, "null");
    else stringa(r, insegnanti[p]);
}

void scrivi_risultato(const char *percorso, const char *esito, const char *errore, const long long int nodi,
                      const double secondi, const long long int orari, const teaching_slot *orario, const int celle) {
    FILE *r = fopen(percorso, "w");
    if (!r) return;
    fprintf(r, "{\"esito\":");
    stringa(r, esito);
    if (errore) {
        fprintf(r, ",\"errore\":");
        stringa(r, errore);
    }
    fprintf(r, ",\"nodi\":%lld,\"secondi\":%.3f,\"orari\":%lld", nodi, secondi, orari);
    nomi(r, "giorni", giorni_settimana, actual_giorni_settimana);
    nomi(r, "ore", ore_giornaliere, actual_ore_giornaliere);
    nomi(r, "classi", classe_sezione, actual_classi_sezione);
    fprintf(r, ",\"orario\":");
    if (!orario) fprintf(r, "null");
    else {
        fputc('[', r);
        for (int i = 0; i < celle; i++) {
            fprintf(r, "%s[", i ? "," : "");
            docente(r, orario[i].insegnante1);
            fputc(',', r);
            docente(r, orario[i].insegnante2);
            fputc(']', r);
        }
        fputc(']', r);
    }
    fprintf(r, "}\n");
    fclose(r);
}
