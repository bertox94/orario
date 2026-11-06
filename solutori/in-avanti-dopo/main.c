#include <stdlib.h>
#include <tgmath.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "utils.h"
#include "priority_queue/prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"
#include "ingresso_uscita/dati_json.h"
#include "ingresso_uscita/traccia.h"
#include "potature/potature.h"

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

long long int n_gen = 0;
long long int n_gen_max = SCHEDULES_PER_GB / 5;
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ...
        GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1
    ] = (teaching_slot){-1, -1}
};

int orario_len;
FILE *f;


int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

/*End of block constraints*/
int verifica_ore_classe_materia(void) {
    // 1. Check standard teacher hours (2D array flattened logic)
    int idx_2d = 0;
    for (int c = 0; c < CLASSE_SEZIONE_SZ; c++) {
        for (int t = 0; t < INSEGNANTE_SZ; t++) {
            if (ore_classi_docenti[idx_2d] != ore_classi_docenti_svolte[idx_2d]
            ) {
                print(orario, orario_len, n_gen, f);
                fprintf(f, "Wrong: Classe: %s, Teacher: %s %d but got %d\n\n",
                        classe_sezione[c],
                        insegnanti[t],
                        ore_classi_docenti[idx_2d],
                        ore_classi_docenti_svolte[idx_2d]);
                return 0;
            }
            idx_2d++;
        }
    }
    /*
        // 2. Check co-teacher hours (3D array flattened logic)
        int idx_3d = 0;
        for (int c = 0; c < CLASSE_SEZIONE_SZ; c++) {
            for (int t1 = 0; t1 < INSEGNANTE_SZ; t1++) {
                for (int t2 = 0; t2 < INSEGNANTE_SZ; t2++) {
                    if (ore_classi_docenti_codocenti[idx_3d] != ore_classi_docenti_codocenti_svolte[idx_3d]) {
                        printf("Wrong: Classe: %s, Codocente: %s, Teacher: %s %d but got %d\n",
                               classe_sezione[c],
                               insegnanti[t2], // t2 matches (idx_3d % INSEGNANTE_SZ)
                               insegnanti[t1], // t1 matches ((idx_3d / INSEGNANTE_SZ) % INSEGNANTE_SZ)
                               ore_classi_docenti_codocenti[idx_3d],
                               ore_classi_docenti_codocenti_svolte[idx_3d]);
                        return 0;
                    }
                    idx_3d++;
                }
            }
        }*/

    return 1;
}

/*Progressive contraints*/
int activities_for_this_class_and_hour(int pos, int gio, const int ora, const int cla) {
    int ore_rimanenti = ore_classi[cla] - ore_classe_svolte[cla];
    int slot_obligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    int slot_minimi_da_riempire = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana;
    int slot_mancanti = slot_minimi_da_riempire - slot_obligatori_gia_occupati;

    if (ore_rimanenti < slot_mancanti) {
        return 0;
    }
    //se c'è almeno un ora buca, non mettere altre lezioni
    if (n_ore_buche[gio * CLASSE_SEZIONE_SZ + cla] > 0)
        return 0;

    //le ore di lezione in ogni clase rimanenti per ogni insegnante devono essere <= a 2*giorni rimanenti
    /*
        int base2dc = cla * INSEGNANTE_SZ;
        for (int i = 0; i < actual_insegnanti; i++) {
            if (ore_insegnante_classi[base2dc + i] > 0
                && ore_insegnante_classi[base2dc + i] - ore_classi_docenti_svolte[base2dc + i] >
                MAX_ORE_STESSA_MATERIA_GIORNO * (actual_giorni_settimana - gio))
                return 0;
        }
    */
    return 1;
}

int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        int prev_prof1 = orario[pos - actual_classi_sezione].insegnante1;
        int prev_prof2 = orario[pos - actual_classi_sezione].insegnante2;

        if (prev_prof1 != -1 && prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] > 0
                && ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] > 0) {
                return prof1 == prev_prof1 && prof2 == prev_prof2
                       || prof1 == prev_prof2 && prof2 == prev_prof1;
            } else if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                       0) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            } else if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                       0) {
                return prof1 == prev_prof2 || prof2 == prev_prof2;
            }
        } else if (prev_prof1 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                0) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            }
        } else if (prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                0) {
                return prof1 == prev_prof2 || prof2 == prev_prof2;
            }
        }
    }
    return 1;
}

