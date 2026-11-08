// La ricerca dell'orario: backtracking cella per cella.
//
// L'orario e' un elenco di celle (giorno, ora, classe) in quest'ordine: per ogni giorno, per ogni ora, tutte le classi.
// La cella pos e' giorno = pos / (ore * classi), ora = (pos / classi) % ore, classe = pos % classi.
// print_orario(pos) decide la cella pos e scende a pos + 1: prova, nell'ordine della coda dei docenti (chi ha piu' ore
// da fare prima), ogni docente in coppia con un codocente e poi da solo; dopo le ore obbligatorie prova anche l'ora
// buca. Quando tutte le celle sono decise (pos == orario_len) l'orario e' completo e si stampa in orario.txt.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"
#include "priority_queue/prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// ---- lo stato della ricerca ----

long long int n_gen = 0;                          // orari completi trovati
long long int n_gen_max = SCHEDULES_PER_GB / 5;   // ci si ferma dopo tanti orari
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ... GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1] = (teaching_slot){-1, -1}
};
int orario_len;                                   // giorni * ore * classi
FILE *f;                                          // orario.txt: gli orari trovati e i backtrack

// ---- la cella ----

int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) { return p % actual_classi_sezione; }

// La cella che si sta decidendo, con gli indici di partenza negli array piatti che la riguardano.
typedef struct {
    int pos, giorno, ora, cla;
    int base2d_ci;     // classe, docente: ore_classi_docenti[base2d_ci + p] e simili
    int base3d_goi;    // giorno, ora, docente: prof_gia_impegnato_giorno_ora[base3d_goi + p]
    int base3d_gci;    // giorno, classe, docente: ore_fatte_da_un_prof_in_una_classe_in_giorno[base3d_gci + p]
    int base4d_queue;  // la coda dei docenti di questa cella: queue_mul[base4d_queue ...]
} Cella;

static Cella cella(const int pos) {
    Cella c = {pos, g_giorno(pos), g_ora(pos), g_classe(pos)};
    c.base2d_ci = c.cla * INSEGNANTE_SZ;
    c.base3d_goi = c.giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + c.ora * INSEGNANTE_SZ;
    c.base3d_gci = c.giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + c.cla * INSEGNANTE_SZ;
    c.base4d_queue = c.giorno * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ
                     + c.ora * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + c.cla * INSEGNANTE_SZ;
    return c;
}

// le ore che restano al docente p nella classe della cella (da solo e in codocenza)
static int ore_rimaste(const Cella *c, const int p) {
    return ore_classi_docenti[c->base2d_ci + p] - ore_classi_docenti_svolte[c->base2d_ci + p];
}

// ---- i vincoli della classe ----

// La classe puo' avere lezione in questa cella?
// - le ore che le restano (contando questa) devono bastare per le celle obbligatorie che restano: le prime
//   PRIME_N_ORE_OBBLIGATORIE ore di ogni giorno. Prima di questa cella ce ne sono giorno * PRIME_N_ORE_OBBLIGATORIE
//   + ora (oggi); dopo l'ultima ora obbligatoria il conto si ferma a PRIME_N_ORE_OBBLIGATORIE - 1, cosi' una lezione
//   in un'ora facoltativa lascia almeno un'ora per ognuna delle celle obbligatorie dei giorni dopo;
// - dopo un'ora buca la giornata della classe e' finita.
int activities_for_this_class_and_hour(const Cella *c) {
    const int ore_rimanenti = ore_classi[c->cla] - ore_classe_svolte[c->cla];
    const int slot_obbligatori_prima = c->giorno * PRIME_N_ORE_OBBLIGATORIE + MIN(c->ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    const int slot_mancanti = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana - slot_obbligatori_prima;
    if (ore_rimanenti < slot_mancanti) return 0;
    return n_ore_buche[c->giorno * CLASSE_SEZIONE_SZ + c->cla] == 0;
}

// ---- i vincoli del docente ----

// a quest'ora non insegna gia' in un'altra classe
int prof_è_disponibile(const Cella *c, const int p) {
    return !prof_gia_impegnato_giorno_ora[c->base3d_goi + p];
}

// ha ancora ore da fare in questa classe
int deve_ancora_svolgere_ore_in_questa_classe(const Cella *c, const int p) {
    return ore_classi_docenti_svolte[c->base2d_ci + p] < ore_classi_docenti[c->base2d_ci + p];
}

// oggi in questa classe ha fatto meno di MAX_ORE_PROF_IN_UNA_CLASSE ore
int prof_fa_al_max_n_ore(const Cella *c, const int p) {
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[c->base3d_gci + p] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

// le sue ore di oggi in questa classe sono di seguito: se ne ha gia' fatte, l'ultima e' l'ora prima (la cella
// pos - classi, stessa classe un'ora prima)
int prof_fa_ore_contigue(const Cella *c, const int p) {
    if (!ore_fatte_da_un_prof_in_una_classe_in_giorno[c->base3d_gci + p]) return 1;
    const teaching_slot *prima = &orario[c->pos - actual_classi_sezione];
    return prima->insegnante1 == p || prima->insegnante2 == p;
}

// oggi insegna al massimo in MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse, contando questa: si scorrono
// le celle di oggi gia' decise (in ogni ora il docente e' in una classe sola: trovata quella si passa all'ora dopo)
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(const Cella *c, const int p) {
    const int inizio = c->giorno * actual_ore_giornaliere * actual_classi_sezione;
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO];
    int n = 0;
    classi[n++] = c->cla;
    for (int i = inizio; i < c->pos; i++) {
        if (orario[i].insegnante1 != p && orario[i].insegnante2 != p) continue;
        const int classe = g_classe(i);
        int gia = 0;
        for (int j = 0; j < n && !gia; j++) gia = classi[j] == classe;
        if (!gia) {
            if (n >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
            classi[n++] = classe;
        }
        i = inizio + (g_ora(i) + 1) * actual_classi_sezione - 1;  // all'ora dopo (il ciclo aggiunge 1)
    }
    return 1;
}

// e' gia' entrato a scuola
int verifica_ora_ingresso(const int p, const int ora) {
    return ora >= ora_di_entrata_prof[p];
}

// con il codocente p2 ha ancora ore da fare insieme in questa classe
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const Cella *c, const int p1, const int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(c->cla, p1, p2) <
           get_ore_insegnanti_codocenza_in_una_classe(c->cla, p1, p2);
}

// ha ancora ore da fare da solo (senza codocente) in questa classe
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const Cella *c, const int p) {
    return ore_classi_docenti_alone_svolte[c->base2d_ci + p] < ore_classi_docenti_alone[c->base2d_ci + p];
}

