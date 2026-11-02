#include "fattibilita.h"

#include <stdio.h>
#include <stdlib.h>

#include "../data_initialization/database_initializers.h"

// Quante lezioni (ore di una classe) possono essere gia' cominciate all'ora k: quelle da solo di un docente entrato
// entro k e quelle in coppia con entrambi i docenti entrati entro k.
static int lezioni_possibili_entro_ora(const int classe, const int k) {
    int n = 0;
    for (int p = 0; p < actual_insegnanti; p++) {
        if (docente_info[p].ora_entrata <= k) n += classe_docente[classe * INSEGNANTE_SZ + p].alone;
        for (int q = p + 1; q < actual_insegnanti; q++) {
            const int ore = codocenza[classe * INSEGNANTE_SZ * INSEGNANTE_SZ + p * INSEGNANTE_SZ + q].ore;
            const int ingresso = docente_info[p].ora_entrata > docente_info[q].ora_entrata
                                     ? docente_info[p].ora_entrata
                                     : docente_info[q].ora_entrata;
            if (ore > 0 && ingresso <= k) n += ore;
        }
    }
    return n;
}

// Ogni classe deve avere una lezione in ognuna delle ore obbligatorie di ogni giorno: alla fine dell'ora k gli slot da
// riempire sono (k + 1) * giorni, e si riempiono solo con lezioni di docenti gia' entrati.
static int classi_hanno_lezioni_a_sufficienza(char *motivo, const size_t dim) {
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int k = 0; k < PRIME_N_ORE_OBBLIGATORIE && k < actual_ore_giornaliere; k++) {
            const int servono = (k + 1) * actual_giorni_settimana;
            const int ci_sono = lezioni_possibili_entro_ora(c, k);
            if (ci_sono < servono) {
                snprintf(motivo, dim, "classe %s: fino all'ora %s servono %d lezioni ma solo %d sono di docenti "
                                      "(o coppie) gia' entrati", classe_sezione[c], ore_giornaliere[k], servono, ci_sono);
                return 0;
            }
        }
    }
    return 1;
}

// A ogni ora obbligatoria tutte le classi fanno lezione insieme, con docenti diversi: servono almeno tanti docenti
// quante sono le classi, gia' entrati a quell'ora.
static int docenti_bastano_per_le_classi(char *motivo, const size_t dim) {
    for (int k = 0; k < PRIME_N_ORE_OBBLIGATORIE && k < actual_ore_giornaliere; k++) {
        int entrati = 0;
        for (int p = 0; p < actual_insegnanti; p++)
            if (docente_info[p].ore > 0 && docente_info[p].ora_entrata <= k) entrati++;
        if (entrati < actual_classi_sezione) {
            snprintf(motivo, dim, "all'ora %s fanno lezione %d classi insieme ma i docenti gia' entrati sono %d",
                     ore_giornaliere[k], actual_classi_sezione, entrati);
            return 0;
        }
    }
    return 1;
}

// Fino all'ora k tutte le classi hanno (k + 1) * giorni slot da riempire, e ogni slot ha almeno un docente. Un docente puo'
// dare al massimo una lezione per ora, dalla sua ora di ingresso, e comunque non piu' delle sue ore settimanali.
static int ore_dei_docenti_bastano_per_gli_slot(char *motivo, const size_t dim) {
    for (int k = 0; k < PRIME_N_ORE_OBBLIGATORIE && k < actual_ore_giornaliere; k++) {
        const int servono = (k + 1) * actual_giorni_settimana * actual_classi_sezione;
        int possono = 0;
        for (int p = 0; p < actual_insegnanti; p++) {
            int capienza = (k - docente_info[p].ora_entrata + 1) * actual_giorni_settimana;
            if (capienza < 0) capienza = 0;
            possono += capienza < docente_info[p].ore ? capienza : docente_info[p].ore;
        }
        if (possono < servono) {
            snprintf(motivo, dim, "fino all'ora %s servono %d lezioni (tutte le classi) ma i docenti possono darne al "
                                  "massimo %d", ore_giornaliere[k], servono, possono);
            return 0;
        }
    }
    return 1;
}

