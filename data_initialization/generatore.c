#include "generatore.h"
#include "database_initializers.h"
#include "nomi_docenti.h"
#include "../priority_queue/prio_queue.h"

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

// ---- i numeri casuali: xorshift64*, come _Casuale in sito/generatore.py ----
static uint64_t stato;

static uint64_t prossimo(void) {
    uint64_t s = stato;
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    stato = s;
    return s * 2685821657736338717ULL;
}

static void semina(const unsigned long long int seme) {
    stato = (uint64_t) seme * 0x9E3779B97F4A7C15ULL + 0x1234567ULL;
    if (!stato) stato = 1;
    for (int i = 0; i < 8; i++) prossimo();
}

// un numero da lo a hi compresi
static int tra(const int lo, const int hi) {
    return lo + (int) (prossimo() % (uint64_t) (hi - lo + 1));
}

// ---- un tentativo ----
typedef struct {
    int classe, docente, ore, codocente, ore_codocenza;  // codocente -1: nessuno
} Lezione;

static Lezione lezioni[MAX_LEZIONI];
static int n_lezioni, n_docenti, carico[INSEGNANTE_SZ];

// il meno carico fra tre docenti presi a caso (diversi da escluso e che restano entro ORE_DOCENTE_MAX), o -1
static int scegli(const int ore, const int escluso) {
    int migliore = -1;
    for (int k = 0; k < 3; k++) {
        const int d = tra(0, n_docenti - 1);
        if (d == escluso || carico[d] + ore > ORE_DOCENTE_MAX) continue;
        if (migliore < 0 || carico[d] < carico[migliore]) migliore = d;
    }
    return migliore;
}

static int tenta(const int n_classi) {
    n_lezioni = 0;
    const int p_cod = tra(15, 60);
    int ore_totali = 0;
    for (int c = 0; c < n_classi; c++) {
        int resto = tra(ORE_CLASSE_MIN, ORE_CLASSE_MAX);
        while (resto > 0) {
            if (n_lezioni == MAX_LEZIONI) return 0;
            int ore = tra(1, 6);
            if (ore > resto) ore = resto;
            resto -= ore;
            Lezione *l = &lezioni[n_lezioni++];
            *l = (Lezione){c, -1, ore, -1, 0};
            ore_totali += ore;
            if (tra(1, 100) <= p_cod) {
                l->ore_codocenza = tra(1, ore);
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
    n_docenti = tra(lo, hi);
    for (int d = 0; d < n_docenti; d++) carico[d] = 0;
    for (int i = 0; i < n_lezioni; i++) {  // le lezioni in ordine casuale (Fisher-Yates)
        const int j = tra(i, n_lezioni - 1);
        const Lezione t = lezioni[i];
        lezioni[i] = lezioni[j];
        lezioni[j] = t;
    }
    int con_cod = 0, da_soli = 0;
    for (int i = 0; i < n_lezioni; i++) {
        Lezione *l = &lezioni[i];
        l->docente = scegli(l->ore, -1);
        if (l->docente < 0) return 0;
        carico[l->docente] += l->ore;
        if (l->codocente == DA_SCEGLIERE) {
            l->codocente = scegli(l->ore_codocenza, l->docente);
            if (l->codocente < 0) return 0;
            carico[l->codocente] += l->ore_codocenza;
        }
        if (l->codocente >= 0) con_cod = 1;
        else da_soli = 1;
    }
    if (!con_cod || !da_soli) return 0;
    for (int d = 0; d < n_docenti; d++) if (carico[d] < ORE_DOCENTE_MIN) return 0;
    return 1;
}

static void nome_docente(const int d, char out[MAX_LEN_WORDS]) {
    if (d < N_NOMI_DOCENTI_CASUALI) snprintf(out, MAX_LEN_WORDS, "%s", NOMI_DOCENTI_CASUALI[d]);
    else snprintf(out, MAX_LEN_WORDS, "D%03d", d + 1);
}

int genera_dati_casuali(const unsigned long long int seme, const int n_classi, const int con_ritardi) {
    if (n_classi < 1 || n_classi > CLASSE_SEZIONE_SZ) return 0;
    semina(seme);
    int ok = 0;
    for (int t = 0; t < MAX_TENTATIVI && !ok; t++) ok = tenta(n_classi);
    if (!ok) return 0;

    // come inizializza_dati(): giorni e ore, la coda, le lezioni (classe per classe), le ore di ingresso
    char giorni[][4] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab"}, ore[][3] = {"08", "09", "10", "11", "12", "13"};
    for (int g = 0; g < 6; g++) set_giorno_settimana(giorni[g]);
    for (int h = 0; h < 6; h++) set_ora_giornaliera(ore[h]);
    queue_mul = malloc(sizeof(QueueItem *) * GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ *
                       INSEGNANTE_SZ);
    for (int c = 0; c < n_classi; c++)
        for (int i = 0; i < n_lezioni; i++) {
            const Lezione *l = &lezioni[i];
            if (l->classe != c) continue;
            char classe[MAX_LEN_WORDS], docente[MAX_LEN_WORDS], codocente[MAX_LEN_WORDS];
            snprintf(classe, sizeof classe, "C%02d", c + 1);
            nome_docente(l->docente, docente);
            if (l->codocente < 0) set_classe_insegnante_ore(classe, docente, l->ore);
            else {
                nome_docente(l->codocente, codocente);
                set_classe_insegnante_ore_cod(classe, docente, l->ore, codocente, l->ore_codocenza);
            }
        }
    if (con_ritardi) {
        // quanti (da 1 a tutti) e quali: i primi `quanti` di una permutazione dei docenti; poi in ordine di indice
        int docenti[INSEGNANTE_SZ], ritardo[INSEGNANTE_SZ] = {0};
        for (int d = 0; d < n_docenti; d++) docenti[d] = d;
        const int quanti = tra(1, n_docenti);
        for (int i = 0; i < quanti; i++) {
            const int j = tra(i, n_docenti - 1);
            const int t = docenti[i];
            docenti[i] = docenti[j];
            docenti[j] = t;
        }
        for (int i = 0; i < quanti; i++) ritardo[docenti[i]] = 1;
        for (int d = 0; d < n_docenti; d++) {
            if (!ritardo[d]) continue;
            int ingresso = tra(1, ORA_INGRESSO_MAX);
            while (ingresso > 0 && carico[d] > (6 - ingresso) * 6) ingresso--;
            char nome[MAX_LEN_WORDS];
            nome_docente(d, nome);
            if (ingresso > 0) ora_di_entrata_prof[p_insegnante(nome)] = ingresso;
        }
    }
    return 1;
}