int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof) {
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        int prev_prof1 = orario[pos - actual_classi_sezione].insegnante1;
        int prev_prof2 = orario[pos - actual_classi_sezione].insegnante2;

        if (prev_prof1 != -1 && prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] > 0
                && ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] > 0) {
                return 0; // both docenti need to be there in this hour, so one will never complete the hours
            } else if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                       0) {
                return prof == prev_prof1;
            } else if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                       0) {
                return prof == prev_prof2;
            }
        } else if (prev_prof1 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof1] - ore_classi_docenti_svolte[base2d_ci + prev_prof1] >
                0) {
                return prof == prev_prof1;
            }
        } else if (prev_prof2 != -1) {
            if (ore_classi_docenti[base2d_ci + prev_prof2] - ore_classi_docenti_svolte[base2d_ci + prev_prof2] >
                0) {
                return prof == prev_prof2;
            }
        }
    }
    return 1;
}

void print_up_to(QueueItem arr[], int pos) {
    for (int i = 0; i < pos; i++) {
        printf("%d)\t %d %.0f ", i, arr[i].teacher, arr[i].priority);
    }
    printf("\n");
}


void debug(const int base3d_goi, const int base2d_ci, const int base3d_gci, int j) {
    if (ore_classi_docenti[base2d_ci + j] == 0) return;
    printf("Insegnante: %s\n", insegnanti[j]);
    if (prof_gia_impegnato_giorno_ora[base3d_goi + j]) printf("Già impegnato ");
    if (ore_classi_docenti_svolte[base2d_ci + j] >= ore_classi_docenti[base2d_ci + j]) printf("Ore eccessive ");
    printf("\n");
}

int assert_non_decreasing(const int base4d_queue, const long long int jjmax, int jj) {
    for (int p = base4d_queue; p < jj; p++) {
        for (int q = p + 1; q < jjmax; q++) {
            if (!(queue_mul[p]->priority >= queue_mul[q]->priority))
                exit(-1);
        }
    }
    return 0;
}

/*
QueueItem *set_max_at_of_remaining(const long long int jjmax, int jj) {
    int max_idx = jj; // Track the INDEX of the maximum element, not just the value

    for (int k = jj + 1; k < jjmax; k++) {
        // k can start at jj + 1
        if (queue_mul[k]->priority > queue_mul[max_idx]->priority) {
            max_idx = k; // Found a higher priority, record its position
        }
    }

    // ONLY SWAP ONCE after finding the true maximum for this position
    if (max_idx != jj) {
        QueueItem *tmp = queue_mul[jj];
        queue_mul[jj] = queue_mul[max_idx];
        queue_mul[max_idx] = tmp;
    }

    return queue_mul[jj];
}
*/

// Comparison function for qsort
int compare_queue_items(const void *a, const void *b) {
    // Cast the void pointers to pointers to QueueItem*
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;

    // Sort by priority (descending: highest priority first)
    if (itemA->priority < itemB->priority) return 1;
    if (itemA->priority > itemB->priority) return -1;
    // a parita' di priorita' decide l'indice del docente (come in main, PR #70): l'ordine non dipende da qsort
    return (itemA->teacher > itemB->teacher) - (itemA->teacher < itemB->teacher);
}

// Assuming you know the number of elements in the array, say 'n'
void sort_queue_mul(size_t start, size_t n) {
    // qsort has an average time complexity of O(N log N)
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof) {
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[
               giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof] <
           MAX_ORE_PROF_IN_UNA_CLASSE;
}

int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof) {
    if (ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof]) {
        return orario[giorno * actual_ore_giornaliere * actual_classi_sezione + (ora - 1) * actual_classi_sezione + cla]
               .insegnante1 == prof
               || orario[giorno * actual_ore_giornaliere * actual_classi_sezione + (ora - 1) * actual_classi_sezione +
                         cla].insegnante2 == prof;
    }
    return 1;
}

int prof_è_disponibile(const int base3d_goi, int p1) {
    return !prof_gia_impegnato_giorno_ora[base3d_goi + p1];
}

int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) {
    return ore_classi_docenti_svolte[base2d_ci + p1] < ore_classi_docenti[base2d_ci + p1];
}

int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, int p1, int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2) <
           get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2);
}

void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof]++;
}

void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + it->teacher]++;
    jt->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + jt->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + jt->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + jt->teacher]++;
    increment_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof]--;
}

