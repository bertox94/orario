#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Lettura a discesa ricorsiva: una funzione per ogni tipo di valore, che avanza il cursore.
typedef struct {
    const char *inizio;
    const char *p;    // cursore
    char *errore;
    size_t dimensione;
    int fallito;
} lettore_t;

static void errore(lettore_t *l, const char *cosa) {
    if (l->fallito) return; // si tiene il primo errore
    l->fallito = 1;
    // riga e colonna del cursore, per un messaggio utile
    int riga = 1, colonna = 1;
    for (const char *c = l->inizio; c < l->p; c++) {
        if (*c == '\n') {
            riga++;
            colonna = 1;
        } else colonna++;
    }
    snprintf(l->errore, l->dimensione, "%s (riga %d, colonna %d)", cosa, riga, colonna);
}

static void spazi(lettore_t *l) {
    while (*l->p == ' ' || *l->p == '\t' || *l->p == '\n' || *l->p == '\r') l->p++;
}

static json_t *nuovo(const json_tipo tipo) {
    json_t *j = calloc(1, sizeof(json_t));
    j->tipo = tipo;
    return j;
}

static json_t *valore(lettore_t *l, int profondita);

// Scrive in out il carattere Unicode cp in UTF-8; ritorna quanti byte.
static int utf8(char *out, const unsigned int cp) {
    if (cp < 0x80) {
        out[0] = (char) cp;
        return 1;
    }
    if (cp < 0x800) {
        out[0] = (char) (0xC0 | cp >> 6);
        out[1] = (char) (0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char) (0xE0 | cp >> 12);
        out[1] = (char) (0x80 | (cp >> 6 & 0x3F));
        out[2] = (char) (0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char) (0xF0 | cp >> 18);
    out[1] = (char) (0x80 | (cp >> 12 & 0x3F));
    out[2] = (char) (0x80 | (cp >> 6 & 0x3F));
    out[3] = (char) (0x80 | (cp & 0x3F));
    return 4;
}

static int esadecimale(lettore_t *l, unsigned int *cp) {
    *cp = 0;
    for (int i = 0; i < 4; i++) {
        const char c = *l->p++;
        *cp <<= 4;
        if (c >= '0' && c <= '9') *cp |= c - '0';
        else if (c >= 'a' && c <= 'f') *cp |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') *cp |= c - 'A' + 10;
        else return 0;
    }
    return 1;
}

// Una stringa tra virgolette (il cursore e' sulla virgoletta iniziale). Ritorna il testo, NULL se errore.
static char *testo_stringa(lettore_t *l) {
    l->p++;
    // il testo decodificato non e' mai piu' lungo di quello scritto
    const char *fine = l->p;
    while (*fine && *fine != '"') fine += *fine == '\\' && fine[1] ? 2 : 1;
    char *out = malloc(fine - l->p + 1);
    size_t n = 0;
    while (*l->p != '"') {
        if (!*l->p || (unsigned char) *l->p < 0x20) {
            free(out);
            errore(l, "stringa non chiusa o con un carattere di controllo");
            return NULL;
        }
        if (*l->p != '\\') {
            out[n++] = *l->p++;
            continue;
        }
        l->p++;
        const char c = *l->p++;
        unsigned int cp;
        switch (c) {
            case '"': out[n++] = '"'; break;
            case '\\': out[n++] = '\\'; break;
            case '/': out[n++] = '/'; break;
            case 'b': out[n++] = '\b'; break;
            case 'f': out[n++] = '\f'; break;
            case 'n': out[n++] = '\n'; break;
            case 'r': out[n++] = '\r'; break;
            case 't': out[n++] = '\t'; break;
            case 'u':
                if (!esadecimale(l, &cp)) {
                    free(out);
                    errore(l, "\\u non seguito da 4 cifre esadecimali");
                    return NULL;
                }
                // coppia di surrogati (caratteri oltre U+FFFF)
                if (cp >= 0xD800 && cp < 0xDC00 && l->p[0] == '\\' && l->p[1] == 'u') {
                    unsigned int basso;
                    l->p += 2;
                    if (esadecimale(l, &basso) && basso >= 0xDC00 && basso < 0xE000)
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (basso - 0xDC00);
                }
                n += utf8(out + n, cp);
                break;
            default:
                free(out);
                errore(l, "sequenza \\ non valida in una stringa");
                return NULL;
        }
    }
    l->p++;
    out[n] = '\0';
    return out;
}

static json_t *numero(lettore_t *l) {
    char *fine;
    const double v = strtod(l->p, &fine);
    if (fine == l->p) {
        errore(l, "valore non valido");
        return NULL;
    }
    l->p = fine;
    json_t *j = nuovo(JSON_NUMERO);
    j->numero = v;
    return j;
}

static void aggiungi(json_t *j, char *chiave, json_t *v) {
    j->elementi = realloc(j->elementi, (j->n + 1) * sizeof(json_t *));
    if (j->tipo == JSON_OGGETTO) j->chiavi = realloc(j->chiavi, (j->n + 1) * sizeof(char *));
    j->elementi[j->n] = v;
    if (j->tipo == JSON_OGGETTO) j->chiavi[j->n] = chiave;
    j->n++;
}

// Elenco [..] o oggetto {..}: il cursore e' sulla parentesi aperta.
static json_t *contenitore(lettore_t *l, const int oggetto, const int profondita) {
    const char chiusa = oggetto ? '}' : ']';
    json_t *j = nuovo(oggetto ? JSON_OGGETTO : JSON_ELENCO);
    l->p++;
    spazi(l);
    if (*l->p == chiusa) {
        l->p++;
        return j;
    }
    for (;;) {
        char *chiave = NULL;
        if (oggetto) {
            spazi(l);
            if (*l->p != '"') {
                errore(l, "attesa una chiave tra virgolette");
                break;
            }
            chiave = testo_stringa(l);
            if (!chiave) break;
            spazi(l);
            if (*l->p != ':') {
                free(chiave);
                errore(l, "atteso ':' dopo la chiave");
                break;
            }
            l->p++;
        }
        json_t *v = valore(l, profondita + 1);
        if (!v) {
            free(chiave);
            break;
        }
        aggiungi(j, chiave, v);
        spazi(l);
        if (*l->p == ',') {
            l->p++;
            continue;
        }
        if (*l->p == chiusa) {
            l->p++;
            return j;
        }
        errore(l, oggetto ? "atteso ',' o '}'" : "atteso ',' o ']'");
        break;
    }
    json_libera(j);
    return NULL;
}

static json_t *valore(lettore_t *l, const int profondita) {
    if (profondita > 64) {
        errore(l, "troppi livelli annidati");
        return NULL;
    }
    spazi(l);
    switch (*l->p) {
        case '{': return contenitore(l, 1, profondita);
        case '[': return contenitore(l, 0, profondita);
        case '"': {
            char *s = testo_stringa(l);
            if (!s) return NULL;
            json_t *j = nuovo(JSON_STRINGA);
            j->stringa = s;
            return j;
        }
        default: break;
    }
    if (!strncmp(l->p, "null", 4)) {
        l->p += 4;
        return nuovo(JSON_NULL);
    }
    if (!strncmp(l->p, "true", 4) || !strncmp(l->p, "false", 5)) {
        json_t *j = nuovo(JSON_BOOL);
        j->numero = l->p[0] == 't';
        l->p += l->p[0] == 't' ? 4 : 5;
        return j;
    }
    return numero(l);
}

json_t *json_leggi(const char *testo, char *messaggio, const size_t dimensione) {
    lettore_t l = {testo, testo, messaggio, dimensione, 0};
    json_t *j = valore(&l, 0);
    if (j) {
        spazi(&l);
        if (*l.p) {
            errore(&l, "testo in piu' dopo la fine del valore");
            json_libera(j);
            return NULL;
        }
    }
    return j;
}

const json_t *json_campo(const json_t *j, const char *chiave) {
    if (!j || j->tipo != JSON_OGGETTO) return NULL;
    for (int i = 0; i < j->n; i++)
        if (!strcmp(j->chiavi[i], chiave)) return j->elementi[i];
    return NULL;
}

void json_libera(json_t *j) {
    if (!j) return;
    for (int i = 0; i < j->n; i++) {
        json_libera(j->elementi[i]);
        if (j->chiavi) free(j->chiavi[i]);
    }
    free(j->elementi);
    free(j->chiavi);
    free(j->stringa);
    free(j);
}
