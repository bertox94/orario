#include "orario.h"

#include <stdlib.h>

teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
int orario_len;
int indice_slot[CLASSE_SEZIONE_SZ];

// Giorno, ora e classe della posizione p nell'ordine iniziale (giorno, poi ora, poi classe).
static int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

static int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

static int g_classe(const int p) {
    return p % actual_classi_sezione;
}

// Ricava indice_slot dal primo blocco (tutti i blocchi hanno lo stesso ordine).
static void ricostruisci_indice_slot(void) {
    for (int k = 0; k < actual_classi_sezione; k++) {
        indice_slot[orario[k].classe] = k;
    }
}

// Calcola i valori precompilati di uno slot a partire da giorno, ora e classe.
// Va richiamata ogni volta che cambia uno di questi tre campi (cioe' in inizializza_orario e in randomizza_ordine_classi).
static void precalcola_slot(teaching_slot *s) {
    s->base2d_ci = s->classe * INSEGNANTE_SZ;
    s->base2d_gc = s->giorno * CLASSE_SEZIONE_SZ;
    s->base3d_gci = s->base2d_gc * INSEGNANTE_SZ + s->base2d_ci;
    s->base3d_goi = s->giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + s->ora * INSEGNANTE_SZ;
    s->base4d_queue = s->giorno * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ
                      + s->ora * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + s->classe * INSEGNANTE_SZ;
}

// Ogni slot conosce la propria posizione (giorno, ora, classe) fin dall'inizio; i docenti sono -1 = non assegnato.
// Qui, e solo qui, giorno/ora/classe si ricavano dalla posizione (ordine iniziale: classi in ordine).
// Va chiamata dopo che i dati sono stati inizializzati e orario_len e' noto.
void inizializza_orario(void) {
    for (int p = 0; p < orario_len; p++) {
        orario[p] = (teaching_slot){-1, -1, g_giorno(p), g_ora(p), g_classe(p)};
        precalcola_slot(&orario[p]);
    }
    ricostruisci_indice_slot();
}

// OPZIONALE. Mescola l'ordine delle classi. L'ordine scelto e' unico e vale per ogni giorno e ora:
// cambia solo il campo classe degli slot, giorno e ora di ogni slot restano gli stessi.
// Stesso seed = stesso ordine. Da chiamare dopo inizializza_orario().
void randomizza_ordine_classi(const unsigned int seed) {
    int ordine[CLASSE_SEZIONE_SZ];
    for (int k = 0; k < actual_classi_sezione; k++) ordine[k] = k;
    srand(seed);
    for (int k = actual_classi_sezione - 1; k > 0; k--) {
        const int j = rand() % (k + 1);
        const int tmp = ordine[k];
        ordine[k] = ordine[j];
        ordine[j] = tmp;
    }
    for (int p = 0; p < orario_len; p++) {
        orario[p].classe = ordine[p % actual_classi_sezione];
        precalcola_slot(&orario[p]);
    }
    ricostruisci_indice_slot();
}
