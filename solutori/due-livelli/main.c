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

// Ordine della ricerca nel livello 2: dentro ogni giorno una classe alla volta (tutte le ore di C01, poi C02...), cosi'
// ogni classe dispone i blocchi dei suoi docenti uno dopo l'altro. L'orario resta indicizzato per cella come in main,
// pos = (giorno*ore + ora)*classi + classe; la ricerca scorre gli indici s e cella_di[s] e' la cella del passo s.
int cella_di[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
int indice_di[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];

static void prepara_ordine(void) {
    int s = 0;
    for (int g = 0; g < actual_giorni_settimana; g++)
        for (int c = 0; c < actual_classi_sezione; c++)
            for (int o = 0; o < actual_ore_giornaliere; o++) {
                const int cella = (g * actual_ore_giornaliere + o) * actual_classi_sezione + c;
                cella_di[s] = cella;
                indice_di[cella] = s++;
            }
}

// ---- solver a due livelli ----
// Livello 1 (all'inizio di ogni giorno): le quote del giorno, cioe' quante ore fa oggi ogni docente in ogni classe, da
// solo e in ogni coppia. Livello 2: la ricerca di main sulle celle del giorno, con i controlli che guardano le quote
// del giorno invece dei totali della settimana. Gli obiettivi sono cumulativi: ore fatte fino a ieri + quota di oggi.
int obiettivo_tot[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];   // per classe e docente (da solo + in codocenza)
int obiettivo_solo[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];  // per classe e docente, da solo
int obiettivo_classe[CLASSE_SEZIONE_SZ];                // ore di lezione della classe
// le coppie (classe, p < q) con ore di codocenza, ordinate per classe
#define MAX_COPPIE (CLASSE_SEZIONE_SZ * 40)
int n_coppie, coppia_c[MAX_COPPIE], coppia_p[MAX_COPPIE], coppia_q[MAX_COPPIE], coppia_obiettivo[MAX_COPPIE];
int prima_coppia_di_classe[CLASSE_SEZIONE_SZ + 1];

int indice_coppia(const int cla, int p1, int p2) {
    if (p1 > p2) { const int x = p1; p1 = p2; p2 = x; }
    for (int k = prima_coppia_di_classe[cla]; k < prima_coppia_di_classe[cla + 1]; k++)
        if (coppia_p[k] == p1 && coppia_q[k] == p2) return k;
    return -1;
}

int obiettivo_coppia(const int cla, const int p1, const int p2) {
    const int k = indice_coppia(cla, p1, p2);
    return k < 0 ? 0 : coppia_obiettivo[k];
}


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
    // come in main, ma per il giorno: le ore della quota di oggi che restano bastano per gli slot obbligatori di oggi
    int ore_rimanenti = obiettivo_classe[cla] - ore_classe_svolte[cla];
    int slot_mancanti = PRIME_N_ORE_OBBLIGATORIE - MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);

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
    return ore_classi_docenti_svolte[base2d_ci + p1] < obiettivo_tot[base2d_ci + p1];
}

int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, int p1, int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2) < obiettivo_coppia(cla, p1, p2);
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
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < obiettivo_solo[
               base2d_ci + prof];
}

// s: l'indice (nell'ordine della ricerca) della cella che si sta decidendo; contano le celle del giorno gia' decise.
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int s, int giorno, int cla, const int prof) {
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO];
    int classi_sz = 0;
    classi[classi_sz++] = cla;
    for (int o = 0; o < actual_ore_giornaliere; o++)
        for (int c = 0; c < actual_classi_sezione; c++) {
            const int i = (giorno * actual_ore_giornaliere + o) * actual_classi_sezione + c;
            if (indice_di[i] >= s || (orario[i].insegnante1 != prof && orario[i].insegnante2 != prof)) continue;
            int found = 0;
            for (int j = 0; j < classi_sz; j++) found |= classi[j] == c;
            if (!found) {
                if (classi_sz >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
                classi[classi_sz++] = c;
            }
        }
    return 1;
}

int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= ora_di_entrata_prof[prof];
}