void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + it->teacher]--;
    jt->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + jt->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + jt->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + jt->teacher]--;
    decrement_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < ore_classi_docenti_alone[
               base2d_ci + prof];
}

int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, const int prof) {
    // Assuming start calculation based on your layout structure
    int start = giorno * actual_ore_giornaliere * actual_classi_sezione;

    // Fixed-size array instead of VLA (Variable Length Array).
    // Since max distinct classes allowed is n, we only ever need n elements.
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO];
    int classi_sz = 0;

    // Add the initial/current class
    classi[classi_sz++] = cla;

    for (int i = start; i < pos; i++) {
        if (orario[i].insegnante1 == prof || orario[i].insegnante2 == prof) {
            int current_class = g_classe(i);

            // Check if the class is already recorded
            int found = 0;
            for (int j = 0; j < classi_sz; j++) {
                if (classi[j] == current_class) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                // If we already have 'max_diff_class' distinct classes and find another one, fail early
                if (classi_sz >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) {
                    return 0;
                }
                classi[classi_sz++] = current_class;
            }

            // Jump to the next hour slot (-1 to compensate for the loop's i++)
            i = start + (g_ora(i) + 1) * actual_classi_sezione - 1;
        }
    }

    return 1;
}

int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= ora_di_entrata_prof[prof];
}

// L'ora buca e' possibile solo dopo le ore obbligatorie e se le ore che restano alla classe stanno ancora nei giorni
// dopo (un'ora buca chiude la giornata). Se no la classe deve fare lezione: senza docenti la ricerca torna indietro.
// (Come in main.)
int ora_buca_possibile(const int giorno, const int ora, const int cla) {
    return ora >= PRIME_N_ORE_OBBLIGATORIE
           && ore_classi[cla] - ore_classe_svolte[cla] <= actual_ore_giornaliere * (actual_giorni_settimana - 1 - giorno);
}

// ---- controllo in avanti (solo in questo solver) ----
// Dopo ogni scelta (una coppia o un docente nella cella pos) si guarda se il resto e' ancora risolvibile, con due
// controlli di fattibilita'. Sono condizioni necessarie: se dicono "no" e' impossibile e la scelta si scarta subito;
// se dicono "si'" non e' garantito niente.
//  1. le celle di oggi, una per una: ogni cella obbligatoria che resta oggi ha ancora almeno un docente possibile;
//  2. le classi, una per una: il conteggio delle lezioni possibili basta per le ore da riempire, oggi e nei giorni che
//     restano.
// In questo solver si fanno all'ingresso della cella dopo la scelta (vedi print_orario). Si fanno a ogni scelta perche'
// dipendono dallo stato, che cambia a ogni cella riempita (e costano poco: classi x
// voci x ore). La funzione da chiamare e' cella_bloccata_dopo_la_scelta(pos); la traccia la usa per il controllo
// situazione_risolvibile (all'ingresso della cella dopo la scelta).

// -- i dati che servono, preparati una volta: i docenti e le voci di ogni classe --
// una voce e' una lezione che la classe deve fare: un docente da solo (q < 0) o una coppia (p < q)
#define MAX_VOCI 64
static int docenti_di_classe[CLASSE_SEZIONE_SZ][INSEGNANTE_SZ], n_docenti_di_classe[CLASSE_SEZIONE_SZ];
static int voce_p[CLASSE_SEZIONE_SZ][MAX_VOCI], voce_q[CLASSE_SEZIONE_SZ][MAX_VOCI], n_voci[CLASSE_SEZIONE_SZ];

static void prepara_controllo_in_avanti(void) {
    for (int c = 0; c < actual_classi_sezione; c++) {
        n_docenti_di_classe[c] = n_voci[c] = 0;
        for (int p = 0; p < actual_insegnanti; p++) {
            if (ore_classi_docenti[c * INSEGNANTE_SZ + p]) docenti_di_classe[c][n_docenti_di_classe[c]++] = p;
            if (ore_classi_docenti_alone[c * INSEGNANTE_SZ + p] && n_voci[c] < MAX_VOCI) {
                voce_p[c][n_voci[c]] = p; voce_q[c][n_voci[c]++] = -1;
            }
            for (int q = p + 1; q < actual_insegnanti; q++)
                if (get_ore_insegnanti_codocenza_in_una_classe(c, p, q) && n_voci[c] < MAX_VOCI) {
                    voce_p[c][n_voci[c]] = p; voce_q[c][n_voci[c]++] = q;
                }
        }
    }
}

