#include "fine.h"

#include <stdio.h>
#include <string.h>

// Il file FILE.fine. Alla fine di una ricerca con i passi si scrive FILE.fine: i numeri che servono per continuarla e che nei passi non
// ci sono (o costerebbe rileggerli tutti). Nell'ordine dei byte del computer: l'esito (int: si continua solo dopo
// LIMITE_DI_NODI), nodi e ritorni (long long), profondita massima (int), secondi (double), i ritorni di ogni cella
// (long long l'uno) e, dopo, quelli recenti di ogni cella e i due conti per motivo (in un file scritto prima che
// esistessero mancano: si riparte da zero).

static void nome_della_fine(char *nome, const size_t lungo, const char *file_passi) {
    snprintf(nome, lungo, "%s.fine", file_passi);
}

void scrivi_la_fine(const char *file_passi, const Ricerca *r, const double secondi) {
    char nome[1024];
    nome_della_fine(nome, sizeof nome, file_passi);
    FILE *f = fopen(nome, "wb");
    if (!f) return;
    const int esito = r->esito;
    fwrite(&esito, sizeof esito, 1, f);
    fwrite(&r->nodi, sizeof r->nodi, 1, f);
    fwrite(&r->ritorni, sizeof r->ritorni, 1, f);
    fwrite(&r->profondita_massima, sizeof r->profondita_massima, 1, f);
    fwrite(&secondi, sizeof secondi, 1, f);
    fwrite(r->ritorni_in, sizeof(long long), r->celle, f);
    fwrite(r->ritorni_recenti_in, sizeof(long long), r->celle, f);
    fwrite(r->ritorni_per, sizeof(long long), posti_dei_motivi(r->s), f);
    fwrite(r->ritorni_recenti_per, sizeof(long long), posti_dei_motivi(r->s), f);
    fclose(f);
}

// rimette in r i numeri della ricerca da continuare; 0 se il file manca, non e' quello di quei `nodi` nodi, o la
// ricerca non era stata fermata dal limite (era finita: non c'e' niente da continuare)
int leggi_la_fine(const char *file_passi, Ricerca *r, const long long nodi, double *secondi) {
    char nome[1024];
    nome_della_fine(nome, sizeof nome, file_passi);
    FILE *f = fopen(nome, "rb");
    if (!f) return 0;
    int esito = IN_CORSO;
    const int ok = fread(&esito, sizeof esito, 1, f) == 1 && esito == LIMITE_DI_NODI
                   && fread(&r->nodi, sizeof r->nodi, 1, f) == 1 && fread(&r->ritorni, sizeof r->ritorni, 1, f) == 1
                   && fread(&r->profondita_massima, sizeof r->profondita_massima, 1, f) == 1
                   && fread(secondi, sizeof *secondi, 1, f) == 1
                   && fread(r->ritorni_in, sizeof(long long), r->celle, f) == (size_t) r->celle && r->nodi == nodi;
    if (ok && fread(r->ritorni_recenti_in, sizeof(long long), r->celle, f) != (size_t) r->celle)
        memset(r->ritorni_recenti_in, 0, sizeof(long long) * r->celle);
    const size_t posti = (size_t) posti_dei_motivi(r->s);
    if (ok && (fread(r->ritorni_per, sizeof(long long), posti, f) != posti
               || fread(r->ritorni_recenti_per, sizeof(long long), posti, f) != posti)) {
        memset(r->ritorni_per, 0, sizeof(long long) * posti);
        memset(r->ritorni_recenti_per, 0, sizeof(long long) * posti);
    }
    fclose(f);
    return ok;
}
