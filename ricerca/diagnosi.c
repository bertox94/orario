#include "diagnosi.h"

#include <stdio.h>
#include <time.h>

#include "../priority_queue/prio_queue.h"
#include "vincoli.h"

long long int nodi = 0;
int pos_max = -1;
long long int nodi_a_pos_max = 0;
double limite_secondi = 0;
double inizio_ricerca;
int interrotto_per_tempo = 0;

#define N_SLOT (GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ) // slot possibili, al massimo

static int registra_motivi = 0;
static long long int marce_per_pos[N_SLOT];                // marce indietro per slot
static long long int motivi_per_pos[N_SLOT][N_MOTIVI];     // motivi di scarto per slot
static long long int docenti_per_pos[N_SLOT];              // docenti con ore da fare, sommati
static long long int motivi_per_docente[INSEGNANTE_SZ][N_MOTIVI];
static long long int scartati_per_pos[N_SLOT][N_MOTIVI_SCARTO]; // rami scartati dopo lo slot, per motivo
static long long int scartati[N_MOTIVI_SCARTO];

static const char *nome_motivo_scarto[N_MOTIVI_SCARTO] = {
    "alla classe non bastano piu' le lezioni per gli slot da riempire",
    "le ore che mancano a un docente non stanno piu' nel tempo rimasto"
};

// Descrizione di ogni motivo, nello stesso ordine dei numeri restituiti da motivo_di_scarto.
static const char *nome_motivo[N_MOTIVI] = {
    "occupato in un'altra classe alla stessa ora",
    "non ancora entrato (ora di ingresso)",
    "gia' al massimo di ore in questa classe oggi",
    "ore non contigue",
    "gia' al massimo di classi diverse oggi",
    "solo ore in codocenza e nessun codocente utilizzabile",
    "bloccato dalla regola del sabato"
};

// Tempo reale (non tempo di CPU) in secondi, da un punto di partenza qualunque: serve solo per differenze.
double secondi_reali(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void diagnosi_imposta(const double secondi, const int con_motivi) {
    limite_secondi = secondi;
    registra_motivi = con_motivi;
}

void diagnosi_avvia(void) {
    inizio_ricerca = secondi_reali();
}

/* ---- registrazione ---- */

// Primo motivo per cui il docente p, che in questa classe ha ancora ore da fare, non e' stato messo nello slot.
// Ripete gli stessi controlli della ricerca, nello stesso ordine; e' chiamata solo a ogni marcia indietro e solo
// con --motivi, quindi il costo non pesa sulla ricerca normale.
static int motivo_di_scarto(const teaching_slot *slot, const int pos, const int p, const int base4d_queue,
                            const int queue_size) {
    if (!prof_è_disponibile(slot->base3d_goi, p)) return 0;
    if (!verifica_ora_ingresso(p, slot->ora)) return 1;
    if (!prof_fa_al_max_n_ore(slot, p)) return 2;
    if (!prof_fa_ore_contigue(slot, p)) return 3;
    if (!ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, slot->giorno, slot->classe, p)) return 4;
    // idoneo e con ore da solo: l'unico controllo rimasto che puo' averlo scartato e' la regola del sabato
    if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(slot->base2d_ci, p)) return 6;
    // idoneo e senza ore da solo: il motivo e' la mancanza di un codocente, a meno che esista un partner utilizzabile
    // che la regola del sabato scarta
    for (int jj = base4d_queue; jj < queue_size; jj++) {
        const int q = queue_mul[jj]->teacher;
        if (q != p && prof_è_disponibile(slot->base3d_goi, q)
            && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p, q)
            && docente_idoneo(slot, pos, q))
            return 6;
    }
    return 5;
}

void diagnosi_ramo_scartato(const int pos, const int motivo) {
    scartati_per_pos[pos][motivo]++;
    scartati[motivo]++;
}