// ---- livello 1: le quote di un giorno ----
// Una "voce" e' quello che una classe deve fare: un docente da solo, o una coppia. Per il giorno g si sceglie quante ore
// di ogni voce fare oggi, classe per classe, con i vincoli che si possono vedere senza l'orario delle ore:
//  - ogni voce: tra 0 e le ore che le restano; ogni docente al massimo MAX_ORE_PROF_IN_UNA_CLASSE ore oggi in una classe;
//  - ogni classe: tra PRIME_N_ORE_OBBLIGATORIE e le ore del giorno, e cosi' che i giorni dopo restino fattibili;
//  - ogni docente: oggi al massimo le ore da quando entra e MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi;
//    per ogni classe le ore che gli restano dopo oggi devono stare nei giorni dopo (MAX_ORE_PROF_IN_UNA_CLASSE al giorno).
// L'ultimo giorno le quote sono tutto quello che resta. Le scelte si provano dalla quota "giusta" (le ore che restano
// divise per i giorni che restano) verso le altre; per ognuna, il livello 2 dispone il giorno.
#define MAX_VOCI_CLASSE 64
typedef struct { int p, q, k; } Voce;  // q < 0: docente p da solo; se no coppia k (p < q)
static Voce voci[CLASSE_SEZIONE_SZ][MAX_VOCI_CLASSE];
static int n_voci[CLASSE_SEZIONE_SZ];
// lo stato del livello 1, uno per giorno: mentre il giorno g e' disposto, il livello 1 del giorno dopo ha il suo
static int quota_voce_g[GIORNO_SETTIMANA_SZ][CLASSE_SEZIONE_SZ][MAX_VOCI_CLASSE];
static int oggi_tc_g[GIORNO_SETTIMANA_SZ][CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];  // ore di oggi per classe e docente
static int oggi_docente_g[GIORNO_SETTIMANA_SZ][INSEGNANTE_SZ], classi_oggi_docente_g[GIORNO_SETTIMANA_SZ][INSEGNANTE_SZ];
static int giorno_l1 = -1, dopo_l1;  // il giorno che si sta decidendo; quanti giorni restano dopo
// Budget del livello 2: un tentativo di disporre un giorno con certe quote si abbandona dopo NODI_PER_TENTATIVO nodi
// del giorno (si passa alle quote successive). Se e' successo almeno una volta, "nessuna soluzione" non e' dimostrato:
// alla fine la traccia dice "non trovato".
#define NODI_PER_TENTATIVO 50000
static long long nodi_tentativo[GIORNO_SETTIMANA_SZ];
static int abbandona_giorno = -1;  // >= 0: si torna al livello 1 di questo giorno
int ricerca_tagliata = 0;
#define quota_voce quota_voce_g[giorno_l1]
// per ogni giorno: le voci di ogni classe in ordine di urgenza, e la "pressione" di ogni docente (quanto e' stretto
// rispetto ai giorni che restano: ore che gli restano / ore che puo' ancora fare, classi da incontrare / classi al
// giorno per i giorni che restano)
static int ordine_voci_g[GIORNO_SETTIMANA_SZ][CLASSE_SEZIONE_SZ][MAX_VOCI_CLASSE];
static double pressione_g[GIORNO_SETTIMANA_SZ][INSEGNANTE_SZ];
#define ordine_voci ordine_voci_g[giorno_l1]
#define pressione pressione_g[giorno_l1]
#define oggi_tc oggi_tc_g[giorno_l1]
#define oggi_docente oggi_docente_g[giorno_l1]
#define classi_oggi_docente classi_oggi_docente_g[giorno_l1]

void print_orario(int pos);

static void prepara_voci(void) {
    n_coppie = 0;
    for (int c = 0; c < actual_classi_sezione; c++) {
        prima_coppia_di_classe[c] = n_coppie;
        n_voci[c] = 0;
        for (int p = 0; p < actual_insegnanti; p++) {
            if (ore_classi_docenti_alone[c * INSEGNANTE_SZ + p])
                voci[c][n_voci[c]++] = (Voce){p, -1, -1};
            for (int q = p + 1; q < actual_insegnanti; q++)
                if (get_ore_insegnanti_codocenza_in_una_classe(c, p, q)) {
                    coppia_c[n_coppie] = c; coppia_p[n_coppie] = p; coppia_q[n_coppie] = q;
                    voci[c][n_voci[c]++] = (Voce){p, q, n_coppie++};
                }
        }
    }
    prima_coppia_di_classe[actual_classi_sezione] = n_coppie;
}

