#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "utils.h"
#include "priority_queue/prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "data_initialization/generatore.h"
#include "printer/printer.h"

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;
long long int n_backtrack = 0; // quante volte non si e' potuto scegliere nessun docente per uno slot
int stampa_backtrack = 0;      // se 1, ad ogni marcia indietro scrive la riga di avviso e la griglia parziale (lento)
long long int n_gen_max = SCHEDULES_PER_GB / 5;

static double secondi_reali(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Diagnosi della ricerca (non influiscono sulla ricerca): servono a capire dove si blocca.
long long int nodi = 0;                       // chiamate di print_orario
int pos_max = -1;                             // posizione piu' avanzata raggiunta
long long int nodi_a_pos_max = 0;             // a quale nodo e' stata raggiunta
long long int marce_per_pos[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ]; // marce indietro per slot
double limite_secondi = 0;                    // se > 0 la ricerca si ferma dopo tanti secondi (--tempo)
int interrotto_per_tempo = 0;
double inizio_ricerca; // secondi (orologio reale), vedi secondi_reali()
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

/*Progressive contraints*/
int activities_for_this_class_and_hour(const teaching_slot *slot) {
    const int gio = slot->giorno;
    const int ora = slot->ora;
    const int cla = slot->classe;
    int ore_rimanenti = classe_info[cla].ore - classe_info[cla].svolte;
    int slot_obligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    int slot_minimi_da_riempire = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana;
    int slot_mancanti = slot_minimi_da_riempire - slot_obligatori_gia_occupati;

    if (ore_rimanenti < slot_mancanti) {
        return 0;
    }
    //se c'è almeno un ora buca, non mettere altre lezioni
    if (n_ore_buche[slot->base2d_gc + cla] > 0)
        return 0;

    return 1;
}

// Regola del sabato (ultimo giorno): se nell'ora precedente c'erano docenti con ore ancora da fare in questa classe,
// non si puo' cambiare: vanno completati subito. prof2 == -1 indica un docente che insegna da solo.
int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        const teaching_slot *prev = slot_precedente(giorno, ora, orario[pos].classe);
        const int prev_prof1 = prev->insegnante1;
        const int prev_prof2 = prev->insegnante2;
        // lo slot precedente ha sempre insegnante1 != -1 se ha insegnante2 != -1 (le coppie sono scritte insieme)

        if (prev_prof1 != -1 && prev_prof2 != -1) {
            const int da_fare1 = classe_docente[base2d_ci + prev_prof1].ore - classe_docente[base2d_ci + prev_prof1].svolte > 0;
            const int da_fare2 = classe_docente[base2d_ci + prev_prof2].ore - classe_docente[base2d_ci + prev_prof2].svolte > 0;
            if (da_fare1 && da_fare2) {
                // entrambi devono esserci anche in quest'ora: possibile solo come la stessa coppia (mai da soli)
                return prof1 == prev_prof1 && prof2 == prev_prof2
                       || prof1 == prev_prof2 && prof2 == prev_prof1;
            } else if (da_fare1) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            } else if (da_fare2) {
                return prof1 == prev_prof2 || prof2 == prev_prof2;
            }
        } else if (prev_prof1 != -1) {
            if (classe_docente[base2d_ci + prev_prof1].ore - classe_docente[base2d_ci + prev_prof1].svolte > 0) {
                return prof1 == prev_prof1 || prof2 == prev_prof1;
            }
        }
    }
    return 1;
}

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
    return classe_docente[base2d_ci + p1].svolte < classe_docente[base2d_ci + p1].ore;
}

// Posizione, in codocenza[_svolte].ore, della coppia (p1, p2) nella classe cla (prof a indice minore prima).
// Locale a main.c cosi' il compilatore la espande inline (le versioni in database_initializers.c sono in un'altra unita').
static inline int idx_codocenza(const int cla, const int p1, const int p2) {
    const int lo = p1 < p2 ? p1 : p2;
    const int hi = p1 < p2 ? p2 : p1;
    return cla * INSEGNANTE_SZ * INSEGNANTE_SZ + lo * INSEGNANTE_SZ + hi;
}