// La regola del sabato. Il sabato e' l'ultimo giorno: un docente che e' nell'ora prima in questa classe e ha ancora
// ore da fare qui non potrebbe tornare dopo (le ore di un giorno sono di seguito) e non ci sono altri giorni, quindi
// deve continuare adesso. Dice quale dei docenti dell'ora prima deve esserci: 1 il primo, 2 il secondo, 3 tutti e due,
// 0 nessuno (non e' sabato, e' la prima ora, o nessuno dei due ha ancora ore qui).
static int sabato_chi_deve_continuare(const Cella *c, int *prev_prof1, int *prev_prof2) {
    if (c->giorno != actual_giorni_settimana - 1 || c->ora == 0) return 0;
    *prev_prof1 = orario[c->pos - actual_classi_sezione].insegnante1;
    *prev_prof2 = orario[c->pos - actual_classi_sezione].insegnante2;
    const int r1 = *prev_prof1 != -1 && ore_rimaste(c, *prev_prof1) > 0;
    const int r2 = *prev_prof2 != -1 && ore_rimaste(c, *prev_prof2) > 0;
    return r1 + 2 * r2;
}

// la coppia (prof1, prof2) rispetta la regola del sabato: contiene chi deve continuare (tutti e due, se tutti e due)
int sabato_esaurisci_priorita_subito_cod(const Cella *c, const int prof1, const int prof2) {
    int prev_prof1, prev_prof2;
    switch (sabato_chi_deve_continuare(c, &prev_prof1, &prev_prof2)) {
        case 3: return prof1 == prev_prof1 && prof2 == prev_prof2 || prof1 == prev_prof2 && prof2 == prev_prof1;
        case 1: return prof1 == prev_prof1 || prof2 == prev_prof1;
        case 2: return prof1 == prev_prof2 || prof2 == prev_prof2;
        default: return 1;
    }
}

// il docente prof da solo rispetta la regola del sabato: e' lui che deve continuare (se devono continuare tutti e due,
// da solo non va bene)
int sabato_esaurisci_priorita_subito(const Cella *c, const int prof) {
    int prev_prof1, prev_prof2;
    switch (sabato_chi_deve_continuare(c, &prev_prof1, &prev_prof2)) {
        case 3: return 0;
        case 1: return prof == prev_prof1;
        case 2: return prof == prev_prof2;
        default: return 1;
    }
}

// I controlli del docente p1 in questa cella, prima di cercargli un codocente o metterlo da solo.
static int docente_puo_insegnare(const Cella *c, const int p1) {
    return prof_è_disponibile(c, p1)
           && deve_ancora_svolgere_ore_in_questa_classe(c, p1)
           && prof_fa_al_max_n_ore(c, p1)
           && prof_fa_ore_contigue(c, p1)
           && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(c, p1)
           && verifica_ora_ingresso(p1, c->ora);
}

// I controlli del codocente p2 insieme a p1. (L'ora di ingresso del codocente non si controlla.)
static int codocente_puo_insegnare(const Cella *c, const int p1, const int p2) {
    return prof_è_disponibile(c, p2)
           && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(c, p1, p2)
           && prof_fa_al_max_n_ore(c, p2)
           && prof_fa_ore_contigue(c, p2)
           && sabato_esaurisci_priorita_subito_cod(c, p1, p2)
           && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(c, p2);
}

// I controlli per mettere p1 da solo.
static int docente_puo_insegnare_da_solo(const Cella *c, const int p1) {
    return deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(c, p1)
           && sabato_esaurisci_priorita_subito(c, p1);
}