// -- lo stato di oggi dopo la scelta in pos --
typedef struct {
    int giorno, ora, cla, inizio;      // la cella appena decisa e la prima cella del giorno
    int classi_oggi[INSEGNANTE_SZ];    // in quante classi diverse ha gia' insegnato oggi ogni docente
} Oggi;

static void calcola_oggi(const int pos, Oggi *o) {
    static int timbri[INSEGNANTE_SZ][CLASSE_SEZIONE_SZ], timbro = 0;
    o->giorno = g_giorno(pos); o->ora = g_ora(pos); o->cla = g_classe(pos);
    o->inizio = o->giorno * actual_ore_giornaliere * actual_classi_sezione;
    timbro++;
    for (int p = 0; p < actual_insegnanti; p++) o->classi_oggi[p] = 0;
    for (int i = o->inizio; i <= pos; i++) {
        const int c = g_classe(i), pp[2] = {orario[i].insegnante1, orario[i].insegnante2};
        for (int k = 0; k < 2; k++) {
            if (pp[k] < 0 || timbri[pp[k]][c] == timbro) continue;
            timbri[pp[k]][c] = timbro;
            o->classi_oggi[pp[k]]++;
        }
    }
}

// l'ultima ora gia' decisa oggi della classe c: quella della scelta se c viene prima o e' la classe scelta, se no
// l'ora prima (-1: nessuna)
static int ultima_ora(const Oggi *o, const int c) { return c <= o->cla ? o->ora : o->ora - 1; }

static const teaching_slot *cella_ultima_ora(const Oggi *o, const int c) {
    const int u = ultima_ora(o, c);
    return u >= 0 ? &orario[o->inizio + u * actual_classi_sezione + c] : NULL;
}

