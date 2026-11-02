#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <sys/time.h>

#include "utils.h"
#include "priority_queue/prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;
long long int n_backtrack = 0; // quante volte non si e' potuto scegliere nessun docente per uno slot
int stampa_backtrack = 0;      // se 1, ad ogni marcia indietro scrive la riga di avviso e la griglia parziale (lento)
long long int n_gen_max = SCHEDULES_PER_GB / 5;
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];

int orario_len;
FILE *f;


int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

// L'ordine delle classi e' lo stesso in ogni (giorno, ora): basta ricordare, per ogni classe, in che posizione
// sta dentro il blocco di un'ora. Per questo indice_slot ha dimensione CLASSE_SEZIONE_SZ e non una per giorno e ora.
int indice_slot[CLASSE_SEZIONE_SZ];

// Dalla tripla (giorno, ora, classe) all'indice dello slot in orario[].
// La classe di uno slot NON si ricava dalla sua posizione ma si legge dallo slot (orario[i].classe):
// l'ordine delle classi puo' essere cambiato da randomizza_ordine_classi.
int slot_di(const int giorno, const int ora, const int classe) {
    return (giorno * actual_ore_giornaliere + ora) * actual_classi_sezione + indice_slot[classe];
}

// Slot dell'ora precedente (ora - 1) della stessa classe. Richiede ora > 0.
const teaching_slot *slot_precedente(const int giorno, const int ora, const int classe) {
    return &orario[slot_di(giorno, ora - 1, classe)];
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
int activities_for_this_class_and_hour(const teaching_slot *slot) {
    const int gio = slot->giorno;
    const int ora = slot->ora;
    const int cla = slot->classe;
    int ore_rimanenti = ore_classi[cla] - ore_classe_svolte[cla];
    int slot_obligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    int slot_minimi_da_riempire = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana;
    int slot_mancanti = slot_minimi_da_riempire - slot_obligatori_gia_occupati;

    if (ore_rimanenti < slot_mancanti) {
        return 0;
    }
    //se c'è almeno un ora buca, non mettere altre lezioni
    if (n_ore_buche[slot->base2d_gc + cla] > 0)
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
        const teaching_slot *prev = slot_precedente(giorno, ora, orario[pos].classe);
        int prev_prof1 = prev->insegnante1;
        int prev_prof2 = prev->insegnante2;

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
        const teaching_slot *prev = slot_precedente(giorno, ora, orario[pos].classe);
        int prev_prof1 = prev->insegnante1;
        int prev_prof2 = prev->insegnante2;

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
    return 0;
}

// Assuming you know the number of elements in the array, say 'n'
void sort_queue_mul(size_t start, size_t n) {
    // qsort has an average time complexity of O(N log N)
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

int prof_fa_al_max_n_ore(const teaching_slot *slot, const int prof) {
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + prof] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

int prof_fa_ore_contigue(const teaching_slot *slot, const int prof) {
    if (ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + prof]) {
        const teaching_slot *prev = slot_precedente(slot->giorno, slot->ora, slot->classe);
        return prev->insegnante1 == prof || prev->insegnante2 == prof;
    }
    return 1;
}

int prof_è_disponibile(const int base3d_goi, int p1) {
    return !prof_gia_impegnato_giorno_ora[base3d_goi + p1];
}

int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) {
    return ore_classi_docenti_svolte[base2d_ci + p1] < ore_classi_docenti[base2d_ci + p1];
}

// Posizione, in ore_classi_codocenza[_svolte], della coppia (p1, p2) nella classe cla (prof a indice minore prima).
// Locale a main.c cosi' il compilatore la espande inline (le versioni in database_initializers.c sono in un'altra unita').
static inline int idx_codocenza(const int cla, const int p1, const int p2) {
    const int lo = p1 < p2 ? p1 : p2;
    const int hi = p1 < p2 ? p2 : p1;
    return cla * INSEGNANTE_SZ * INSEGNANTE_SZ + lo * INSEGNANTE_SZ + hi;
}

static inline int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, const int p1, const int p2) {
    const int i = idx_codocenza(cla, p1, p2);
    return ore_classi_codocenza_svolte[i] < ore_classi_codocenza[i];
}

