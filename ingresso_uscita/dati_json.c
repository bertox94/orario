#include "dati_json.h"
#include "json.h"

#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void set_classe_insegnante_ore_cod(char cla[], char prof[], int ore, char cod[], int ore_cod);
int p_classe_sezione(char cla[]);

static int fallisci(char *errore, const size_t dimensione, const char *formato, ...) {
    va_list a;
    va_start(a, formato);
    vsnprintf(errore, dimensione, formato, a);
    va_end(a);
    return 0;
}

static char *leggi_file(const char *percorso) {
    FILE *f = fopen(percorso, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    const long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *testo = malloc(n + 1);
    if (testo && fread(testo, 1, n, f) != (size_t) n) {
        free(testo);
        testo = NULL;
    }
    if (testo) testo[n] = '\0';
    fclose(f);
    return testo;
}

// Una stringa del JSON in un buffer modificabile (le funzioni set_* di main vogliono char[]).
static int nome(const json_t *v, char out[MAX_LEN_WORDS]) {
    if (!v || v->tipo != JSON_STRINGA || !v->stringa[0] || strlen(v->stringa) >= MAX_LEN_WORDS) return 0;
    strcpy(out, v->stringa);
    return 1;
}

static int intero(const json_t *v, int *out) {
    if (!v || v->tipo != JSON_NUMERO || v->numero != (int) v->numero) return 0;
    *out = (int) v->numero;
    return 1;
}

int inizializza_dati_da_json(const char *percorso, char *errore, const size_t dimensione) {
    char *testo = leggi_file(percorso);
    if (!testo) return fallisci(errore, dimensione, "non riesco a leggere il file %s", percorso);
    char msg[256];
    json_t *j = json_leggi(testo, msg, sizeof msg);
    free(testo);
    if (!j) return fallisci(errore, dimensione, "il file non e' JSON valido: %s", msg);

    int ok = 0;
    char a[MAX_LEN_WORDS], b[MAX_LEN_WORDS];
    const json_t *giorni = json_campo(j, "giorni"), *ore = json_campo(j, "ore");
    const json_t *lezioni = json_campo(j, "lezioni"), *ingressi = json_campo(j, "ingressi");
    if (!giorni || giorni->tipo != JSON_ELENCO || giorni->n < 1 || giorni->n > GIORNO_SETTIMANA_SZ) {
        fallisci(errore, dimensione, "\"giorni\": serve un elenco da 1 a %d nomi", GIORNO_SETTIMANA_SZ);
        goto fine;
    }
    if (!ore || ore->tipo != JSON_ELENCO || ore->n < 1 || ore->n > ORA_GIORNALIERA_SZ) {
        fallisci(errore, dimensione, "\"ore\": serve un elenco da 1 a %d nomi", ORA_GIORNALIERA_SZ);
        goto fine;
    }
    if (!lezioni || lezioni->tipo != JSON_ELENCO || lezioni->n < 1) {
        fallisci(errore, dimensione, "\"lezioni\": serve un elenco non vuoto");
        goto fine;
    }

    // come inizializza_dati(): prima giorni e ore...
    for (int i = 0; i < giorni->n; i++) {
        if (!nome(giorni->elementi[i], a)) {
            fallisci(errore, dimensione, "giorno %d: nome mancante o piu' lungo di %d caratteri", i + 1, MAX_LEN_WORDS - 1);
            goto fine;
        }
        set_giorno_settimana(a);
    }
    for (int i = 0; i < ore->n; i++) {
        if (!nome(ore->elementi[i], a)) {
            fallisci(errore, dimensione, "ora %d: nome mancante o piu' lungo di %d caratteri", i + 1, MAX_LEN_WORDS - 1);
            goto fine;
        }
        set_ora_giornaliera(a);
    }
    // ...poi la coda...
    queue_mul = malloc(
        sizeof(QueueItem *) * GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ);

    // ...poi le lezioni, nell'ordine del file
    for (int i = 0; i < lezioni->n; i++) {
        const json_t *l = lezioni->elementi[i];
        char cod[MAX_LEN_WORDS];
        int n_ore, n_cod = 0;
        if (!l || l->tipo != JSON_OGGETTO || !nome(json_campo(l, "classe"), a) || !nome(json_campo(l, "docente"), b)
            || !intero(json_campo(l, "ore"), &n_ore) || n_ore < 1) {
            fallisci(errore, dimensione, "lezione %d: servono \"classe\", \"docente\" e \"ore\" (almeno 1)", i + 1);
            goto fine;
        }
        const json_t *c = json_campo(l, "codocente");
        const int con_cod = c && c->tipo != JSON_NULL;
        if (con_cod && (!nome(c, cod) || !intero(json_campo(l, "ore_codocenza"), &n_cod) || n_cod < 1 || n_cod > n_ore
                        || !strcmp(cod, b))) {
            fallisci(errore, dimensione, "lezione %d: con \"codocente\" (diverso dal docente) servono \"ore_codocenza\" "
                     "tra 1 e le ore della lezione", i + 1);
            goto fine;
        }
        // i limiti che le funzioni set_* controllano con assert
        if (p_classe_sezione(a) < 0 && actual_classi_sezione >= CLASSE_SEZIONE_SZ) {
            fallisci(errore, dimensione, "troppe classi (al massimo %d)", CLASSE_SEZIONE_SZ);
            goto fine;
        }
        if ((p_insegnante(b) < 0) + (con_cod && p_insegnante(cod) < 0) + actual_insegnanti > INSEGNANTE_SZ) {
            fallisci(errore, dimensione, "troppi docenti (al massimo %d)", INSEGNANTE_SZ);
            goto fine;
        }
        if (con_cod) set_classe_insegnante_ore_cod(a, b, n_ore, cod, n_cod);
        else set_classe_insegnante_ore(a, b, n_ore);
    }

    // ...poi le ore di ingresso
    if (ingressi && ingressi->tipo != JSON_NULL) {
        if (ingressi->tipo != JSON_ELENCO) {
            fallisci(errore, dimensione, "\"ingressi\": serve un elenco");
            goto fine;
        }
        for (int i = 0; i < ingressi->n; i++) {
            const json_t *e = ingressi->elementi[i];
            int ora;
            if (!e || e->tipo != JSON_OGGETTO || !nome(json_campo(e, "docente"), a) || !intero(json_campo(e, "ora"), &ora)
                || ora < 0 || ora >= actual_ore_giornaliere) {
                fallisci(errore, dimensione, "ingresso %d: servono \"docente\" e \"ora\" (da 0 a %d)", i + 1,
                         actual_ore_giornaliere - 1);
                goto fine;
            }
            if (p_insegnante(a) < 0) {
                fallisci(errore, dimensione, "ingresso %d: il docente \"%s\" non ha lezioni", i + 1, a);
                goto fine;
            }
            ora_di_entrata_prof[p_insegnante(a)] = ora;
        }
    }

    // gli stessi controlli finali di inizializza_dati() (li' sono assert)
    for (int i = 0; i < actual_classi_sezione; i++) {
        if (ore_classi[i] > actual_giorni_settimana * actual_ore_giornaliere) {
            fallisci(errore, dimensione, "la classe %s ha %d ore, piu' degli slot della settimana (%d)",
                     classe_sezione[i], ore_classi[i], actual_giorni_settimana * actual_ore_giornaliere);
            goto fine;
        }
        if (ore_classi[i] < PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana) {
            fallisci(errore, dimensione, "la classe %s ha %d ore, meno delle ore obbligatorie (%d)",
                     classe_sezione[i], ore_classi[i], PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
            goto fine;
        }
    }
    ok = 1;
fine:
    json_libera(j);
    return ok;
}