// Conta la marcia indietro nello slot e, con --motivi, perche' ogni docente con ore da fare nella classe e' stato
// scartato (per lo slot e per il docente).
void diagnosi_marcia_indietro(const teaching_slot *slot, const int pos, const int base4d_queue, const int queue_size) {
    marce_per_pos[pos]++;
    if (!registra_motivi) return;
    for (int ii = base4d_queue; ii < queue_size; ii++) {
        const int p = queue_mul[ii]->teacher;
        if (!deve_ancora_svolgere_ore_in_questa_classe(slot->base2d_ci, p)) continue;
        const int m = motivo_di_scarto(slot, pos, p, base4d_queue, queue_size);
        motivi_per_pos[pos][m]++;
        motivi_per_docente[p][m]++;
        docenti_per_pos[pos]++;
    }
}

/* ---- lettura dei contatori ---- */

long long int diagnosi_marce_nello_slot(const int pos) { return marce_per_pos[pos]; }

long long int diagnosi_scartati_nello_slot(const int pos, const int motivo_scarto) {
    return scartati_per_pos[pos][motivo_scarto];
}

long long int diagnosi_motivo_nello_slot(const int pos, const int motivo) { return motivi_per_pos[pos][motivo]; }

long long int diagnosi_motivo_del_docente(const int prof, const int motivo) { return motivi_per_docente[prof][motivo]; }

int diagnosi_con_motivi(void) { return registra_motivi; }

const char *diagnosi_nome_motivo(const int motivo) { return nome_motivo[motivo]; }

const char *diagnosi_nome_motivo_scarto(const int motivo_scarto) { return nome_motivo_scarto[motivo_scarto]; }

/* ---- stampa ---- */

static void stampa_nome_slot(const int pos) {
    const teaching_slot *s = &orario[pos];
    printf("giorno %s, ora %s, classe %s", giorni_settimana[s->giorno], ore_giornaliere[s->ora],
           classe_sezione[s->classe]);
}

// Quanto si e' esplorato e fin dove si e' arrivati. Se la posizione piu' avanzata e' stata raggiunta presto (piccola
// percentuale dei nodi) la ricerca da li' in poi non e' piu' andata avanti: e' bloccata.
static void stampa_riepilogo(const long long int n_backtrack) {
    printf("Nodi esplorati: %lld\n", nodi);
    if (pos_max >= 0) {
        printf("Posizione piu' avanzata: %d su %d (", pos_max, orario_len);
        stampa_nome_slot(pos_max < orario_len ? pos_max : orario_len - 1);
        printf("), raggiunta al nodo %lld (%.1f%% dei nodi)\n", nodi_a_pos_max,
               nodi ? 100.0 * nodi_a_pos_max / nodi : 0.0);
    }
    int distinti = 0;
    for (int i = 0; i < orario_len; i++) if (marce_per_pos[i]) distinti++;
    printf("Marce indietro: %lld in %d slot diversi\n", n_backtrack, distinti);
}

// Rami scartati dopo lo slot pos, per qualunque motivo.
static long long int scartati_nello_slot(const int pos) {
    long long int n = 0;
    for (int m = 0; m < N_MOTIVI_SCARTO; m++) n += scartati_per_pos[pos][m];
    return n;
}

// Come per le marce indietro: se i rami scartati sono quasi tutti dopo pochi slot, la ricerca e' bloccata li'.
static void stampa_rami_scartati(void) {
    long long int totale = 0;
    for (int m = 0; m < N_MOTIVI_SCARTO; m++) totale += scartati[m];
    int distinti = 0;
    for (int i = 0; i < orario_len; i++) if (scartati_nello_slot(i)) distinti++;
    printf("Rami scartati: %lld dopo %d slot diversi\n", totale, distinti);
    for (int m = 0; m < N_MOTIVI_SCARTO; m++)
        if (scartati[m]) printf("  %lld: %s\n", scartati[m], nome_motivo_scarto[m]);
    char usato[N_SLOT] = {0};
    for (int k = 0; k < 5; k++) {
        int best = -1;
        for (int i = 0; i < orario_len; i++)
            if (!usato[i] && scartati_nello_slot(i) && (best < 0 || scartati_nello_slot(i) > scartati_nello_slot(best)))
                best = i;
        if (best < 0) break;
        usato[best] = 1;
        printf("  Rami scartati dopo lo slot %d: posizione %d (", k + 1, best);
        stampa_nome_slot(best);
        printf("): %lld (%.1f%%)\n", scartati_nello_slot(best), 100.0 * scartati_nello_slot(best) / totale);
    }
}