// Quante volte e' stato assegnato uno slot (in qualunque punto della ricerca). Serve a capire, senza una variabile
// per nodo, se un nodo ha trovato almeno un docente: le chiamate piu' profonde partono solo dopo un'assegnazione
// fatta da quel nodo, quindi il contatore cresce se e solo se il nodo ha assegnato qualcuno.
long long int n_assegnazioni = 0;

// Operazioni comuni a ogni docente di un'assegnazione (da solo o in coppia) e il loro annullamento.
// base2d_ci = cla * INSEGNANTE_SZ, base3d_gci = giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + base2d_ci.
static inline void prep_chiamata_ric_basic(const int base2d_ci, const int base3d_goi, const int base3d_gci,
                                           QueueItem *it) {
    const int p = it->teacher;
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + p] = 1;
    ore_classi_docenti_svolte[base2d_ci + p]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[base3d_gci + p]++;
}

static inline void post_chiamata_ric_basic(const int base2d_ci, const int base3d_goi, const int base3d_gci,
                                           QueueItem *it) {
    const int p = it->teacher;
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + p] = 0;
    ore_classi_docenti_svolte[base2d_ci + p]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[base3d_gci + p]--;
}

// Le funzioni prep/post fanno tutto l'aggiornamento di stato di un'assegnazione di uno slot (e il suo annullamento):
// le operazioni per docente (basic), quelle specifiche (ore da solo, codocenza) e, una volta per slot,
// il conteggio delle ore svolte dalla classe (ore_classe_svolte) e il contatore n_assegnazioni.
static inline void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, const int base3d_gci,
                                           const int cla, QueueItem *it) {
    prep_chiamata_ric_basic(base2d_ci, base3d_goi, base3d_gci, it);
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
    ore_classe_svolte[cla]++;
    n_assegnazioni++;
}

static inline void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int base3d_gci,
                                                   const int cla, QueueItem *it, QueueItem *jt) {
    prep_chiamata_ric_basic(base2d_ci, base3d_goi, base3d_gci, it);
    prep_chiamata_ric_basic(base2d_ci, base3d_goi, base3d_gci, jt);
    ore_classi_codocenza_svolte[idx_codocenza(cla, it->teacher, jt->teacher)]++;
    ore_classe_svolte[cla]++;
    n_assegnazioni++;
}

static inline void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, const int base3d_gci,
                                           const int cla, QueueItem *it) {
    post_chiamata_ric_basic(base2d_ci, base3d_goi, base3d_gci, it);
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
    ore_classe_svolte[cla]--;
}

static inline void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int base3d_gci,
                                                   const int cla, QueueItem *it, QueueItem *jt) {
    post_chiamata_ric_basic(base2d_ci, base3d_goi, base3d_gci, it);
    post_chiamata_ric_basic(base2d_ci, base3d_goi, base3d_gci, jt);
    ore_classi_codocenza_svolte[idx_codocenza(cla, it->teacher, jt->teacher)]--;
    ore_classe_svolte[cla]--;
}

int è_un_docente_che_insegna_da_solo_in_questa_classe(const int base2d_ci, const int prof) {
    return classi_docenti_teorici[base2d_ci + prof];
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
            int current_class = orario[i].classe;

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
            i = start + (orario[i].ora + 1) * actual_classi_sezione - 1;
        }
    }

    return 1;
}

int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= ora_di_entrata_prof[prof];
}