static inline int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, const int p1, const int p2) {
    const int i = idx_codocenza(cla, p1, p2);
    return codocenza[i].svolte < codocenza[i].ore;
}

// Quante volte e' stato assegnato uno slot (in qualunque punto della ricerca). Serve a capire, senza una variabile
// per nodo, se un nodo ha trovato almeno un docente: le chiamate piu' profonde partono solo dopo un'assegnazione
// fatta da quel nodo, quindi il contatore cresce se e solo se il nodo ha assegnato qualcuno.
long long int n_assegnazioni = 0;

void print_orario(int pos);

// Operazioni comuni a ogni docente di un'assegnazione (da solo o in coppia) e il loro annullamento.
static inline void prep_chiamata_ric_basic(const teaching_slot *slot, QueueItem *it) {
    const int p = it->teacher;
    it->priority--;
    prof_gia_impegnato_giorno_ora[slot->base3d_goi + p] = 1;
    classe_docente[slot->base2d_ci + p].svolte++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + p]++;
}

static inline void post_chiamata_ric_basic(const teaching_slot *slot, QueueItem *it) {
    const int p = it->teacher;
    it->priority++;
    prof_gia_impegnato_giorno_ora[slot->base3d_goi + p] = 0;
    classe_docente[slot->base2d_ci + p].svolte--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[slot->base3d_gci + p]--;
}

// Le funzioni prep/post fanno tutto l'aggiornamento di stato di un'assegnazione di uno slot e il suo annullamento:
// l'assegnazione dei docenti nello slot (insegnante1/insegnante2), le operazioni per docente (basic), quelle
// specifiche (ore da solo, codocenza) e, una volta per slot, classe_info[].svolte e il contatore n_assegnazioni.
// post rimette lo slot a "non assegnato" (-1, -1): uno slot fuori dal percorso corrente e' sempre -1/-1.
// jt == NULL: it insegna da solo; altrimenti it e jt insegnano insieme (codocenza).
static inline void prep_chiamata_ricorsiva(teaching_slot *slot, QueueItem *it, QueueItem *jt) {
    prep_chiamata_ric_basic(slot, it);
    if (jt) {
        prep_chiamata_ric_basic(slot, jt);
        codocenza[idx_codocenza(slot->classe, it->teacher, jt->teacher)].svolte++;
    } else {
        classe_docente[slot->base2d_ci + it->teacher].alone_svolte++;
    }
    slot->insegnante1 = it->teacher;
    slot->insegnante2 = jt ? jt->teacher : -1;
    classe_info[slot->classe].svolte++;
    n_assegnazioni++;
}

static inline void post_chiamata_ricorsiva(teaching_slot *slot, QueueItem *it, QueueItem *jt) {
    post_chiamata_ric_basic(slot, it);
    if (jt) {
        post_chiamata_ric_basic(slot, jt);
        codocenza[idx_codocenza(slot->classe, it->teacher, jt->teacher)].svolte--;
    } else {
        classe_docente[slot->base2d_ci + it->teacher].alone_svolte--;
    }
    slot->insegnante1 = -1;
    slot->insegnante2 = -1;
    classe_info[slot->classe].svolte--;
}

// Assegna lo slot (da solo o in coppia), prosegue la ricerca e annulla.
static inline void assegna_e_prosegui(teaching_slot *slot, const int pos, QueueItem *it, QueueItem *jt) {
    prep_chiamata_ricorsiva(slot, it, jt);
    print_orario(pos + 1);
    post_chiamata_ricorsiva(slot, it, jt);
}

int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return classe_docente[base2d_ci + prof].alone_svolte < classe_docente[
               base2d_ci + prof].alone;
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
    return ora >= docente_info[prof].ora_entrata;
}