// quante ore puo' ancora dare oggi il docente p nella classe c: 0 se il suo blocco e' chiuso (ha gia' insegnato oggi
// nella classe ma non nell'ultima ora decisa) o se non ci ha insegnato e ha gia' il massimo di classi oggi
static int massimo_oggi(const Oggi *o, const int c, const int p) {
    const int fatte = ore_fatte_da_un_prof_in_una_classe_in_giorno[o->giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ +
                                                                   c * INSEGNANTE_SZ + p];
    const teaching_slot *u = cella_ultima_ora(o, c);
    if (fatte && (!u || (u->insegnante1 != p && u->insegnante2 != p))) return 0;
    if (!fatte && o->classi_oggi[p] >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
    return MAX_ORE_PROF_IN_UNA_CLASSE - fatte;
}

// il docente p e' gia' entrato all'ora h ed e' libero
static int libero(const Oggi *o, const int h, const int p) {
    return h >= ora_di_entrata_prof[p] &&
           !prof_gia_impegnato_giorno_ora[o->giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + h * INSEGNANTE_SZ + p];
}

// -- 1. le celle di oggi --
// Ogni cella obbligatoria che resta oggi deve avere almeno un docente possibile: ha ancora ore nella classe, e' gia'
// entrato e libero a quell'ora, puo' ancora dare ore oggi nella classe (massimo_oggi). Ritorna la prima cella senza
// nessuno, o -1.
static int cella_senza_docenti(const int pos, const Oggi *o) {
    const int fine = o->inizio + PRIME_N_ORE_OBBLIGATORIE * actual_classi_sezione;  // dopo: ore che possono restare vuote
    for (int q = pos + 1; q < fine; q++) {
        const int h = g_ora(q), c = g_classe(q);
        int possibile = 0;
        for (int k = 0; k < n_docenti_di_classe[c] && !possibile; k++) {
            const int p = docenti_di_classe[c][k];
            possibile = ore_classi_docenti_svolte[c * INSEGNANTE_SZ + p] < ore_classi_docenti[c * INSEGNANTE_SZ + p]
                        && libero(o, h, p) && massimo_oggi(o, c, p) > 0;
        }
        if (!possibile) return q;
    }
    return -1;
}

// -- 2. il conteggio delle lezioni --
// Per la classe c, dalla sua prima ora non ancora decisa oggi (da) alle ore obbligatorie, e in ognuno dei giorni dopo
// (dopo): fino a ogni ora h servono (h - da + 1) lezioni oggi (se h >= da) piu' (h + 1) in ognuno dei giorni dopo. Le
// lezioni si contano per voce, come le fa la ricerca: un docente da solo, o una coppia (una lezione con due docenti,
// possibile solo quando tutti e due sono entrati). Una voce ne puo' dare: oggi, quante ne entrano tra da e h nelle ore
// in cui i suoi docenti sono liberi (e al massimo massimo_oggi di ognuno); in ognuno dei giorni dopo, al massimo
// MAX_ORE_PROF_IN_UNA_CLASSE tra le 0 e h dalle ore in cui sono entrati; in tutto, non piu' delle ore che le restano.
// Ritorna la cella dell'ora per cui non bastano (oggi, o il giorno dopo), o -1.
// Esempio (test 309): alla C07 restano Kakashi da solo e Hinata con Zoro, e Zoro entra alle 11: fino alle 10 di ogni
// giorno che resta servono 3 lezioni e ci sono solo quelle di Kakashi.
static int conteggio_lezioni(const Oggi *o, const int c, const int da, const int dopo) {
    int possibili[ORA_GIORNALIERA_SZ] = {0};  // lezioni possibili fino a ogni ora (oggi e nei giorni dopo)
    for (int v = 0; v < n_voci[c]; v++) {
        const int p = voce_p[c][v], q = voce_q[c][v];
        const int resto = q < 0
            ? ore_classi_docenti_alone[c * INSEGNANTE_SZ + p] - ore_classi_docenti_alone_svolte[c * INSEGNANTE_SZ + p]
            : get_ore_insegnanti_codocenza_in_una_classe(c, p, q) - get_ore_insegnanti_codocenza_svolte_in_una_classe(c, p, q);
        if (resto <= 0) continue;
        int massimo = 0;  // oggi: solo se resta qualcosa da decidere
        if (da < PRIME_N_ORE_OBBLIGATORIE) {
            massimo = MIN(resto, massimo_oggi(o, c, p));
            if (q >= 0) massimo = MIN(massimo, massimo_oggi(o, c, q));
        }
        const int entra = MAX(ora_di_entrata_prof[p], q >= 0 ? ora_di_entrata_prof[q] : 0);
        int ore = 0;
        for (int h = 0; h < PRIME_N_ORE_OBBLIGATORIE; h++) {
            if (h >= da && libero(o, h, p) && (q < 0 || libero(o, h, q)) && ore < massimo) ore++;
            const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, h + 1 - entra));
            possibili[h] += MIN(resto, ore + al_giorno * dopo);
        }
    }
    for (int h = 0; h < PRIME_N_ORE_OBBLIGATORIE; h++)
        if (possibili[h] < MAX(0, h - da + 1) + (h + 1) * dopo)
            return ((o->giorno + (h >= da || !dopo ? 0 : 1)) * actual_ore_giornaliere + h) * actual_classi_sezione + c;
    return -1;
}

// per ogni classe: prima il conteggio di oggi da solo, poi con i giorni dopo
static int classe_senza_lezioni(const Oggi *o) {
    const int dopo = actual_giorni_settimana - 1 - o->giorno;
    for (int c = 0; c < actual_classi_sezione; c++) {
        const int da = ultima_ora(o, c) + 1;
        int q = conteggio_lezioni(o, c, da, 0);
        if (q < 0) q = conteggio_lezioni(o, c, da, dopo);
        if (q >= 0) return q;
    }
    return -1;
}

// -- tutto insieme --
int motivo_blocco = 0;  // perche' la scelta si scarta: 0 una cella senza docenti, 1 non bastano le lezioni

int cella_bloccata_dopo_la_scelta(const int pos) {
    static Oggi o;
    calcola_oggi(pos, &o);
    motivo_blocco = 0;
    int q = cella_senza_docenti(pos, &o);
    if (q < 0) {
        motivo_blocco = 1;
        q = classe_senza_lezioni(&o);
    }
    return q;
}