static int ore_voce(const int c, const Voce *v) {
    return v->q < 0 ? ore_classi_docenti_alone[c * INSEGNANTE_SZ + v->p]
                    : get_ore_insegnanti_codocenza_in_una_classe(c, v->p, v->q);
}

static int fatte_voce(const int c, const Voce *v) {
    return v->q < 0 ? ore_classi_docenti_alone_svolte[c * INSEGNANTE_SZ + v->p]
                    : get_ore_insegnanti_codocenza_svolte_in_una_classe(c, v->p, v->q);
}

// mette (+1) o toglie (-1) x ore di oggi al docente p nella classe c
static int aggiungi_al_docente(const int c, const int p, const int x, const int segno) {
    const int i = c * INSEGNANTE_SZ + p;
    if (segno > 0) {
        if (oggi_tc[i] + x > MAX_ORE_PROF_IN_UNA_CLASSE) return 0;
        if (oggi_docente[p] + x > actual_ore_giornaliere - ora_di_entrata_prof[p]) return 0;
        if (x && !oggi_tc[i] && classi_oggi_docente[p] >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
        if (x && !oggi_tc[i]) classi_oggi_docente[p]++;
        oggi_tc[i] += x; oggi_docente[p] += x;
    } else {
        oggi_tc[i] -= x; oggi_docente[p] -= x;
        if (x && !oggi_tc[i]) classi_oggi_docente[p]--;
    }
    return 1;
}

static void applica_quote(const int segno);
static void scegli_classe(int c);

// la voce v della classe c, poi le successive; somma = ore di oggi della classe finora
static void scegli_voce(const int c, const int v, const int somma) {
    if (n_gen_max <= n_gen || abbandona_giorno >= 0) return;
    const int rem_c = ore_classi[c] - ore_classe_svolte[c];
    const int lo = MAX(PRIME_N_ORE_OBBLIGATORIE, rem_c - actual_ore_giornaliere * dopo_l1);
    const int hi = MIN(actual_ore_giornaliere, rem_c - PRIME_N_ORE_OBBLIGATORIE * dopo_l1);
    if (v == n_voci[c]) {
        if (somma < lo || somma > hi) return;
        // ogni docente della classe: quello che gli resta dopo oggi deve stare nei giorni dopo
        for (int k = 0; k < n_voci[c]; k++) {
            const int pp[2] = {voci[c][k].p, voci[c][k].q};
            for (int z = 0; z < 2; z++) {
                if (pp[z] < 0) continue;
                const int i = c * INSEGNANTE_SZ + pp[z];
                if (ore_classi_docenti[i] - ore_classi_docenti_svolte[i] - oggi_tc[i] > MAX_ORE_PROF_IN_UNA_CLASSE * dopo_l1)
                    return;
            }
        }
        obiettivo_classe[c] = ore_classe_svolte[c] + somma;
        scegli_classe(c + 1);
        obiettivo_classe[c] = ore_classe_svolte[c];
        return;
    }
    // anche mettendo il massimo a tutte le voci che restano non si arriva al minimo della classe: inutile andare avanti
    int massimo_resto = 0;
    for (int k = v; k < n_voci[c]; k++) {
        const Voce *y = &voci[c][ordine_voci[c][k]];
        massimo_resto += MIN(MAX_ORE_PROF_IN_UNA_CLASSE, ore_voce(c, y) - fatte_voce(c, y));
    }
    if (somma + massimo_resto < lo || somma > hi) return;
    const int kv = ordine_voci[c][v];  // le voci si provano in ordine di urgenza
    const Voce *x = &voci[c][kv];
    const int rem = ore_voce(c, x) - fatte_voce(c, x);
    const int massimo = MIN(rem, MAX_ORE_PROF_IN_UNA_CLASSE);
    // l'ultimo giorno si fa tutto quello che resta
    const int minimo = dopo_l1 ? 0 : rem;
    if (minimo > massimo) return;
    // la quota giusta: le ore che restano divise per i giorni che restano; per le voci urgenti (docenti stretti)
    // arrotondata per eccesso (si anticipa), per le altre per difetto; poi le quote vicine
    const int giorni = dopo_l1 + 1;
    const double urgenza = MAX(pressione[x->p], x->q >= 0 ? pressione[x->q] : 0);
    const int giusta = MIN(massimo, MAX(minimo, urgenza >= 0.6 ? (rem + giorni - 1) / giorni : rem / giorni));
    for (int d = 0; d <= massimo - minimo; d++) {
        const int vv[2] = {giusta + (d + 1) / 2 * (d % 2 ? 1 : -1), 0};
        const int val = d == 0 ? giusta : vv[0];
        if (val < minimo || val > massimo) continue;
        if (!aggiungi_al_docente(c, x->p, val, 1)) continue;
        if (x->q >= 0 && !aggiungi_al_docente(c, x->q, val, 1)) { aggiungi_al_docente(c, x->p, val, -1); continue; }
        quota_voce[c][kv] = val;
        scegli_voce(c, v + 1, somma + val);
        aggiungi_al_docente(c, x->p, val, -1);
        if (x->q >= 0) aggiungi_al_docente(c, x->q, val, -1);
    }
}

// Controllo in avanti del livello 1: ogni classe ancora da fare (da c in poi) deve poter arrivare al suo minimo di ore
// di oggi con i docenti che restano: una voce conta al massimo le ore che le restano, MAX_ORE_PROF_IN_UNA_CLASSE e le
// ore libere oggi dei suoi docenti, e solo se i suoi docenti possono ancora entrare in una classe nuova oggi.
static int classi_successive_possibili(const int c) {
    for (int k = c; k < actual_classi_sezione; k++) {
        const int rem_c = ore_classi[k] - ore_classe_svolte[k];
        const int lo = MAX(PRIME_N_ORE_OBBLIGATORIE, rem_c - actual_ore_giornaliere * dopo_l1);
        int massimo = 0;
        for (int v = 0; v < n_voci[k] && massimo < lo; v++) {
            const Voce *x = &voci[k][v];
            int m = MIN(ore_voce(k, x) - fatte_voce(k, x), MAX_ORE_PROF_IN_UNA_CLASSE);
            const int pp[2] = {x->p, x->q};
            for (int z = 0; z < 2 && m > 0; z++) {
                const int p = pp[z];
                if (p < 0) continue;
                if (classi_oggi_docente[p] >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) m = 0;
                m = MIN(m, actual_ore_giornaliere - ora_di_entrata_prof[p] - oggi_docente[p]);
            }
            massimo += MAX(m, 0);
        }
        if (massimo < lo) return 0;
    }
    return 1;
}

// Dopo le quote di tutte le classi: per ogni docente, quello che gli resta dopo oggi deve stare nei giorni dopo, come
// ore al giorno (da quando entra) e come classi al giorno (MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi, ognuna
// con al massimo MAX_ORE_PROF_IN_UNA_CLASSE ore).
static int docenti_possibili_dopo(void) {
    static int ore_dopo[INSEGNANTE_SZ], incontri_dopo[INSEGNANTE_SZ];
    for (int p = 0; p < actual_insegnanti; p++) ore_dopo[p] = incontri_dopo[p] = 0;
    for (int c = 0; c < actual_classi_sezione; c++)
        for (int p = 0; p < actual_insegnanti; p++) {
            const int i = c * INSEGNANTE_SZ + p;
            const int resto = ore_classi_docenti[i] - ore_classi_docenti_svolte[i] - oggi_tc[i];
            if (resto <= 0) continue;
            ore_dopo[p] += resto;
            incontri_dopo[p] += (resto + MAX_ORE_PROF_IN_UNA_CLASSE - 1) / MAX_ORE_PROF_IN_UNA_CLASSE;
        }
    for (int p = 0; p < actual_insegnanti; p++) {
        if (ore_dopo[p] > (actual_ore_giornaliere - ora_di_entrata_prof[p]) * dopo_l1) return 0;
        if (incontri_dopo[p] > MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO * dopo_l1) return 0;
    }
    return 1;
}

static void scegli_classe(const int c) {
    if (c < actual_classi_sezione) {
        if (classi_successive_possibili(c)) scegli_voce(c, 0, 0);
        return;
    }
    if (!docenti_possibili_dopo()) return;
    // tutte le classi hanno la quota: il livello 2 dispone il giorno
    applica_quote(1);
    nodi_tentativo[giorno_l1] = 0;
    print_orario(giorno_l1 * actual_ore_giornaliere * actual_classi_sezione);
    if (abbandona_giorno == giorno_l1) abbandona_giorno = -1;  // tentativo abbandonato: si provano altre quote
    applica_quote(-1);
}

// imposta (+1) o toglie (-1) gli obiettivi del giorno: fatte fino a ieri + quota di oggi
static void applica_quote(const int segno) {
    for (int c = 0; c < actual_classi_sezione; c++)
        for (int k = 0; k < n_voci[c]; k++) {
            const Voce *v = &voci[c][k];
            const int x = segno * quota_voce[c][k];
            if (v->q < 0) obiettivo_solo[c * INSEGNANTE_SZ + v->p] += x;
            else coppia_obiettivo[v->k] += x;
            obiettivo_tot[c * INSEGNANTE_SZ + v->p] += x;
            if (v->q >= 0) obiettivo_tot[c * INSEGNANTE_SZ + v->q] += x;
        }
}

// le quote del giorno g, e per ognuna il giorno disposto (che alla fine passa al giorno dopo)
static void livello_giorno(const int g) {
    const int salva_g = giorno_l1, salva_d = dopo_l1;
    giorno_l1 = g;
    dopo_l1 = actual_giorni_settimana - 1 - g;
    // obiettivi di partenza = ore fatte (il giorno prima si e' chiuso esattamente con le sue quote)
    for (int c = 0; c < actual_classi_sezione; c++) obiettivo_classe[c] = ore_classe_svolte[c];
    // la pressione di ogni docente e le voci di ogni classe in ordine di urgenza
    const int giorni = dopo_l1 + 1;
    for (int p = 0; p < actual_insegnanti; p++) {
        int ore = 0, incontri = 0;
        for (int c = 0; c < actual_classi_sezione; c++) {
            const int i = c * INSEGNANTE_SZ + p, r = ore_classi_docenti[i] - ore_classi_docenti_svolte[i];
            if (r > 0) { ore += r; incontri += (r + MAX_ORE_PROF_IN_UNA_CLASSE - 1) / MAX_ORE_PROF_IN_UNA_CLASSE; }
        }
        const double a = (double) ore / ((actual_ore_giornaliere - ora_di_entrata_prof[p]) * giorni);
        const double b = (double) incontri / (MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO * giorni);
        pressione[p] = MAX(a, b);
    }
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int k = 0; k < n_voci[c]; k++) ordine_voci[c][k] = k;
        for (int a = 1; a < n_voci[c]; a++)  // in ordine di urgenza, la piu' alta prima (insertion sort: poche voci)
            for (int b = a; b > 0; b--) {
                const Voce *x = &voci[c][ordine_voci[c][b]], *y = &voci[c][ordine_voci[c][b - 1]];
                const double ux = MAX(pressione[x->p], x->q >= 0 ? pressione[x->q] : 0);
                const double uy = MAX(pressione[y->p], y->q >= 0 ? pressione[y->q] : 0);
                if (ux <= uy) break;
                const int t = ordine_voci[c][b]; ordine_voci[c][b] = ordine_voci[c][b - 1]; ordine_voci[c][b - 1] = t;
            }
    }
    scegli_classe(0);
    giorno_l1 = salva_g;
    dopo_l1 = salva_d;
}

