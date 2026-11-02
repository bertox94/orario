#include "generatore.h"
#include "database_initializers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Generatore xorshift64*: stessa sequenza su ogni piattaforma (rand() no).
static unsigned long long rng_state;

static unsigned long long rng_next(void) {
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 2685821657736338717ULL;
}

// intero uniforme in [lo, hi]
static int rnd(const int lo, const int hi) {
    return lo + (int) (rng_next() % (unsigned long long) (hi - lo + 1));
}

#define MAX_LEZIONI 4000
#define MAX_TENTATIVI 20000

typedef struct {
    int classe;
    int prof;
    int ore;
    int cod;     // -1 se senza codocente
    int ore_cod; // ore in cui il codocente e' presente (<= ore)
} lezione_t;

static lezione_t lezioni[MAX_LEZIONI];
static int n_lezioni;
static int carico[INSEGNANTE_SZ];

// Sceglie il docente meno carico tra 3 candidati casuali che ci stia ancora con `ore` ore (!= escluso).
static int scegli_docente(const int n_doc, const int ore, const int escluso) {
    int best = -1;
    for (int k = 0; k < 3; k++) {
        const int d = rnd(0, n_doc - 1);
        if (d == escluso || carico[d] + ore > GEN_ORE_DOCENTE_MAX) continue;
        if (best < 0 || carico[d] < carico[best]) best = d;
    }
    return best;
}

// Un tentativo: true se il piano rispetta tutti i vincoli.
static int tenta(const int n_classi, int *n_doc_usati) {
    int min_classe = GEN_ORE_CLASSE_MIN;
    if (min_classe < PRIME_N_ORE_OBBLIGATORIE * 6) min_classe = PRIME_N_ORE_OBBLIGATORIE * 6;

    n_lezioni = 0;
    const int p_cod = rnd(15, 60); // percentuale di lezioni con codocente, diversa a ogni tentativo
    int ore_totali = 0;
    for (int c = 0; c < n_classi; c++) {
        int resto = rnd(min_classe, GEN_ORE_CLASSE_MAX);
        while (resto > 0) {
            if (n_lezioni == MAX_LEZIONI) return 0;
            int ore = rnd(1, 6);
            if (ore > resto) ore = resto;
            resto -= ore;
            lezione_t *l = &lezioni[n_lezioni++];
            l->classe = c;
            l->prof = -1;
            l->ore = ore;
            l->cod = -1;
            l->ore_cod = 0;
            ore_totali += ore;
            if (rnd(1, 100) <= p_cod) {
                l->ore_cod = rnd(1, ore);
                ore_totali += l->ore_cod;
                l->cod = 0; // da assegnare
            }
        }
    }

    // numero di docenti: media di ore tra 10 e 20, se possibile; altrimenti qualunque numero ammesso
    int lo = (ore_totali + 19) / 20, hi = ore_totali / 10;
    const int abs_lo = (ore_totali + GEN_ORE_DOCENTE_MAX - 1) / GEN_ORE_DOCENTE_MAX;
    const int abs_hi = ore_totali / GEN_ORE_DOCENTE_MIN;
    if (lo < abs_lo) lo = abs_lo;
    if (hi > abs_hi) hi = abs_hi;
    if (lo > hi) {
        lo = abs_lo;
        hi = abs_hi;
    }
    if (lo > hi || hi >= INSEGNANTE_SZ) return 0;
    const int n_doc = rnd(lo, hi);

    for (int d = 0; d < n_doc; d++) carico[d] = 0;
    // le lezioni lunghe per prime: sono le piu' difficili da sistemare
    for (int i = 0; i < n_lezioni; i++) {
        const int j = rnd(i, n_lezioni - 1);
        const lezione_t t = lezioni[i];
        lezioni[i] = lezioni[j];
        lezioni[j] = t;
    }
    for (int i = 0; i < n_lezioni; i++) {
        lezione_t *l = &lezioni[i];
        l->prof = scegli_docente(n_doc, l->ore, -1);
        if (l->prof < 0) return 0;
        carico[l->prof] += l->ore;
        if (l->cod == 0) {
            l->cod = scegli_docente(n_doc, l->ore_cod, l->prof);
            if (l->cod < 0) return 0;
            carico[l->cod] += l->ore_cod;
        }
    }

    int con_cod = 0, senza_cod = 0;
    for (int i = 0; i < n_lezioni; i++) {
        if (lezioni[i].cod >= 0) con_cod++; else senza_cod++;
    }
    if (!con_cod || !senza_cod) return 0;
    for (int d = 0; d < n_doc; d++)
        if (carico[d] < GEN_ORE_DOCENTE_MIN) return 0;
    *n_doc_usati = n_doc;
    return 1;
}

void genera_dati_casuali(const unsigned long long seed, const int n_classi) {
    assert(n_classi >= 1 && n_classi <= CLASSE_SEZIONE_SZ);
    rng_state = seed * 0x9E3779B97F4A7C15ULL + 0x1234567ULL;
    if (!rng_state) rng_state = 1;
    for (int i = 0; i < 8; i++) rng_next();

    int n_doc = 0;
    int ok = 0;
    for (int t = 0; t < MAX_TENTATIVI && !ok; t++) ok = tenta(n_classi, &n_doc);
    if (!ok) {
        fprintf(stderr, "Impossibile generare dati validi per %d classi\n", n_classi);
        exit(1);
    }

    // ordine di apparizione: per classe, nell'ordine generato
    for (int c = 0; c < n_classi; c++) {
        char cla[MAX_LEN_WORDS];
        snprintf(cla, sizeof cla, "C%02d", c + 1);
        for (int i = 0; i < n_lezioni; i++) {
            const lezione_t *l = &lezioni[i];
            if (l->classe != c) continue;
            char prof[MAX_LEN_WORDS], cod[MAX_LEN_WORDS];
            snprintf(prof, sizeof prof, "D%03d", l->prof + 1);
            if (l->cod < 0) {
                set_classe_insegnante_ore(cla, prof, l->ore);
            } else {
                snprintf(cod, sizeof cod, "D%03d", l->cod + 1);
                set_classe_insegnante_ore_cod(cla, prof, l->ore, cod, l->ore_cod);
            }
        }
    }

    for (int i = 0; i < actual_classi_sezione; i++) {
        assert(classe_info[i].ore >= PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
        assert(classe_info[i].ore <= GEN_ORE_CLASSE_MAX);
    }
}