// Vincoli comuni a ogni docente (il primo o il secondo di una coppia) per essere messo in questo slot.
static inline int docente_idoneo(const teaching_slot *slot, const int pos, const int prof) {
    // non prima della sua ora di ingresso
    return verifica_ora_ingresso(prof, slot->ora)
           // un prof fa al max n ore in una classe
           && prof_fa_al_max_n_ore(slot, prof)
           // se un prof fa piu' ore in una classe in un giorno, devono essere contigue
           && prof_fa_ore_contigue(slot, prof)
           // ogni docente fa al max MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse ogni giorno
           && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, slot->giorno, slot->classe, prof);
}


void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;
    nodi++;
    if (pos > pos_max) {
        pos_max = pos;
        nodi_a_pos_max = nodi;
    }
    if (limite_secondi > 0 && (nodi & 0xFFFF) == 0
        && secondi_reali() - inizio_ricerca > limite_secondi) {
        interrotto_per_tempo = 1;
        n_gen_max = 0; // tutte le chiamate tornano subito e lo stato viene annullato normalmente
        return;
    }

    if (pos == orario_len) {
        n_gen++;
        print(orario, pos, n_gen, f);
        return;
    }

    teaching_slot *slot = &orario[pos];
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

            if (prof_è_disponibile(base3d_goi, p1) // non e' gia' in un'altra classe nella stessa ora
                // ha ancora ore da fare in questa classe
                && deve_ancora_svolgere_ore_in_questa_classe(base2d_ci, p1)
                && docente_idoneo(slot, pos, p1)
            ) {
                for (int jj = ii + 1; jj < queue_size; jj++) {
                    QueueItem *jt = queue_mul[jj];
                    const int p2 = jt->teacher;
                    if (prof_è_disponibile(base3d_goi, p2)
                        && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(cla, p1, p2)
                        // poiche un prof se deve fare due ore no può farle spezzettate, e sabato è l'ultimo giorno,
                        // non si puo mettere un altro prof: la seconda ora deve farla subito
                        && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1, p2)
                        && docente_idoneo(slot, pos, p2)
                    ) {
                        assegna_e_prosegui(slot, pos, it, jt);
                    }
                }
                //non abbiamo trovato combinazioni di codocenza, verifica se all'insegnante mancano ore di docenza da solo
                if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                    && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1, -1)) {
                    assegna_e_prosegui(slot, pos, it, NULL);
                }
            }
        }
        if (n_assegnazioni == assegnazioni_prima) {
            n_backtrack++;
            marce_per_pos[pos]++;
            if (stampa_backtrack) {
                fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                        giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
                print(orario, pos, n_gen, f);
            }
        }
    }

    // ora buca (solo oltre le ore obbligatorie): lo slot e' gia' -1/-1 (vedi post_chiamata_*).
    // Dopo la buca la classe non fa altre lezioni quel giorno: le ore che le mancano devono stare nei giorni dopo.
    if (ora >= PRIME_N_ORE_OBBLIGATORIE
        && classe_info[cla].ore - classe_info[cla].svolte <= (actual_giorni_settimana - giorno - 1) * actual_ore_giornaliere) {
        n_ore_buche[base2d_gc + cla]++;
        print_orario(pos + 1);
        n_ore_buche[base2d_gc + cla]--;
    }
}

