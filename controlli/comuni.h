// Lo stato e i controlli della ricerca uguali in main e nei solver che decidono le celle una alla volta con la stessa
// coda (solutori/giorni-liberi, ordini-classi). Non e' un header normale: contiene definizioni e va incluso
// una volta sola, nel main.c del solver, dopo gli altri #include. Cosi' le funzioni sono nello stesso file della ricerca
// (il compilatore le espande come prima: stessa velocita') e restano visibili alla traccia, alle potature e al
// controllo in avanti, che le chiamano per nome. Ogni solver aggiunge i controlli che dipendono dall'ordine delle celle
// o dai suoi giri (activities_for_this_class_and_hour, ora_buca_possibile, sabato_esaurisci_priorita_subito_cod,
// ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno) e la sua ricerca.
#ifndef CONTROLLI_COMUNI_H
#define CONTROLLI_COMUNI_H

#include "controlli.h"  // le dichiarazioni (anche dei controlli che definisce il solver)

#define SCHEDULES_PER_GB 55000  // orari completi per GB di orario.txt; senza -n se ne cercano un quinto (circa 200 MB)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int n_gen = 0;                        // orari completi trovati
long long int n_gen_max = SCHEDULES_PER_GB / 5; // quanti cercarne al massimo (-n)
// l'orario: una cella per (giorno, ora, classe), con i due docenti (-1: nessuno)
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ... GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1] = (teaching_slot){-1, -1}
};
int orario_len;  // quante celle ha l'orario (giorni x ore x classi)
FILE *f;         // orario.txt (NULL con --txt /dev/null)

// ---- le celle: pos = (giorno * ore + ora) * classi + classe nell'orario ----

int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) { return p % actual_classi_sezione; }

// la cella dell'ora prima della stessa classe
static const teaching_slot *cella_prima(const int pos) { return &orario[pos - actual_classi_sezione]; }

// ---- le ore che restano ----

// le ore che restano al docente p nella classe (base2d_ci = classe * INSEGNANTE_SZ)
static int ore_rimaste(const int base2d_ci, const int p) {
    return ore_classi_docenti[base2d_ci + p] - ore_classi_docenti_svolte[base2d_ci + p];
}

// l'indice di (giorno, classe, docente) in ore_fatte_da_un_prof_in_una_classe_in_giorno
static int idx_oggi(const int giorno, const int cla, const int p) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + p;
}

// ---- la regola del sabato, da solo (la coppia e' nel solver) ----

int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof) {
    return sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, prof, -1);
}

// ---- i controlli sul singolo docente ----

int prof_è_disponibile(const int base3d_goi, int p1) {
    return !prof_gia_impegnato_giorno_ora[base3d_goi + p1];
}

int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) {
    return ore_classi_docenti_svolte[base2d_ci + p1] < ore_classi_docenti[base2d_ci + p1];
}

// oggi in questa classe ha fatto meno di MAX_ORE_PROF_IN_UNA_CLASSE ore
int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof) {
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

// se oggi ha gia' fatto ore in questa classe, c'era anche nell'ora prima (il blocco continua); altrimenti il suo blocco
// e' finito e oggi non puo' tornare
int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof) {
    if (!ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]) return 1;
    const teaching_slot *prima = &orario[(giorno * actual_ore_giornaliere + ora - 1) * actual_classi_sezione + cla];
    return prima->insegnante1 == prof || prima->insegnante2 == prof;
}

int verifica_ora_ingresso(const int prof, const int ora) {
    return ora >= ora_di_entrata_prof[prof];
}

// ---- i controlli sulla coppia e sul docente da solo ----

int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, int p1, int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2) <
           get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2);
}

int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < ore_classi_docenti_alone[base2d_ci + prof];
}

// ---- mettere e togliere un docente dalla cella (lo stato della ricerca) ----

// verso: +1 mette il docente nella cella (priorita' -1, occupato in quest'ora, un'ora in piu' nella classe e oggi),
// -1 lo toglie
static void segna(const int base2d_ci, const int base3d_goi, QueueItem *it, const int giorno, const int cla,
                  const int verso) {
    it->priority -= verso;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = verso > 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher] += verso;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, it->teacher)] += verso;
}

void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, +1);
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
}

void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, -1);
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
}

void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, +1);
    segna(base2d_ci, base3d_goi, jt, giorno, cla, +1);
    increment_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    segna(base2d_ci, base3d_goi, it, giorno, cla, -1);
    segna(base2d_ci, base3d_goi, jt, giorno, cla, -1);
    decrement_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

// ---- la coda dei docenti ----
// In ogni cella si provano prima i docenti con meno margine: il margine e' quante ore il docente puo' ancora fare da
// quest'ora alla fine della settimana (oggi da adesso, o dal suo ingresso se entra dopo; nei giorni dopo dal suo
// ingresso) meno le ore che gli restano in tutto (la priorita' della coda). A parita' di margine prima chi ha piu' ore
// che restano, poi l'indice del docente (prima il piu' basso): senza, l'ordine dei pari lo sceglierebbe qsort, che su
// Linux (glibc) e su macOS li mette in modo diverso, e la ricerca cambierebbe da una macchina all'altra.
// Senza ritardi (tutti entrano alla prima ora) il margine e' lo stesso numero meno la priorita', quindi l'ordine e'
// quello di prima (piu' ore che restano prima). Con i ritardi chi entra tardi ha meno ore a disposizione e va prima: sui
// casi di prova con ritardi trova molti piu' orari (7 novembre 2026, richiesta di Alb: 140 su 342 casi invece di 126).
static double margine_coda[INSEGNANTE_SZ];

// il margine del docente all'ora `ora`, con `giorni_dopo` giorni ancora da riempire dopo questo (anche per la traccia)
double margine_docente(const QueueItem *it, const int ora, const int giorni_dopo) {
    const int O = actual_ore_giornaliere, ingresso = ora_di_entrata_prof[it->teacher];
    const int oggi = O - (ora > ingresso ? ora : ingresso);
    return (oggi > 0 ? oggi : 0) + giorni_dopo * (O - ingresso) - it->priority;
}

int compare_queue_items(const void *a, const void *b) {
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;
    const double ma = margine_coda[itemA->teacher], mb = margine_coda[itemB->teacher];
    if (ma < mb) return -1;
    if (ma > mb) return 1;
    if (itemA->priority < itemB->priority) return 1;
    if (itemA->priority > itemB->priority) return -1;
    return (itemA->teacher > itemB->teacher) - (itemA->teacher < itemB->teacher);
}

// la coda della cella (start, n docenti) all'ora `ora`, con `giorni_dopo` giorni ancora da riempire dopo questo
void sort_queue_mul(size_t start, size_t n, const int ora, const int giorni_dopo) {
    for (size_t i = 0; i < n; i++) {
        const QueueItem *it = queue_mul[start + i];
        margine_coda[it->teacher] = margine_docente(it, ora, giorni_dopo);
    }
    qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items);
}

#endif
