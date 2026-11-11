#include "generatore.h"
#include "database_initializers.h"
#include "nomi_docenti.h"
#include "../json.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ORE_CLASSE_MIN 30   // max(29, ore obbligatorie della settimana)
#define ORE_CLASSE_MAX 36
#define ORE_DOCENTE_MIN 5
#define ORE_DOCENTE_MAX 24
#define ORA_INGRESSO_MAX 3
#define MAX_LEZIONI 4000
#define MAX_TENTATIVI 20000
#define DA_SCEGLIERE (-2)   // la lezione avra' un codocente, non ancora scelto

// Un caso generato: lo stato dei numeri casuali, le lezioni, quanti docenti e le ore di ognuno.
typedef struct {
    int classe, docente, ore, codocente, ore_codocenza;  // codocente -1: nessuno
} Lezione;

typedef struct {
    uint64_t stato;
    Lezione lezioni[MAX_LEZIONI];
    int n_lezioni, n_docenti, carico[INSEGNANTE_SZ];
} Generatore;

// ---- i numeri casuali: xorshift64*, come _Casuale in sito/generatore.py ----
static uint64_t prossimo(Generatore *gen) {
    uint64_t s = gen->stato;
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    gen->stato = s;
    return s * 2685821657736338717ULL;
}

static void semina(Generatore *gen, const unsigned long long int seme) {
    gen->stato = (uint64_t) seme * 0x9E3779B97F4A7C15ULL + 0x1234567ULL;
    if (!gen->stato) gen->stato = 1;
    for (int i = 0; i < 8; i++) prossimo(gen);
}

// un numero da lo a hi compresi
static int tra(Generatore *gen, const int lo, const int hi) {
    return lo + (int) (prossimo(gen) % (uint64_t) (hi - lo + 1));
}

// ---- un tentativo ----

// il meno carico fra tre docenti presi a caso (diversi da escluso e che restano entro ORE_DOCENTE_MAX), o -1
static int scegli(Generatore *gen, const int ore, const int escluso) {
    int migliore = -1;
    for (int k = 0; k < 3; k++) {
        const int d = tra(gen, 0, gen->n_docenti - 1);
        if (d == escluso || gen->carico[d] + ore > ORE_DOCENTE_MAX) continue;
        if (migliore < 0 || gen->carico[d] < gen->carico[migliore]) migliore = d;
    }
    return migliore;
}

static int tenta(Generatore *gen, const int n_classi) {
    gen->n_lezioni = 0;
    const int p_cod = tra(gen, 15, 60);
    int ore_totali = 0;
    for (int c = 0; c < n_classi; c++) {
        int resto = tra(gen, ORE_CLASSE_MIN, ORE_CLASSE_MAX);
        while (resto > 0) {
            if (gen->n_lezioni == MAX_LEZIONI) return 0;
            int ore = tra(gen, 1, 6);
            if (ore > resto) ore = resto;
            resto -= ore;
            Lezione *l = &gen->lezioni[gen->n_lezioni++];
            *l = (Lezione){c, -1, ore, -1, 0};
            ore_totali += ore;
            if (tra(gen, 1, 100) <= p_cod) {
                l->ore_codocenza = tra(gen, 1, ore);
                ore_totali += l->ore_codocenza;
                l->codocente = DA_SCEGLIERE;
            }
        }
    }
    // quanti docenti: in media tra 10 e 20 ore, e comunque tra 5 e 24
    int lo = (ore_totali + 19) / 20, hi = ore_totali / 10;
    const int abs_lo = (ore_totali + ORE_DOCENTE_MAX - 1) / ORE_DOCENTE_MAX, abs_hi = ore_totali / ORE_DOCENTE_MIN;
    if (abs_lo > lo) lo = abs_lo;
    if (abs_hi < hi) hi = abs_hi;
    if (lo > hi) {
        lo = abs_lo;
        hi = abs_hi;
    }
    if (lo > hi || hi >= INSEGNANTE_SZ) return 0;
    gen->n_docenti = tra(gen, lo, hi);
    for (int d = 0; d < gen->n_docenti; d++) gen->carico[d] = 0;
    for (int i = 0; i < gen->n_lezioni; i++) {  // le lezioni in ordine casuale (Fisher-Yates)
        const int j = tra(gen, i, gen->n_lezioni - 1);
        const Lezione t = gen->lezioni[i];
        gen->lezioni[i] = gen->lezioni[j];
        gen->lezioni[j] = t;
    }
    int con_cod = 0, da_soli = 0;
    for (int i = 0; i < gen->n_lezioni; i++) {
        Lezione *l = &gen->lezioni[i];
        l->docente = scegli(gen, l->ore, -1);
        if (l->docente < 0) return 0;
        gen->carico[l->docente] += l->ore;
        if (l->codocente == DA_SCEGLIERE) {
            l->codocente = scegli(gen, l->ore_codocenza, l->docente);
            if (l->codocente < 0) return 0;
            gen->carico[l->codocente] += l->ore_codocenza;
        }
        if (l->codocente >= 0) con_cod = 1;
        else da_soli = 1;
    }
    if (!con_cod || !da_soli) return 0;
    for (int d = 0; d < gen->n_docenti; d++) if (gen->carico[d] < ORE_DOCENTE_MIN) return 0;
    return 1;
}

static void nome_docente(const int d, char out[MAX_LEN_WORDS]) {
    if (d < N_NOMI_DOCENTI_CASUALI) snprintf(out, MAX_LEN_WORDS, "%s", NOMI_DOCENTI_CASUALI[d]);
    else snprintf(out, MAX_LEN_WORDS, "D%03d", d + 1);
}

