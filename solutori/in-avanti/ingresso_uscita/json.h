#ifndef UNTITLED_JSON_H
#define UNTITLED_JSON_H

// Lettore JSON minimo, per i dati in ingresso (--input). Legge tutto il testo in un albero di valori.
// Basta per file piccoli scritti da un programma (il sito): niente commenti, niente estensioni.

#include <stddef.h>

typedef enum { JSON_NULL, JSON_BOOL, JSON_NUMERO, JSON_STRINGA, JSON_ELENCO, JSON_OGGETTO } json_tipo;

typedef struct json {
    json_tipo tipo;
    double numero;         // JSON_NUMERO; JSON_BOOL: 1 o 0
    char *stringa;         // JSON_STRINGA (UTF-8, terminata da \0)
    int n;                 // JSON_ELENCO, JSON_OGGETTO: quanti elementi
    struct json **elementi; // JSON_ELENCO, JSON_OGGETTO: i valori
    char **chiavi;         // JSON_OGGETTO: le chiavi, nello stesso ordine dei valori
} json_t;

// Legge il testo. Ritorna NULL se non e' JSON valido, con in `errore` cosa non va e dove.
json_t *json_leggi(const char *testo, char *errore, size_t dimensione);

// Il valore della chiave in un oggetto (NULL se manca o se j non e' un oggetto).
const json_t *json_campo(const json_t *j, const char *chiave);

void json_libera(json_t *j);

#endif //UNTITLED_JSON_H