static void stampa_diagnosi(void) {
    printf("\nDiagnosi della ricerca\n");
    if (interrotto_per_tempo) printf("Interrotto per tempo\n");
    printf("Nodi esplorati: %lld\n", nodi);
    if (pos_max >= 0) {
        const teaching_slot *s = &orario[pos_max < orario_len ? pos_max : orario_len - 1];
        printf("Posizione piu' avanzata: %d su %d (giorno %s, ora %s, classe %s), raggiunta al nodo %lld (%.1f%% dei nodi)\n",
               pos_max, orario_len, giorni_settimana[s->giorno], ore_giornaliere[s->ora], classe_sezione[s->classe],
               nodi_a_pos_max, nodi ? 100.0 * nodi_a_pos_max / nodi : 0.0);
    }
    int distinti = 0;
    for (int i = 0; i < orario_len; i++) if (marce_per_pos[i]) distinti++;
    printf("Marce indietro: %lld in %d slot diversi\n", n_backtrack, distinti);
    // i 5 slot con piu' marce indietro
    char usato[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {0};
    for (int k = 0; k < 5; k++) {
        int best = -1;
        for (int i = 0; i < orario_len; i++)
            if (!usato[i] && marce_per_pos[i] && (best < 0 || marce_per_pos[i] > marce_per_pos[best])) best = i;
        if (best < 0) break;
        usato[best] = 1;
        const teaching_slot *s = &orario[best];
        printf("  Slot piu' frequente %d: posizione %d (giorno %s, ora %s, classe %s): %lld marce (%.1f%%)\n", k + 1, best,
               giorni_settimana[s->giorno], ore_giornaliere[s->ora], classe_sezione[s->classe], marce_per_pos[best],
               n_backtrack ? 100.0 * marce_per_pos[best] / n_backtrack : 0.0);
    }
}

// Uso: ./untitled [seed] [-b] [--casuale SEME [--classi N]] [-n MAX]
//   seed: l'ordine delle classi viene mescolato (stesso seed = stesso ordine)
//   -b / --backtrack: scrive in orario.txt anche ogni marcia indietro (riga "Backtrack" + griglia parziale).
//                     Senza, orario.txt contiene solo gli orari completi e si va molto piu' veloci.
//   --casuale SEME: invece dei dati del programma usa dati generati a caso (riproducibili con lo stesso SEME):
//                   classi da 29 a 36 ore, docenti da 5 a 24 ore, con lezioni da soli e in codocenza.
//   --classi N: numero di classi dei dati casuali (default 8)
//   -n MAX: si ferma dopo MAX orari completi
//   --tempo SEC: interrompe la ricerca dopo SEC secondi (la diagnosi viene stampata comunque)
//   --dati FILE: scrive in FILE la descrizione dei dati (classi, docenti, lezioni, codocenze)
int main(int argc, char **argv) {
    time_t currentTime;
    time(&currentTime); // Get the current time
    const clock_t start = clock();

    int usa_seed = 0;
    unsigned int seed = 0;
    int casuale = 0, n_classi_casuali = 8;
    const char *file_dati = NULL;
    unsigned long long seme_dati = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--backtrack")) {
            stampa_backtrack = 1;
        } else if (!strcmp(argv[i], "--casuale") && i + 1 < argc) {
            casuale = 1;
            seme_dati = strtoull(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--classi") && i + 1 < argc) {
            n_classi_casuali = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--dati") && i + 1 < argc) {
            file_dati = argv[++i];
        } else if (!strcmp(argv[i], "--tempo") && i + 1 < argc) {
            limite_secondi = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            n_gen_max = atoll(argv[++i]);
        } else {
            seed = (unsigned int) strtoul(argv[i], NULL, 10);
            usa_seed = 1;
        }
    }

    printf("Program started at: %s\n", ctime(&currentTime));
    if (casuale) {
        inizializza_calendario();
        genera_dati_casuali(seme_dati, n_classi_casuali);
    } else {
        inizializza_dati();
    }
    stampa_dati();
    fflush(stdout);
    if (file_dati) {
        FILE *fd = fopen(file_dati, "w");
        if (casuale) fprintf(fd, "Dati casuali: --casuale %llu --classi %d\n\n", seme_dati, n_classi_casuali);
        stampa_dati_su_file(fd);
        fclose(fd);
    }
    //imposta la lunghezza dell'orario
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    inizializza_orario();
    if (usa_seed) randomizza_ordine_classi(seed);

    inizio_ricerca = secondi_reali();
    f = fopen("orario.txt", "w");
    // al massimo ogni ora ci possono essere 2 prof in tutte le classi
    print_orario(0);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Marce indietro: %lld\n", n_backtrack);
    stampa_diagnosi();
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed);
    fflush(stdout);
    fclose(f);

    return 0;
}