// Genera il caso in gen e lo registra nella scuola s. Ritorna 0 se non riesce.
static int genera(Generatore *gen, Scuola *s, const unsigned long long int seme, const int n_classi,
                  const int con_ritardi) {
    if (n_classi < 1 || n_classi > CLASSE_SEZIONE_SZ) return 0;
    semina(gen, seme);
    int ok = 0;
    for (int t = 0; t < MAX_TENTATIVI && !ok; t++) ok = tenta(gen, n_classi);
    if (!ok) return 0;

    // come inizializza_dati(): giorni e ore, le lezioni (classe per classe), le ore di ingresso
    char giorni[][4] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab"}, ore[][3] = {"08", "09", "10", "11", "12", "13"};
    for (int g = 0; g < 6; g++) set_giorno_settimana(s, giorni[g]);
    for (int h = 0; h < 6; h++) set_ora_giornaliera(s, ore[h]);
    for (int c = 0; c < n_classi; c++)
        for (int i = 0; i < gen->n_lezioni; i++) {
            const Lezione *l = &gen->lezioni[i];
            if (l->classe != c) continue;
            char classe[MAX_LEN_WORDS], docente[MAX_LEN_WORDS], codocente[MAX_LEN_WORDS];
            snprintf(classe, sizeof classe, "C%02d", c + 1);
            nome_docente(l->docente, docente);
            if (l->codocente < 0) set_classe_insegnante_ore(s, classe, docente, l->ore);
            else {
                nome_docente(l->codocente, codocente);
                set_classe_insegnante_ore_cod(s, classe, docente, l->ore, codocente, l->ore_codocenza);
            }
        }
    if (con_ritardi) {
        // quanti (da 1 a tutti) e quali: i primi `quanti` di una permutazione dei docenti; poi in ordine di indice
        int docenti[INSEGNANTE_SZ], ritardo[INSEGNANTE_SZ] = {0};
        for (int d = 0; d < gen->n_docenti; d++) docenti[d] = d;
        const int quanti = tra(gen, 1, gen->n_docenti);
        for (int i = 0; i < quanti; i++) {
            const int j = tra(gen, i, gen->n_docenti - 1);
            const int t = docenti[i];
            docenti[i] = docenti[j];
            docenti[j] = t;
        }
        for (int i = 0; i < quanti; i++) ritardo[docenti[i]] = 1;
        for (int d = 0; d < gen->n_docenti; d++) {
            if (!ritardo[d]) continue;
            int ingresso = tra(gen, 1, ORA_INGRESSO_MAX);
            while (ingresso > 0 && gen->carico[d] > (6 - ingresso) * 6) ingresso--;
            char nome[MAX_LEN_WORDS];
            nome_docente(d, nome);
            if (ingresso > 0) s->ora_di_entrata_prof[p_insegnante(s, nome)] = ingresso;
        }
    }
    return 1;
}

int genera_dati_casuali(Scuola *s, const unsigned long long int seme, const int n_classi, const int con_ritardi) {
    Generatore *gen = malloc(sizeof(Generatore));
    assert(gen);
    const int ok = genera(gen, s, seme, n_classi, con_ritardi);
    free(gen);
    return ok;
}

// Il caso gen in JSON su stdout: le lezioni come sono state generate (classe, docente, ore e, se c'e', codocente e ore
// insieme), classe per classe, e le ore di ingresso di chi entra tardi.
static void stampa(const Generatore *gen, const Scuola *s) {
    printf("{\"giorni\":[");
    for (int g = 0; g < s->actual_giorni_settimana; g++) {
        if (g) putchar(',');
        json_stringa(stdout, s->giorni_settimana[g]);
    }
    printf("],\"ore\":[");
    for (int h = 0; h < s->actual_ore_giornaliere; h++) {
        if (h) putchar(',');
        json_stringa(stdout, s->ore_giornaliere[h]);
    }
    printf("],\"lezioni\":[");
    char nome[MAX_LEN_WORDS];
    int scritte = 0;
    for (int c = 0; c < s->actual_classi_sezione; c++)
        for (int i = 0; i < gen->n_lezioni; i++) {
            const Lezione *l = &gen->lezioni[i];
            if (l->classe != c) continue;
            printf("%s{\"classe\":", scritte++ ? "," : "");
            json_stringa(stdout, s->classe_sezione[c]);
            printf(",\"docente\":");
            nome_docente(l->docente, nome);
            json_stringa(stdout, nome);
            printf(",\"ore\":%d", l->ore);
            if (l->codocente >= 0) {
                printf(",\"codocente\":");
                nome_docente(l->codocente, nome);
                json_stringa(stdout, nome);
                printf(",\"ore_codocenza\":%d", l->ore_codocenza);
            }
            putchar('}');
        }
    printf("],\"ingressi\":[");
    scritte = 0;
    for (int d = 0; d < gen->n_docenti; d++) {
        nome_docente(d, nome);
        const int ora = s->ora_di_entrata_prof[p_insegnante(s, nome)];
        if (!ora) continue;
        printf("%s{\"docente\":", scritte++ ? "," : "");
        json_stringa(stdout, nome);
        printf(",\"ora\":%d}", ora);
    }
    printf("]}\n");
}

int stampa_dati_casuali(const unsigned long long int seme, const int n_classi, const int con_ritardi) {
    Generatore *gen = malloc(sizeof(Generatore));
    Scuola *s = nuova_scuola();
    assert(gen);
    const int ok = genera(gen, s, seme, n_classi, con_ritardi);
    if (ok) stampa(gen, s);
    libera_scuola(s);
    free(gen);
    return ok;
}
