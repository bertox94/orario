#ifndef UNTITLED_JSON_H
#define UNTITLED_JSON_H

// Il poco che serve per scrivere JSON (lo stato, i dati e i passi per il sito).

#include <stdio.h>

// una stringa tra virgolette (i nomi non hanno caratteri di controllo: basta proteggere virgolette e barre)
static inline void json_stringa(FILE *r, const char *s) {
    fputc('"', r);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') fputc('\\', r);
        fputc(*s, r);
    }
    fputc('"', r);
}

// una stringa, o null se manca
static inline void json_nome(FILE *r, const char *s) {
    if (s) json_stringa(r, s);
    else fputs("null", r);
}

#endif //UNTITLED_JSON_H