// alla fine di un giorno: ogni classe ha fatto esattamente la sua quota (e quindi ogni voce la sua)
static int giorno_chiuso(void) {
    for (int c = 0; c < actual_classi_sezione; c++) if (ore_classe_svolte[c] != obiettivo_classe[c]) return 0;
    return 1;
}

// Livello 2: la ricerca di main sulle celle del giorno. All'inizio di un giorno dopo il primo si controlla che il giorno
// prima si sia chiuso con le sue quote e si passa al livello 1 per il giorno nuovo.
static void cerca_cella(int pos);

void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;
    const int celle_giorno = actual_ore_giornaliere * actual_classi_sezione;
    if (pos > 0 && pos % celle_giorno == 0 && pos / celle_giorno != giorno_l1 + 1) {
        // e' l'inizio di un giorno appena scelto dal livello 1: si cercano le celle
        cerca_cella(pos);
        return;
    }
    if (pos > 0 && pos % celle_giorno == 0) {
        if (!giorno_chiuso()) return;
        if (pos < orario_len) {
            // ai giorni dopo arrivano solo le ore fatte, che dipendono dalle quote e non da come il giorno e' disposto:
            // se il resto della settimana non ha soluzione, non ce l'ha per nessun'altra disposizione di questo giorno
            // con le stesse quote, e si torna subito al livello 1 a cambiarle (taglio esatto, non un limite)
            const long long trovati = n_gen;
            livello_giorno(pos / celle_giorno);
            if (n_gen == trovati) abbandona_giorno = pos / celle_giorno - 1;
            return;
        }
    }
    cerca_cella(pos);
}