void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;

    //print(orario, pos, n_gen, f);
    if (pos == orario_len) {
        //if (verifica_ore_classe_materia()) { //not needed ? all hours should match
        n_gen++;
        //printf("%lld \n", n_gen);
        print(orario, pos, n_gen, f);
        //  }
        return;
    }

    const teaching_slot *slot = &orario[pos];
    const int giorno = slot->giorno;
    const int ora = slot->ora;
    const int cla = slot->classe;
    const int base2d_ci = slot->base2d_ci;
    const int base2d_gc = slot->base2d_gc;
    const int base3d_gci = slot->base3d_gci;
    const int base3d_goi = slot->base3d_goi;
    const int base4d_queue = slot->base4d_queue;


    sort_queue_mul(base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(slot)) {
        // se alla fine il contatore non e' cresciuto, questo nodo non ha assegnato nessun docente (marcia indietro)
        const long long int assegnazioni_prima = n_assegnazioni;
        const long long int queue_size = base4d_queue + queue_sz;
        for (int ii = base4d_queue; ii < queue_size; ii++) {
            QueueItem *it = queue_mul[ii];
            const int p1 = it->teacher;

            if ( // verifica che nella stessa ora ciascun prof non sia già in altre classi
                prof_è_disponibile(base3d_goi, p1)
                // controlla ore svolte per materia, verifica che siano inferiori a quelle totali
                && deve_ancora_svolgere_ore_in_questa_classe(base2d_ci, p1)
                // un prof fa al max n ore in una classe
                && prof_fa_al_max_n_ore(slot, p1)
                // se un prof fa più ore in una classe in un giorno, devono essere contigue
                && prof_fa_ore_contigue(slot, p1)
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
                        && prof_fa_al_max_n_ore(slot, p2)
                        // se un prof fa più ore in una classe in un giorno, devono essere contigue
                        && prof_fa_ore_contigue(slot, p2)
                        // poiche un prof se deve fare due ore no può farle spezzettate, e sabato è 'ulyimo giorno, non si puo mettere un altro prof,
                        // la seconda ora deve farla subito
                        && sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, p1, p2)
                        //ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO materie (classi) diverse ogni giorno
                        && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p2)
                        && 1
                    ) {
                        prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, base3d_gci, cla, it, jt);
                        orario[pos].insegnante1 = p1;
                        orario[pos].insegnante2 = p2;
                        print_orario(pos + 1);
                        post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, base3d_gci, cla, it, jt);
                    }
                }
                //non abbiamo trovato combinazioni di codocenza, verifica se all'insegnante mancano ore di docenza da solo
                if (è_un_docente_che_insegna_da_solo_in_questa_classe(base2d_ci, p1)
                    && deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                    && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)
                    && 1) {
                    prep_chiamata_ricorsiva(base2d_ci, base3d_goi, base3d_gci, cla, it);
                    orario[pos].insegnante1 = p1;
                    orario[pos].insegnante2 = -1;
                    print_orario(pos + 1);
                    post_chiamata_ricorsiva(base2d_ci, base3d_goi, base3d_gci, cla, it);
                }
            }
        }
        if (n_assegnazioni == assegnazioni_prima) {
            n_backtrack++;
            if (stampa_backtrack) {
                fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                        giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
                print(orario, pos, n_gen, f);
            }
        }
    }


    if (ora >= PRIME_N_ORE_OBBLIGATORIE) {
        orario[pos].insegnante1 = -1;
        orario[pos].insegnante2 = -1;
        n_ore_buche[base2d_gc + cla]++;
        print_orario(pos + 1);
        n_ore_buche[base2d_gc + cla]--;
    }
}

// Uso: ./untitled [seed] [-b]
//   seed: l'ordine delle classi viene mescolato (stesso seed = stesso ordine)
//   -b / --backtrack: scrive in orario.txt anche ogni marcia indietro (riga "Backtrack" + griglia parziale).
//                     Senza, orario.txt contiene solo gli orari completi e si va molto piu' veloci.
int main(int argc, char **argv) {
    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    printf("Program started at: %s\n", ctime(&currentTime));
    inizializza_dati();
    stampa_dati();
    //imposta la lunghezza dell'orario
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    inizializza_orario();
    int usa_seed = 0;
    unsigned int seed = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--backtrack")) {
            stampa_backtrack = 1;
        } else {
            seed = (unsigned int) strtoul(argv[i], NULL, 10);
            usa_seed = 1;
        }
    }
    if (usa_seed) randomizza_ordine_classi(seed);
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


    f = fopen("orario.txt", "w");
    //f = stdout;
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(0);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Marce indietro: %lld\n", n_backtrack);
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed);
    fflush(stdout);
    fclose(f);

    return 0;
}
