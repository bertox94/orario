#include "dati_da_file.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "database_initializers.h"
#include "json.h"
#include "../priority_queue/prio_queue.h"

// Il testo intero del file (NULL se non si legge).
static char *leggi_file(const char *file) {
    FILE *f = fopen(file, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    const long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *testo = malloc(n + 1);
    const size_t letti = fread(testo, 1, n, f);
    fclose(f);
    testo[letti] = '\0';
    return testo;
}

// Un elenco di al massimo `massimo` elementi (e almeno `minimo`).
static const json_t *elenco(const json_t *dati, const char *chiave, const int minimo, const int massimo, char *errore,
                            const size_t dim) {
    const json_t *e = json_campo(dati, chiave);
    if (!e || e->tipo != JSON_ELENCO) {
        snprintf(errore, dim, "manca l'elenco \"%s\"", chiave);
        return NULL;
    }
    if (e->n < minimo || e->n > massimo) {
        snprintf(errore, dim, "\"%s\" deve avere da %d a %d elementi (ne ha %d)", chiave, minimo, massimo, e->n);
        return NULL;
    }
    return e;
}

// Un nome: stringa non vuota che sta in MAX_LEN_WORDS. Copiato in `nome`.
static int leggi_nome(const json_t *v, char nome[MAX_LEN_WORDS], const char *cosa, const int i, char *errore,
                      const size_t dim) {
    if (!v || v->tipo != JSON_STRINGA || !v->stringa[0]) {
        snprintf(errore, dim, "%s %d: manca il nome", cosa, i + 1);
        return 0;
    }
    if (strlen(v->stringa) >= MAX_LEN_WORDS) {
        snprintf(errore, dim, "%s %d: nome troppo lungo (al massimo %d caratteri)", cosa, i + 1, MAX_LEN_WORDS - 1);
        return 0;
    }
    strcpy(nome, v->stringa);
    return 1;
}

// Un intero in [minimo, massimo]; se manca vale `predefinito` (se predefinito < minimo il campo e' obbligatorio).
static int leggi_intero(const json_t *v, int *out, const int minimo, const int massimo, const int predefinito) {
    if (!v || v->tipo == JSON_NULL) {
        *out = predefinito;
        return predefinito >= minimo;
    }
    if (v->tipo != JSON_NUMERO || v->numero != floor(v->numero) || v->numero < minimo || v->numero > massimo)
        return 0;
    *out = (int) v->numero;
    return 1;
}

static int leggi_calendario(const json_t *dati, char *errore, const size_t dim) {
    const json_t *giorni = elenco(dati, "giorni", 1, GIORNO_SETTIMANA_SZ, errore, dim);
    if (!giorni) return 0;
    const json_t *ore = elenco(dati, "ore", PRIME_N_ORE_OBBLIGATORIE, ORA_GIORNALIERA_SZ, errore, dim);
    if (!ore) return 0;
    char nome[MAX_LEN_WORDS];
    for (int i = 0; i < giorni->n; i++) {
        if (!leggi_nome(giorni->elementi[i], nome, "giorno", i, errore, dim)) return 0;
        set_giorno_settimana(nome);
    }
    for (int i = 0; i < ore->n; i++) {
        if (!leggi_nome(ore->elementi[i], nome, "ora", i, errore, dim)) return 0;
        set_ora_giornaliera(nome);
    }
    if (actual_giorni_settimana != giorni->n || actual_ore_giornaliere != ore->n) {
        snprintf(errore, dim, "giorni e ore devono avere nomi diversi tra loro");
        return 0;
    }
    return 1;
}

// Classi e docenti si registrano nell'ordine dei file, cosi' i loro indici sono quelli usati dalle lezioni.
static int leggi_classi_e_docenti(const json_t *dati, char *errore, const size_t dim) {
    const json_t *classi = elenco(dati, "classi", 1, CLASSE_SEZIONE_SZ, errore, dim);
    if (!classi) return 0;
    const json_t *docenti = elenco(dati, "docenti", 1, INSEGNANTE_SZ, errore, dim);
    if (!docenti) return 0;
    char nome[MAX_LEN_WORDS];
    for (int i = 0; i < classi->n; i++) {
        if (!leggi_nome(json_campo(classi->elementi[i], "nome"), nome, "classe", i, errore, dim)) return 0;
        if (p_classe_sezione(nome) >= 0) {
            snprintf(errore, dim, "classe \"%s\" ripetuta", nome);
            return 0;
        }
        set_classe_sezione(nome);
    }
    for (int i = 0; i < docenti->n; i++) {
        const json_t *d = docenti->elementi[i];
        if (!leggi_nome(json_campo(d, "nome"), nome, "docente", i, errore, dim)) return 0;
        if (p_insegnante(nome) >= 0) {
            snprintf(errore, dim, "docente \"%s\" ripetuto", nome);
            return 0;
        }
        set_insegnante(nome);
        int ingresso;
        if (!leggi_intero(json_campo(d, "ingresso"), &ingresso, 0, actual_ore_giornaliere - 1, 0)) {
            snprintf(errore, dim, "docente \"%s\": \"ingresso\" deve essere un indice di ora da 0 a %d", nome,
                     actual_ore_giornaliere - 1);
            return 0;
        }
        docente_info[i].ora_entrata = ingresso;
    }
    return 1;
}

// Mette in coda i docenti nell'ordine indicato, con priorita' 0: le lezioni poi aggiungono le ore.
static int leggi_ordine_coda(const json_t *dati, char *errore, const size_t dim) {
    const json_t *ordine = json_campo(dati, "ordine_coda");
    if (!ordine || ordine->tipo == JSON_NULL) return 1;
    if (ordine->tipo != JSON_ELENCO) {
        snprintf(errore, dim, "\"ordine_coda\" deve essere un elenco di indici di docenti");
        return 0;
    }
    for (int i = 0; i < ordine->n; i++) {
        int p;
        if (!leggi_intero(ordine->elementi[i], &p, 0, actual_insegnanti - 1, -1)) {
            snprintf(errore, dim, "\"ordine_coda\": elemento %d non e' un indice di docente", i + 1);
            return 0;
        }
        add_to_queue(p, 0);
    }
    return 1;
}

static int leggi_lezioni(const json_t *dati, char *errore, const size_t dim) {
    const json_t *lezioni = elenco(dati, "lezioni", 0, 1 << 20, errore, dim);
    if (!lezioni) return 0;
    const int max_ore = actual_giorni_settimana * actual_ore_giornaliere;
    for (int i = 0; i < lezioni->n; i++) {
        const json_t *l = lezioni->elementi[i];
        int c, p, q = -1, ore;
        const json_t *codocente = json_campo(l, "codocente");
        const int con_codocente = codocente && codocente->tipo != JSON_NULL;
        if (!leggi_intero(json_campo(l, "classe"), &c, 0, actual_classi_sezione - 1, -1)
            || !leggi_intero(json_campo(l, "docente"), &p, 0, actual_insegnanti - 1, -1)
            || (con_codocente && !leggi_intero(codocente, &q, 0, actual_insegnanti - 1, -1))
            || !leggi_intero(json_campo(l, "ore"), &ore, 1, max_ore, -1)) {
            snprintf(errore, dim, "lezione %d: classe, docente, codocente o ore non validi", i + 1);
            return 0;
        }
        if (q == p) {
            snprintf(errore, dim, "lezione %d: docente e codocente sono la stessa persona", i + 1);
            return 0;
        }
        // set_* vogliono char[] modificabili
        char cla[MAX_LEN_WORDS], prof[MAX_LEN_WORDS], cod[MAX_LEN_WORDS];
        strcpy(cla, classe_sezione[c]);
        strcpy(prof, insegnanti[p]);
        if (q < 0) {
            set_classe_insegnante_ore(cla, prof, ore);
        } else {
            strcpy(cod, insegnanti[q]);
            // tutte le ore della lezione sono in coppia: il docente non ha ore da solo in questa lezione
            set_classe_insegnante_ore_cod(cla, prof, ore, cod, ore);
        }
    }
    return 1;
}

// Se il file indica anche le ore di classi e docenti (come quello scritto da --json), devono tornare con le lezioni.
static int ore_dichiarate_coerenti(const json_t *dati, char *errore, const size_t dim) {
    const json_t *classi = json_campo(dati, "classi"), *docenti = json_campo(dati, "docenti");
    for (int i = 0; i < classi->n; i++) {
        const json_t *ore = json_campo(classi->elementi[i], "ore");
        if (ore && ore->tipo == JSON_NUMERO && (int) ore->numero != classe_info[i].ore) {
            snprintf(errore, dim, "classe \"%s\": %d ore dichiarate, ma dalle lezioni sono %d", classe_sezione[i],
                     (int) ore->numero, classe_info[i].ore);
            return 0;
        }
    }
    for (int i = 0; i < docenti->n; i++) {
        const json_t *ore = json_campo(docenti->elementi[i], "ore");
        if (ore && ore->tipo == JSON_NUMERO && (int) ore->numero != docente_info[i].ore) {
            snprintf(errore, dim, "docente \"%s\": %d ore dichiarate, ma dalle lezioni sono %d", insegnanti[i],
                     (int) ore->numero, docente_info[i].ore);
            return 0;
        }
    }
    return 1;
}

int carica_dati_da_file(const char *file, char *errore, const size_t dimensione) {
    char *testo = leggi_file(file);
    if (!testo) {
        snprintf(errore, dimensione, "impossibile leggere %s", file);
        return 0;
    }
    char messaggio[256];
    json_t *radice = json_leggi(testo, messaggio, sizeof messaggio);
    free(testo);
    if (!radice) {
        snprintf(errore, dimensione, "%s non e' JSON valido: %s", file, messaggio);
        return 0;
    }
    const json_t *dati = json_campo(radice, "dati") ? json_campo(radice, "dati") : radice;
    const int ok = leggi_calendario(dati, errore, dimensione)
                   && leggi_classi_e_docenti(dati, errore, dimensione)
                   && leggi_ordine_coda(dati, errore, dimensione)
                   && leggi_lezioni(dati, errore, dimensione)
                   && ore_dichiarate_coerenti(dati, errore, dimensione);
    json_libera(radice);
    return ok;
}