// Le ore settimanali di un docente devono stare nelle ore che gli restano, dalla sua ora di ingresso.
static int ore_dei_docenti_stanno_nella_settimana(char *motivo, const size_t dim) {
    for (int p = 0; p < actual_insegnanti; p++) {
        const int disponibili = (actual_ore_giornaliere - docente_info[p].ora_entrata) * actual_giorni_settimana;
        if (docente_info[p].ore > disponibili) {
            snprintf(motivo, dim, "docente %s: %d ore a settimana ma entrando all'ora %s ne ha al massimo %d",
                     insegnanti[p], docente_info[p].ore, ore_giornaliere[docente_info[p].ora_entrata], disponibili);
            return 0;
        }
    }
    return 1;
}

/* ---- flusso massimo: le classi, nelle ore obbligatorie, trovano abbastanza lezioni? ---- */

// Rete: sorgente -> classe (slot da riempire) -> gruppo di lezioni (classe, docente) o (classe, coppia) -> docente
// -> pozzo (ore che il docente puo' dare). Una lezione in coppia occupa anche l'altro docente, ma qui si addebita a uno
// solo dei due: e' un rilassamento, quindi un flusso troppo piccolo prova che il caso e' impossibile.
typedef struct {
    int n_nodi, n_archi;
    int *testa, *prossimo, *verso, *capacita; // liste di adiacenza; l'arco i ha come inverso i ^ 1
} Rete;

static void rete_inizia(Rete *r, const int n_nodi, const int max_archi) {
    r->n_nodi = n_nodi;
    r->n_archi = 0;
    r->testa = malloc(sizeof(int) * n_nodi);
    for (int i = 0; i < n_nodi; i++) r->testa[i] = -1;
    r->prossimo = malloc(sizeof(int) * max_archi * 2);
    r->verso = malloc(sizeof(int) * max_archi * 2);
    r->capacita = malloc(sizeof(int) * max_archi * 2);
}

static void rete_libera(Rete *r) {
    free(r->testa);
    free(r->prossimo);
    free(r->verso);
    free(r->capacita);
}

static void rete_arco(Rete *r, const int da, const int a, const int capacita) {
    r->verso[r->n_archi] = a;
    r->capacita[r->n_archi] = capacita;
    r->prossimo[r->n_archi] = r->testa[da];
    r->testa[da] = r->n_archi++;
    r->verso[r->n_archi] = da;
    r->capacita[r->n_archi] = 0;
    r->prossimo[r->n_archi] = r->testa[a];
    r->testa[a] = r->n_archi++;
}

// Flusso massimo con cammini minimi (Edmonds-Karp).
static int rete_flusso_massimo(Rete *r, const int sorgente, const int pozzo) {
    int flusso = 0;
    int *arco_entrante = malloc(sizeof(int) * r->n_nodi);
    int *coda = malloc(sizeof(int) * r->n_nodi);
    for (;;) {
        for (int i = 0; i < r->n_nodi; i++) arco_entrante[i] = -2;
        arco_entrante[sorgente] = -1;
        int testa = 0, fine = 0;
        coda[fine++] = sorgente;
        while (testa < fine && arco_entrante[pozzo] == -2) {
            const int u = coda[testa++];
            for (int e = r->testa[u]; e >= 0; e = r->prossimo[e]) {
                if (r->capacita[e] > 0 && arco_entrante[r->verso[e]] == -2) {
                    arco_entrante[r->verso[e]] = e;
                    coda[fine++] = r->verso[e];
                }
            }
        }
        if (arco_entrante[pozzo] == -2) break;
        int quanto = 1 << 30;
        for (int v = pozzo; v != sorgente; v = r->verso[arco_entrante[v] ^ 1])
            if (r->capacita[arco_entrante[v]] < quanto) quanto = r->capacita[arco_entrante[v]];
        for (int v = pozzo; v != sorgente; v = r->verso[arco_entrante[v] ^ 1]) {
            r->capacita[arco_entrante[v]] -= quanto;
            r->capacita[arco_entrante[v] ^ 1] += quanto;
        }
        flusso += quanto;
    }
    free(arco_entrante);
    free(coda);
    return flusso;
}

static int minimo(const int a, const int b) { return a < b ? a : b; }

