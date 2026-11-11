#ifndef UNTITLED_JSON_H
#define UNTITLED_JSON_H

// Il poco che serve per scrivere JSON (lo stato, i dati e i passi per il sito).

#include <stdio.h>

#include "data_initialization/database_initializers.h"

// una stringa tra virgolette: protette virgolette e barre, e l'a capo (il motivo di un errore nei dati ha piu' righe)
static inline void json_stringa(FILE *r, const char *s) {
    fputc('"', r);
    for (; *s; s++) {
        if (*s == '\n') {
            fputs("\\n", r);
            continue;
        }
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

// un elenco di nomi: ["a","b",...]
static inline void json_nomi(FILE *r, const char nomi[][MAX_LEN_WORDS], const int n) {
    fputc('[', r);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', r);
        json_stringa(r, nomi[i]);
    }
    fputc(']', r);
}

// una cella dell'orario: i nomi dei suoi due docenti, [docente, codocente] (null: nessuno)
static inline void json_cella(FILE *r, const Scuola *s, const teaching_slot cella) {
    fputc('[', r);
    json_nome(r, cella.insegnante1 < 0 ? NULL : s->insegnanti[cella.insegnante1]);
    fputc(',', r);
    json_nome(r, cella.insegnante2 < 0 ? NULL : s->insegnanti[cella.insegnante2]);
    fputc(']', r);
}

#endif //UNTITLED_JSON_H