// Con --motivi: in percentuale, perche' i docenti sono stati scartati nello slot pos.
static void stampa_motivi_dello_slot(const int pos) {
    if (!registra_motivi || !docenti_per_pos[pos]) return;
    printf("      motivi per cui i docenti con ore da fare in quella classe sono stati scartati (%.1f per marcia):\n",
           (double) docenti_per_pos[pos] / marce_per_pos[pos]);
    for (int m = 0; m < N_MOTIVI; m++)
        if (motivi_per_pos[pos][m])
            printf("        %5.1f%%  %s\n", 100.0 * motivi_per_pos[pos][m] / docenti_per_pos[pos], nome_motivo[m]);
}

// Lo slot con piu' marce indietro tra quelli non ancora stampati (-1 se non ce ne sono).
static int slot_con_piu_marce(const char usato[]) {
    int best = -1;
    for (int i = 0; i < orario_len; i++)
        if (!usato[i] && marce_per_pos[i] && (best < 0 || marce_per_pos[i] > marce_per_pos[best])) best = i;
    return best;
}

// I 5 slot con piu' marce indietro. Se pochi slot hanno quasi tutte le marce, il blocco e' li' (o poco prima).
static void stampa_slot_piu_frequenti(const long long int n_backtrack) {
    char usato[N_SLOT] = {0};
    for (int k = 0; k < 5; k++) {
        const int best = slot_con_piu_marce(usato);
        if (best < 0) break;
        usato[best] = 1;
        printf("  Slot piu' frequente %d: posizione %d (", k + 1, best);
        stampa_nome_slot(best);
        printf("): %lld marce (%.1f%%)\n", marce_per_pos[best],
               n_backtrack ? 100.0 * marce_per_pos[best] / n_backtrack : 0.0);
        stampa_motivi_dello_slot(best);
    }
}

// I docenti scartati piu' spesso, in tutta la ricerca.
static void stampa_docenti_scartati_piu_spesso(void) {
    if (!registra_motivi) return;
    printf("  Docenti scartati piu' spesso (docente: motivo, volte):\n");
    char usato[INSEGNANTE_SZ * N_MOTIVI] = {0};
    for (int k = 0; k < 6; k++) {
        int bp = -1, bm = -1;
        for (int p = 0; p < actual_insegnanti; p++)
            for (int m = 0; m < N_MOTIVI; m++)
                if (!usato[p * N_MOTIVI + m] && motivi_per_docente[p][m]
                    && (bp < 0 || motivi_per_docente[p][m] > motivi_per_docente[bp][bm])) {
                    bp = p;
                    bm = m;
                }
        if (bp < 0) break;
        usato[bp * N_MOTIVI + bm] = 1;
        printf("    %s: %s, %lld\n", insegnanti[bp], nome_motivo[bm], motivi_per_docente[bp][bm]);
    }
}

// Riepilogo, marce indietro per slot, docenti scartati (con --motivi) e rami scartati dalla potatura.
// tests/casuali.py legge questo testo: se si cambiano le frasi, va aggiornato anche lui.
void stampa_diagnosi(const long long int n_backtrack) {
    printf("\nDiagnosi della ricerca\n");
    if (interrotto_per_tempo) printf("Interrotto per tempo\n");
    stampa_riepilogo(n_backtrack);
    stampa_slot_piu_frequenti(n_backtrack);
    stampa_docenti_scartati_piu_spesso();
    stampa_rami_scartati();
}