// Nelle ore da a a b (indici, entrambe obbligatorie) ogni classe ha (b - a + 1) * giorni slot da riempire. Un docente
// puo' dare al massimo una lezione per ora (dalla sua ora di ingresso) e, in una classe, al massimo
// MAX_ORE_PROF_IN_UNA_CLASSE ore al giorno. Ritorna quante lezioni servono e quante al massimo si possono avere.
static int flusso_nella_finestra(const int da, const int a, int *servono) {
    const int giorni = actual_giorni_settimana, larghezza = a - da + 1;
    const int per_classe = larghezza * giorni;
    const int max_in_classe = minimo(MAX_ORE_PROF_IN_UNA_CLASSE, larghezza) * giorni;
    const int C = actual_classi_sezione, T = actual_insegnanti;

    // i gruppi di lezioni: (classe, docente da solo) e (classe, coppia), con almeno un'ora e docenti entrati entro a
    int max_gruppi = C * T;
    for (int c = 0; c < C; c++)
        for (int p = 0; p < T; p++)
            for (int q = p + 1; q < T; q++)
                if (codocenza[c * INSEGNANTE_SZ * INSEGNANTE_SZ + p * INSEGNANTE_SZ + q].ore > 0) max_gruppi++;

    const int sorgente = 0, pozzo = 1, prima_classe = 2, primo_docente = prima_classe + C, primo_gruppo = primo_docente + T;
    Rete r;
    rete_inizia(&r, primo_gruppo + max_gruppi, C + T + max_gruppi * 4);
    for (int c = 0; c < C; c++) rete_arco(&r, sorgente, prima_classe + c, per_classe);
    for (int p = 0; p < T; p++) {
        const int da_dove = docente_info[p].ora_entrata > da ? docente_info[p].ora_entrata : da;
        int capienza = a - da_dove + 1;
        if (capienza < 0) capienza = 0;
        rete_arco(&r, primo_docente + p, pozzo, minimo(capienza * giorni, docente_info[p].ore));
    }
    int g = 0;
    for (int c = 0; c < C; c++) {
        for (int p = 0; p < T; p++) {
            const int alone = classe_docente[c * INSEGNANTE_SZ + p].alone;
            if (alone > 0 && docente_info[p].ora_entrata <= a) {
                rete_arco(&r, prima_classe + c, primo_gruppo + g, minimo(alone, max_in_classe));
                rete_arco(&r, primo_gruppo + g, primo_docente + p, 1 << 20);
                g++;
            }
            for (int q = p + 1; q < T; q++) {
                const int ore = codocenza[c * INSEGNANTE_SZ * INSEGNANTE_SZ + p * INSEGNANTE_SZ + q].ore;
                if (ore > 0 && docente_info[p].ora_entrata <= a && docente_info[q].ora_entrata <= a) {
                    rete_arco(&r, prima_classe + c, primo_gruppo + g, minimo(ore, max_in_classe));
                    rete_arco(&r, primo_gruppo + g, primo_docente + p, 1 << 20);
                    rete_arco(&r, primo_gruppo + g, primo_docente + q, 1 << 20);
                    g++;
                }
            }
        }
    }
    const int flusso = rete_flusso_massimo(&r, sorgente, pozzo);
    rete_libera(&r);
    *servono = per_classe * C;
    return flusso;
}

static int lezioni_trovano_docenti_nelle_ore_obbligatorie(char *motivo, const size_t dim) {
    const int ultima = minimo(PRIME_N_ORE_OBBLIGATORIE, actual_ore_giornaliere) - 1;
    for (int da = 0; da <= ultima; da++) {
        for (int a = da; a <= ultima; a++) {
            int servono;
            const int possibili = flusso_nella_finestra(da, a, &servono);
            if (possibili < servono) {
                snprintf(motivo, dim, "dalle ore %s alle %s le classi hanno bisogno di %d lezioni, ma tenendo conto di "
                                      "ingressi, ore dei docenti e massimo giornaliero per classe se ne possono avere "
                                      "al massimo %d", ore_giornaliere[da], ore_giornaliere[a], servono, possibili);
                return 0;
            }
        }
    }
    return 1;
}

int verifica_fattibilita(char *motivo, const size_t dimensione) {
    return ore_dei_docenti_stanno_nella_settimana(motivo, dimensione)
           && docenti_bastano_per_le_classi(motivo, dimensione)
           && ore_dei_docenti_bastano_per_gli_slot(motivo, dimensione)
           && classi_hanno_lezioni_a_sufficienza(motivo, dimensione)
           && lezioni_trovano_docenti_nelle_ore_obbligatorie(motivo, dimensione);
}