void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;
    // traccia: solo conteggio dei nodi e, ogni tanto, il progresso (non cambia la ricerca)
    if (!(++traccia_nodi & 0xFFFF)) traccia_progresso(pos);
    if (pos > traccia_pos_max) traccia_pos_max = pos;  // la posizione piu' avanti raggiunta (solo traccia)
    if (traccia_con_nodi) traccia_nodo(pos);

    // le potature (potature/potature.h): la scelta nella cella prima lascia un resto ancora possibile?
    if (pos < orario_len && potatura_dopo(pos)) {
        traccia_potatura(pos, g_giorno(pos), g_ora(pos), g_classe(pos));
        return;
    }

    // Controllo in avanti, nella cella dopo la scelta (la differenza da in-avanti, che lo fa prima di scendere): se la
    // coppia o il docente messi nella cella prima rendono il resto impossibile, si torna subito indietro. Il resto di
    // print_orario e' quello di main, riga per riga.
    if (pos > 0 && (orario[pos - 1].insegnante1 >= 0 || orario[pos - 1].insegnante2 >= 0)) {
        const int q = cella_bloccata_dopo_la_scelta(pos - 1);
        if (q >= 0) {
            traccia_situazione_impossibile(pos, q, motivo_blocco);
            return;
        }
    }

    //print(orario, pos, n_gen, f);
    if (pos == orario_len) {
        //if (verifica_ore_classe_materia()) { //not needed ? all hours should match
        n_gen++;
        //printf("%lld \n", n_gen);
        print(orario, pos, n_gen, f);
        traccia_orario_trovato();
        //  }
        return;
    }

    const int giorno = g_giorno(pos);
    const int ora = g_ora(pos);
    const int cla = g_classe(pos);
    const int base2d_ci = cla * INSEGNANTE_SZ;
    const int base2d_gc = giorno * CLASSE_SEZIONE_SZ;
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;
    const int base4d_queue = giorno * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + ora *
                             CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ;


    sort_queue_mul(base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(pos, giorno, ora, cla)) {
        int found_prof = 0;
        ore_classe_svolte[cla]++;
        const long long int queue_size = base4d_queue + queue_sz;
        for (int ii = base4d_queue; ii < queue_size; ii++) {
            QueueItem *it = queue_mul[ii];
            const int p1 = it->teacher;

            if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                prof_è_disponibile(base3d_goi, p1)
                // controlla ore svolte per materia, verifica che siano inferiori a quelle totali
                && deve_ancora_svolgere_ore_in_questa_classe(base2d_ci, p1)
                // un prof fa al max n ore in una classe
                && prof_fa_al_max_n_ore(giorno, ora, cla, p1)
                // se un prof fa più ore in una classe in un giorno, devono essere contigue
                && prof_fa_ore_contigue(giorno, ora, cla, p1)
                //se un prof ha due ore il sabato deve farle consecutive, altrimenti non la fa, essendo non contigue
                //ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO materie (classi) diverse ogni giorno
                && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p1)
                && verifica_ora_ingresso(p1, ora)
                // NB. non e possibile verificare qua di esaurire ore il sabato (?)
                && 1
            ) {
                // assert(iitem->priority > 0);

                for (int jj = ii + 1; jj < queue_size; jj++) {
                    QueueItem *jt = queue_mul[jj];
                    const int p2 = jt->teacher;
                    if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                        prof_è_disponibile(base3d_goi, p2)
                        && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(cla, p1, p2)
                        // un prof fa al max n ore in una classe
                        && prof_fa_al_max_n_ore(giorno, ora, cla, p2)
                        // se un prof fa più ore in una classe in un giorno, devono essere contigue
                        && prof_fa_ore_contigue(giorno, ora, cla, p2)
                        // poiche un prof se deve fare due ore no può farle spezzettate, e sabato è 'ulyimo giorno, non si puo mettere un altro prof,
                        // la seconda ora deve farla subito
                        && sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, p1, p2)
                        //ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO materie (classi) diverse ogni giorno
                        && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p2)
                        // anche il codocente deve essere gia' entrato a quest'ora
                        && verifica_ora_ingresso(p2, ora)
                        && 1
                    ) {
                        prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                        found_prof = 1;
                        orario[pos] = (teaching_slot){p1, p2};
                        print_orario(pos + 1);
                        post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                    }
                }
                //non abbiamo trovato combinazioni di codocenza, verifica se all'insegnante mancano ore di docenza da solo
                if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                    && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)
                    && 1) {
                    prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                    found_prof = 1;
                    orario[pos] = (teaching_slot){p1, -1};
                    print_orario(pos + 1);
                    post_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                }
            }
        }
        ore_classe_svolte[cla]--;
        if (!found_prof) {
            fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                    giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
            print(orario, pos, n_gen, f);
            traccia_backtrack(pos, giorno, ora, cla, base2d_ci, base3d_goi, base4d_queue,
                              ora_buca_possibile(giorno, ora, cla));
        }
    } else if (ora < PRIME_N_ORE_OBBLIGATORIE) {
        // lo slot non puo' avere lezione e l'ora buca non e' ammessa: la ricerca torna indietro (solo traccia)
        traccia_slot_chiuso(pos, giorno, ora, cla);
    }


    if (ora_buca_possibile(giorno, ora, cla)) {
        orario[pos] = (teaching_slot){-1, -1};
        n_ore_buche[base2d_gc + cla]++;
        print_orario(pos + 1);
        n_ore_buche[base2d_gc + cla]--;
    }
}