// All'inizio della giornata di una classe: ogni suo docente con ore oggi deve avere ancora, a partire dal suo ingresso,
// tante ore libere consecutive quante sono le sue ore di oggi nella classe (le sue ore in una classe sono contigue).
static int blocchi_possibili(const int giorno, const int cla) {
    for (int v = 0; v < n_voci[cla]; v++) {
        const int pp[2] = {voci[cla][v].p, voci[cla][v].q};
        for (int z = 0; z < 2; z++) {
            const int p = pp[z];
            if (p < 0) continue;
            const int quota = obiettivo_tot[cla * INSEGNANTE_SZ + p] - ore_classi_docenti_svolte[cla * INSEGNANTE_SZ + p];
            if (quota <= 0) continue;
            int di_fila = 0, ok = 0;
            for (int o = ora_di_entrata_prof[p]; o < actual_ore_giornaliere && !ok; o++) {
                di_fila = prof_gia_impegnato_giorno_ora[giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + o * INSEGNANTE_SZ + p]
                          ? 0 : di_fila + 1;
                ok = di_fila >= quota;
            }
            if (!ok) return 0;
        }
    }
    return 1;
}

// Dopo aver messo una coppia o un docente nella cella (giorno, ora, cla): la giornata della classe e' ancora possibile?
// Con le quote esatte le ore della classe oggi sono le prime (lezioni) e poi le buche; le ore di un docente nella classe
// sono un blocco contiguo. Quindi: chi ha un blocco aperto (era nell'ora prima) e ha ancora quota deve esserci anche
// adesso; chi c'e' adesso e ha ancora quota deve trovare libere le ore subito dopo; chi non ha ancora cominciato deve
// trovare, nelle ore di lezione che restano alla classe, tante ore libere consecutive (dopo il suo ingresso) quante
// sono le sue. Ritorna il primo docente per cui non c'e' spazio, o -1.
int docente_senza_spazio(const int giorno, const int cla, const int ora) {
    const int pos = (giorno * actual_ore_giornaliere + ora) * actual_classi_sezione + cla;
    const int restano = obiettivo_classe[cla] - ore_classe_svolte[cla];  // lezioni dopo questa ora
    const int ultima = ora + restano;                                      // l'ultima ora di lezione della classe oggi
    for (int v = 0; v < n_voci[cla]; v++) {
        const int pp[2] = {voci[cla][v].p, voci[cla][v].q};
        for (int z = 0; z < 2; z++) {
            const int p = pp[z];
            if (p < 0) continue;
            const int i = cla * INSEGNANTE_SZ + p;
            const int quota = obiettivo_tot[i] - ore_classi_docenti_svolte[i];
            if (quota <= 0) continue;
            const int adesso = orario[pos].insegnante1 == p || orario[pos].insegnante2 == p;
            const int fatte = ore_fatte_da_un_prof_in_una_classe_in_giorno[giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ +
                                                                           cla * INSEGNANTE_SZ + p];
            if (fatte && !adesso) return p;  // il suo blocco si e' interrotto
            int da = adesso ? ora + 1 : MAX(ora + 1, ora_di_entrata_prof[p]);
            int di_fila = 0, ok = 0;
            for (int o = da; o <= ultima && !ok; o++) {
                const int occupato = prof_gia_impegnato_giorno_ora[giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ +
                                                                   o * INSEGNANTE_SZ + p];
                if (occupato) {
                    if (adesso) break;  // deve continuare subito: un'ora occupata interrompe il blocco
                    di_fila = 0;
                } else di_fila++;
                ok = di_fila >= quota;
            }
            if (!ok) return p;
        }
    }
    return -1;
}