// ---- la coda dei docenti ----

// dalla priorita' piu' alta (piu' ore da fare) alla piu' bassa
int compare_queue_items(const void *a, const void *b) {
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;
    if (itemA->priority < itemB->priority) return 1;
    if (itemA->priority > itemB->priority) return -1;
    return 0;
}

// Ordina la coda della cella (ogni cella ha la sua copia dei puntatori, vedi prio_queue.h). A parita' di priorita'
// l'ordine dipende da qsort, che non e' stabile: per questo l'impronta di orario.txt cambia tra Mac e Linux.
void sort_queue_mul(const size_t start, const size_t n) {
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

// ---- mettere e togliere un docente ----

// Un'ora del docente di it in questa cella (delta = 1) o tolta (delta = -1): priorita' nella coda, impegnato a
// quest'ora, ore fatte nella classe e ore fatte oggi nella classe.
static void segna_ora(const Cella *c, QueueItem *it, const int delta) {
    const int p = it->teacher;
    it->priority -= delta;
    prof_gia_impegnato_giorno_ora[c->base3d_goi + p] = delta > 0;
    ore_classi_docenti_svolte[c->base2d_ci + p] += delta;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[c->base3d_gci + p] += delta;
}

static void segna_da_solo(const Cella *c, QueueItem *it, const int delta) {
    segna_ora(c, it, delta);
    ore_classi_docenti_alone_svolte[c->base2d_ci + it->teacher] += delta;
}

static void segna_coppia(const Cella *c, QueueItem *it, QueueItem *jt, const int delta) {
    segna_ora(c, it, delta);
    segna_ora(c, jt, delta);
    if (delta > 0) increment_ore_classi_codocenza_svolte(c->cla, it->teacher, jt->teacher);
    else decrement_ore_classi_codocenza_svolte(c->cla, it->teacher, jt->teacher);
}

// ---- la ricerca ----

void print_orario(int pos);

// Prova tutte le lezioni possibili in questa cella, nell'ordine della coda: ogni docente prima con ognuno dei
// codocenti dopo di lui nella coda, poi da solo. Ritorna 1 se almeno una lezione e' stata provata.
static int prova_lezioni(const Cella *c) {
    int provata = 0;
    const int fine = c->base4d_queue + queue_sz;
    for (int ii = c->base4d_queue; ii < fine; ii++) {
        QueueItem *it = queue_mul[ii];
        const int p1 = it->teacher;
        if (!docente_puo_insegnare(c, p1)) continue;
        for (int jj = ii + 1; jj < fine; jj++) {
            QueueItem *jt = queue_mul[jj];
            const int p2 = jt->teacher;
            if (!codocente_puo_insegnare(c, p1, p2)) continue;
            segna_coppia(c, it, jt, 1);
            provata = 1;
            orario[c->pos] = (teaching_slot){p1, p2};
            print_orario(c->pos + 1);
            segna_coppia(c, it, jt, -1);
        }
        if (docente_puo_insegnare_da_solo(c, p1)) {
            segna_da_solo(c, it, 1);
            provata = 1;
            orario[c->pos] = (teaching_slot){p1, -1};
            print_orario(c->pos + 1);
            segna_da_solo(c, it, -1);
        }
    }
    return provata;
}

// L'ora buca: solo dopo le ore obbligatorie; da li' la classe non ha piu' lezione oggi.
static void prova_ora_buca(const Cella *c) {
    if (c->ora < PRIME_N_ORE_OBBLIGATORIE) return;
    orario[c->pos] = (teaching_slot){-1, -1};
    n_ore_buche[c->giorno * CLASSE_SEZIONE_SZ + c->cla]++;
    print_orario(c->pos + 1);
    n_ore_buche[c->giorno * CLASSE_SEZIONE_SZ + c->cla]--;
}

void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;
    if (pos == orario_len) {  // tutte le celle decise: un orario completo
        n_gen++;
        print(orario, pos, n_gen, f);
        return;
    }
    const Cella c = cella(pos);
    sort_queue_mul(c.base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(&c)) {
        ore_classe_svolte[c.cla]++;  // la lezione di questa cella conta gia' tra le ore della classe
        const int provata = prova_lezioni(&c);
        ore_classe_svolte[c.cla]--;
        if (!provata) {  // backtrack: nessun docente possibile in questa cella
            fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                    giorni_settimana[c.giorno], ore_giornaliere[c.ora], classe_sezione[c.cla]);
            print(orario, pos, n_gen, f);
        }
    }
    prova_ora_buca(&c);
}

int main(void) {
    time_t adesso;
    time(&adesso);
    const clock_t inizio = clock();
    printf("Program started at: %s\n", ctime(&adesso));

    inizializza_dati();
    stampa_dati();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;

    f = fopen("orario.txt", "w");
    print_orario(0);

    const double elapsed = (clock() - inizio) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) elapsed / 60, (int) elapsed % 60,
           (double) n_gen / elapsed);
    fflush(stdout);
    fclose(f);
    return 0;
}