// Uso: ./untitled [--input FILE] [--traccia FILE] [-n N] [--txt FILE] [--max-backtrack N] [--max-nodi N] [--progresso]
//   senza opzioni: come sempre, i dati di inizializza_dati() e gli orari in orario.txt
//   --input FILE:       i dati da un file JSON (vedi ingresso_uscita/dati_json.h) invece che da inizializza_dati()
//   --traccia FILE:     la traccia della ricerca per il sito, un JSON per riga (vedi ingresso_uscita/traccia.h)
//   -n N:               quanti orari cercare al massimo (n_gen_max; senza: SCHEDULES_PER_GB / 5)
//   --txt FILE:         dove scrivere orario.txt (per esempio /dev/null)
//   --max-backtrack N:  quanti backtrack registrare per esteso nella traccia (gli altri si contano), default 1000
//   --max-nodi N:       registra nella traccia anche i nodi della ricerca, al massimo N (default 0: nessuno)
//   --progresso:        ogni mezzo secondo una riga JSON di progresso su stderr
int main(int argc, char **argv) {
    const char *file_input = NULL, *file_traccia = NULL, *file_txt = "orario.txt";
    long long int max_backtrack = 1000, max_nodi = 0;
    int progresso = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--input") && i + 1 < argc) file_input = argv[++i];
        else if (!strcmp(argv[i], "--traccia") && i + 1 < argc) file_traccia = argv[++i];
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) n_gen_max = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--txt") && i + 1 < argc) file_txt = argv[++i];
        else if (!strcmp(argv[i], "--max-backtrack") && i + 1 < argc) max_backtrack = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--max-nodi") && i + 1 < argc) max_nodi = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--progresso")) progresso = 1;
        else {
            fprintf(stderr, "Opzione sconosciuta: %s\n", argv[i]);
            return 2;
        }
    }
    if (!traccia_apri(file_traccia, max_backtrack, max_nodi, progresso)) {
        fprintf(stderr, "Non riesco a scrivere la traccia in %s\n", file_traccia);
        return 2;
    }

    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    printf("Program started at: %s\n", ctime(&currentTime));
    if (file_input) {
        char errore[512];
        if (!inizializza_dati_da_json(file_input, errore, sizeof errore)) {
            traccia_errore(errore);
            traccia_chiudi();
            return 2;
        }
    } else {
        inizializza_dati();
    }
    stampa_dati();
    //imposta la lunghezza dell'orario
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    prepara_controllo_in_avanti();
    prepara_potature(NULL);
    //test(100);

    /*  // if you initialize with less elements it will set the remaming elements to 0.
      QueueItem *queue[INSEGNANTE_SZ] = {0};

      // Should do this while setting classi e insegnanti
      initialize_queue_arr(queue);
      print_priority_queue(queue);

      // 2. Extracted the total dimension to avoid recalculating
      const size_t num_blocks = GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ;
      const size_t total_elements = num_blocks * INSEGNANTE_SZ;

      // 3. Type-safe malloc (no explicit cast needed in C, but use sizeof(*ptr))
      queue_mul = malloc(total_elements * sizeof(*queue_mul));

      if (queue_mul == NULL) {
          // Always handle allocation failures!
          fprintf(stderr, "Memory allocation failed\n");
          return 0;
      }

      // 4. Highly optimized copying using memcpy instead of a nested loop
      for (size_t i = 0; i < num_blocks; i++) {
          memcpy(&queue_mul[i * INSEGNANTE_SZ], queue, sizeof(queue));
      }*/


    traccia_dati();
    f = fopen(file_txt, "w");
    //f = stdout;
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(0);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed);
    fflush(stdout);
    traccia_fine(elapsed);
    traccia_chiudi();
    fclose(f);

    return 0;
}