// s: l'indice della cella nell'ordine della ricerca; pos = cella_di[s] e' la cella (come in main)
static void cerca_cella(const int s) {
    if (n_gen_max <= n_gen || abbandona_giorno >= 0) return;
    if (s < orario_len) {
        const int g = g_giorno(cella_di[s]);
        if (++nodi_tentativo[g] > NODI_PER_TENTATIVO) {
            abbandona_giorno = g;
            ricerca_tagliata = 1;
            return;
        }
    }
    // traccia: solo conteggio dei nodi e, ogni tanto, il progresso (non cambia la ricerca)
    if (!(++traccia_nodi & 0xFFFF)) traccia_progresso(s);
    if (s > traccia_pos_max) traccia_pos_max = s;  // la posizione piu' avanti raggiunta (solo traccia)
    if (traccia_con_nodi) traccia_nodo(s);

    if (s == orario_len) {
        // (solo nei solver nuovi) ogni classe deve aver fatto tutte le sue ore: in main questo controllo e' commentato
        // e una classe puo' chiudere la settimana con ore buche al posto di lezioni
        for (int c = 0; c < actual_classi_sezione; c++) if (ore_classe_svolte[c] != ore_classi[c]) return;
        //if (verifica_ore_classe_materia()) { //not needed ? all hours should match
        n_gen++;
        //printf("%lld \n", n_gen);
        print(orario, orario_len, n_gen, f);
        traccia_orario_trovato();
        //  }
        return;
    }

    // le potature (potature/potature.h): la scelta nella cella prima lascia un resto ancora possibile?
    if (potatura_dopo(s)) {
        traccia_potatura(s, g_giorno(cella_di[s]), g_ora(cella_di[s]), g_classe(cella_di[s]));
        return;
    }

    const int pos = cella_di[s];
    if (g_ora(pos) == 0 && !blocchi_possibili(g_giorno(pos), g_classe(pos))) return;
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
                && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(s, giorno, cla, p1)
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
                        && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(s, giorno, cla, p2)
                        // (solo nei solver nuovi) anche il codocente deve essere gia' entrato
                        && verifica_ora_ingresso(p2, ora)
                        && 1
                    ) {
                        prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                        orario[pos] = (teaching_slot){p1, p2};
                        if (docente_senza_spazio(giorno, cla, ora) < 0) {
                            found_prof = 1;
                            print_orario(s + 1);
                        }
                        post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                    }
                }
                //non abbiamo trovato combinazioni di codocenza, verifica se all'insegnante mancano ore di docenza da solo
                if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                    && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)
                    && 1) {
                    prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                    orario[pos] = (teaching_slot){p1, -1};
                    if (docente_senza_spazio(giorno, cla, ora) < 0) {
                        found_prof = 1;
                        print_orario(s + 1);
                    }
                    post_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                }
            }
        }
        ore_classe_svolte[cla]--;
        if (!found_prof) {
            fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                    giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
            print(orario, orario_len, n_gen, f);
            traccia_backtrack(s, giorno, ora, cla, base2d_ci, base3d_goi, base4d_queue,
                              ora >= PRIME_N_ORE_OBBLIGATORIE);
        }
    } else if (ora < PRIME_N_ORE_OBBLIGATORIE) {
        // lo slot non puo' avere lezione e l'ora buca non e' ammessa: la ricerca torna indietro (solo traccia)
        traccia_slot_chiuso(s, giorno, ora, cla);
    }


    // ora buca: solo se la classe ha gia' fatto tutta la quota di oggi (con quote esatte, se le restano ore la lezione
    // va messa adesso: le ore buche stanno solo in fondo alla giornata)
    if (ora >= PRIME_N_ORE_OBBLIGATORIE && ore_classe_svolte[cla] == obiettivo_classe[cla]) {
        orario[pos] = (teaching_slot){-1, -1};
        n_ore_buche[base2d_gc + cla]++;
        print_orario(s + 1);
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
    prepara_voci();
    prepara_ordine();
    prepara_potature(cella_di);
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
    livello_giorno(0);

    const double elapsed = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed);
    fflush(stdout);
    traccia_fine(elapsed);
    traccia_chiudi();
    fclose(f);

    return 0;
}
