// Solver «in-avanti»: la ricerca di main (stessi controlli, stesso ordine delle celle e della coda) più un controllo in
// avanti dopo ogni scelta (vedi «controllo in avanti» qui sotto e solutori/LEGGIMI.md).
//
// L'orario e' un vettore di celle: pos = (giorno * ore + ora) * classi + classe, cioe' un giorno alla volta, un'ora
// alla volta, tutte le classi di quell'ora. print_orario(pos) decide la cella pos e scende alla successiva.

#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "priority_queue/prio_queue.h"
#include "data_initialization/database_initializers.h"
#include "printer/printer.h"
#include "ingresso_uscita/dati_json.h"
#include "ingresso_uscita/traccia.h"

#define SCHEDULES_PER_GB 55000
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

long long int n_gen = 0;                              // orari trovati
long long int n_gen_max = SCHEDULES_PER_GB / 5;      // quanti cercarne al massimo (-n)
teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ] = {
    [0 ... GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ - 1] = (teaching_slot){-1, -1}
};
int orario_len;                                       // quante celle ha l'orario
FILE *f;                                              // orario.txt (gli orari trovati e i backtrack); NULL: niente

// ---- posizioni e indici ----

int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) { return p % actual_classi_sezione; }

// la cella (giorno, ora, classe)
static int cella(const int giorno, const int ora, const int cla) {
    return (giorno * actual_ore_giornaliere + ora) * actual_classi_sezione + cla;
}

// l'indice di ore_fatte_da_un_prof_in_una_classe_in_giorno: le ore che il docente p ha fatto oggi nella classe
static int idx_oggi(const int giorno, const int cla, const int p) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + p;
}

// le ore che restano al docente p nella classe (base2d_ci = classe * INSEGNANTE_SZ)
static int ore_rimaste(const int base2d_ci, const int p) {
    return ore_classi_docenti[base2d_ci + p] - ore_classi_docenti_svolte[base2d_ci + p];
}

// ---- i controlli della cella (come in main) ----

// La classe puo' fare lezione in quest'ora: le ore che le restano bastano per gli slot obbligatori che mancano (questo
// compreso) e oggi non ha ancora avuto un'ora buca (dopo un'ora buca, oggi niente lezioni).
int activities_for_this_class_and_hour(int pos, const int gio, const int ora, const int cla) {
    (void) pos;
    const int ore_rimanenti = ore_classi[cla] - ore_classe_svolte[cla];
    const int slot_obbligatori_gia_occupati = gio * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    const int slot_mancanti = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana - slot_obbligatori_gia_occupati;
    if (ore_rimanenti < slot_mancanti) return 0;
    return n_ore_buche[gio * CLASSE_SEZIONE_SZ + cla] == 0;
}

// Regole del sabato (l'ultimo giorno): chi era nella classe nell'ora prima e ha ancora ore deve restare, perche' non ci
// sono altri giorni per finirle e le sue ore devono essere contigue.
// - in coppia: se a tutti e due quelli dell'ora prima restano ore, la coppia deve essere proprio loro; se resta a uno
//   solo, deve esserci lui;
// - da solo: se restano ore a tutti e due quelli dell'ora prima, nessuno da solo va bene; se resta a uno, deve essere lui.
static int ha_ancora_ore(const int base2d_ci, const int p) { return p >= 0 && ore_rimaste(base2d_ci, p) > 0; }

int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2) {
    if (giorno != actual_giorni_settimana - 1 || ora == 0) return 1;
    const int a = orario[pos - actual_classi_sezione].insegnante1, b = orario[pos - actual_classi_sezione].insegnante2;
    const int resta_a = ha_ancora_ore(base2d_ci, a), resta_b = ha_ancora_ore(base2d_ci, b);
    if (resta_a && resta_b) return (prof1 == a && prof2 == b) || (prof1 == b && prof2 == a);
    if (resta_a) return prof1 == a || prof2 == a;
    if (resta_b) return prof1 == b || prof2 == b;
    return 1;
}

int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof) {
    if (giorno != actual_giorni_settimana - 1 || ora == 0) return 1;
    const int a = orario[pos - actual_classi_sezione].insegnante1, b = orario[pos - actual_classi_sezione].insegnante2;
    const int resta_a = ha_ancora_ore(base2d_ci, a), resta_b = ha_ancora_ore(base2d_ci, b);
    if (resta_a && resta_b) return 0;  // devono esserci tutti e due: da solo nessuno va bene
    if (resta_a) return prof == a;
    if (resta_b) return prof == b;
    return 1;
}

// il docente e' libero in quest'ora (base3d_goi: giorno e ora)
int prof_è_disponibile(const int base3d_goi, const int p1) { return !prof_gia_impegnato_giorno_ora[base3d_goi + p1]; }

// il docente ha ancora ore da fare nella classe
int deve_ancora_svolgere_ore_in_questa_classe(const int base2d_ci, const int p1) { return ore_rimaste(base2d_ci, p1) > 0; }

// oggi il docente ha fatto nella classe meno del massimo di ore
int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof) {
    (void) ora;
    return ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

// le ore di oggi del docente nella classe restano contigue: se oggi ci ha gia' insegnato, c'era nell'ora prima
int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof) {
    if (!ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]) return 1;
    const teaching_slot *prima = &orario[cella(giorno, ora - 1, cla)];
    return prima->insegnante1 == prof || prima->insegnante2 == prof;
}

// docente e codocente hanno ancora ore da fare insieme nella classe
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const int cla, int p1, int p2) {
    return get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2) <
           get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2);
}

// il docente ha ancora ore da solo (senza codocente) nella classe
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const int base2d_ci, const int prof) {
    return ore_classi_docenti_alone_svolte[base2d_ci + prof] < ore_classi_docenti_alone[base2d_ci + prof];
}

// Oggi il docente resta entro MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse, contando questa: si guardano
// le celle di oggi gia' decise (prima di pos). Un docente sta in una sola classe per ora: trovato in un'ora, si passa
// all'ora dopo.
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, const int prof) {
    const int inizio = cella(giorno, 0, 0);
    int classi[MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO], n = 0;
    classi[n++] = cla;
    for (int i = inizio; i < pos; i++) {
        if (orario[i].insegnante1 != prof && orario[i].insegnante2 != prof) continue;
        const int c = g_classe(i);
        int gia = 0;
        for (int j = 0; j < n; j++) gia |= classi[j] == c;
        if (!gia) {
            if (n >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
            classi[n++] = c;
        }
        i = inizio + (g_ora(i) + 1) * actual_classi_sezione - 1;  // all'ora dopo (il ciclo aggiunge 1)
    }
    return 1;
}

// il docente e' gia' entrato a quest'ora
int verifica_ora_ingresso(const int prof, const int ora) { return ora >= ora_di_entrata_prof[prof]; }

// L'ora buca e' possibile solo dopo le ore obbligatorie e se le ore che restano alla classe stanno ancora nei giorni
// dopo (un'ora buca chiude la giornata). Se no la classe deve fare lezione: senza docenti la ricerca torna indietro.
int ora_buca_possibile(const int giorno, const int ora, const int cla) {
    return ora >= PRIME_N_ORE_OBBLIGATORIE
           && ore_classi[cla] - ore_classe_svolte[cla] <= actual_ore_giornaliere * (actual_giorni_settimana - 1 - giorno);
}

// ---- la coda dei docenti ----
// In ogni cella i docenti si provano in ordine di priorita' (le ore che restano loro), la piu' alta prima; a parita'
// decide l'indice del docente, cosi' l'ordine non dipende da qsort (e da macOS o Linux).
int compare_queue_items(const void *a, const void *b) {
    const QueueItem *x = *(const QueueItem **) a, *y = *(const QueueItem **) b;
    if (x->priority < y->priority) return 1;
    if (x->priority > y->priority) return -1;
    return (x->teacher > y->teacher) - (x->teacher < y->teacher);
}

void sort_queue_mul(size_t start, size_t n) { qsort(&queue_mul[start], n, sizeof(QueueItem *), compare_queue_items); }

// ---- mettere e togliere una scelta (prima e dopo la discesa) ----
// mettere: il docente (o la coppia) e' occupato in quest'ora, ha un'ora in piu' nella classe (da solo o insieme) e oggi;
// la sua priorita' scende. Togliere fa il contrario.

void prep_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority--;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 1;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]++;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]++;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]++;
}

void post_chiamata_ricorsiva(const int base2d_ci, const int base3d_goi, QueueItem *it, int giorno, int cla, int prof) {
    it->priority++;
    prof_gia_impegnato_giorno_ora[base3d_goi + it->teacher] = 0;
    ore_classi_docenti_svolte[base2d_ci + it->teacher]--;
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher]--;
    ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, prof)]--;
}

void prep_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    QueueItem *q[2] = {it, jt};
    for (int z = 0; z < 2; z++) {
        q[z]->priority--;
        prof_gia_impegnato_giorno_ora[base3d_goi + q[z]->teacher] = 1;
        ore_classi_docenti_svolte[base2d_ci + q[z]->teacher]++;
        ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, q[z]->teacher)]++;
    }
    increment_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

void post_chiamata_ricorsiva_insieme(const int base2d_ci, const int base3d_goi, const int cla, QueueItem *it,
                                     QueueItem *jt, int giorno) {
    QueueItem *q[2] = {it, jt};
    for (int z = 0; z < 2; z++) {
        q[z]->priority++;
        prof_gia_impegnato_giorno_ora[base3d_goi + q[z]->teacher] = 0;
        ore_classi_docenti_svolte[base2d_ci + q[z]->teacher]--;
        ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, q[z]->teacher)]--;
    }
    decrement_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
}

// ---- controllo in avanti (solo in questo solver) ----
// Dopo ogni scelta (una coppia o un docente nella cella pos) si guarda se il resto e' ancora risolvibile, con due
// controlli di fattibilita'. Sono condizioni necessarie: se dicono "no" e' impossibile e la scelta si scarta subito;
// se dicono "si'" non e' garantito niente.
//  1. le celle di oggi, una per una: ogni cella obbligatoria che resta oggi ha ancora almeno un docente possibile;
//  2. le classi, una per una: il conteggio delle lezioni possibili basta per le ore da riempire, oggi e nei giorni che
//     restano.
// Si fanno a ogni scelta perche' dipendono dallo stato, che cambia a ogni cella riempita (e costano poco: classi x
// voci x ore). La funzione da chiamare e' cella_bloccata_dopo_la_scelta(pos); la traccia la usa per il controllo
// lascia_un_docente_alle_celle_di_oggi.

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
                voce_p[c][n_voci[c]] = p;
                voce_q[c][n_voci[c]++] = -1;
            }
            for (int q = p + 1; q < actual_insegnanti; q++)
                if (get_ore_insegnanti_codocenza_in_una_classe(c, p, q) && n_voci[c] < MAX_VOCI) {
                    voce_p[c][n_voci[c]] = p;
                    voce_q[c][n_voci[c]++] = q;
                }
        }
    }
}

// le ore che restano a una voce: da solo, o insieme
static int ore_rimaste_voce(const int c, const int p, const int q) {
    return q < 0
        ? ore_classi_docenti_alone[c * INSEGNANTE_SZ + p] - ore_classi_docenti_alone_svolte[c * INSEGNANTE_SZ + p]
        : get_ore_insegnanti_codocenza_in_una_classe(c, p, q) - get_ore_insegnanti_codocenza_svolte_in_una_classe(c, p, q);
}

// -- lo stato di oggi dopo la scelta in pos --
typedef struct {
    int giorno, ora, cla, inizio;      // la cella appena decisa e la prima cella del giorno
    int classi_oggi[INSEGNANTE_SZ];    // in quante classi diverse ha gia' insegnato oggi ogni docente
} Oggi;

static void calcola_oggi(const int pos, Oggi *o) {
    static int timbri[INSEGNANTE_SZ][CLASSE_SEZIONE_SZ], timbro = 0;  // per contare ogni classe una volta sola
    o->giorno = g_giorno(pos);
    o->ora = g_ora(pos);
    o->cla = g_classe(pos);
    o->inizio = cella(o->giorno, 0, 0);
    timbro++;
    memset(o->classi_oggi, 0, sizeof(int) * actual_insegnanti);
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

// quante ore puo' ancora dare oggi il docente p nella classe c: 0 se il suo blocco e' chiuso (ha gia' insegnato oggi
// nella classe ma non nell'ultima ora decisa) o se non ci ha insegnato e ha gia' il massimo di classi oggi
static int massimo_oggi(const Oggi *o, const int c, const int p) {
    const int fatte = ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(o->giorno, c, p)];
    if (fatte) {
        const int u = ultima_ora(o, c);
        const teaching_slot *ultima = u >= 0 ? &orario[o->inizio + u * actual_classi_sezione + c] : NULL;
        if (!ultima || (ultima->insegnante1 != p && ultima->insegnante2 != p)) return 0;
    } else if (o->classi_oggi[p] >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;
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
            possibile = ore_rimaste(c * INSEGNANTE_SZ, p) > 0 && libero(o, h, p) && massimo_oggi(o, c, p) > 0;
        }
        if (!possibile) return q;
    }
    return -1;
}

// -- 2. il conteggio delle lezioni --
// Per la classe c, dalla sua prima ora non ancora decisa oggi (da) alle ore obbligatorie: fino a ogni ora h servono
// (h - da + 1) lezioni oggi (se h >= da) e (h + 1) in ognuno dei giorni dopo (dopo). Le lezioni si contano per voce,
// come le fa la ricerca: un docente da solo, o una coppia (una lezione con due docenti, possibile solo quando tutti e
// due sono entrati). Una voce ne puo' dare: oggi, quante ne entrano tra da e h nelle ore in cui i suoi docenti sono
// liberi (al massimo massimo_oggi di ognuno); in ognuno dei giorni dopo, al massimo MAX_ORE_PROF_IN_UNA_CLASSE tra le
// 0 e h dalle ore in cui sono entrati; in tutto, non piu' delle ore che le restano.
// Si guarda prima oggi da solo, poi oggi insieme ai giorni dopo (in una passata sola). Ritorna la cella dell'ora per
// cui non bastano (oggi, o il giorno dopo), o -1.
// Esempio (test 309): alla C07 restano Kakashi da solo e Hinata con Zoro, e Zoro entra alle 11: fino alle 10 di ogni
// giorno che resta servono 3 lezioni e ci sono solo quelle di Kakashi.
static int conteggio_lezioni(const Oggi *o, const int c, const int da, const int dopo) {
    int solo_oggi[ORA_GIORNALIERA_SZ] = {0}, con_dopo[ORA_GIORNALIERA_SZ] = {0};  // lezioni possibili fino a ogni ora
    for (int v = 0; v < n_voci[c]; v++) {
        const int p = voce_p[c][v], q = voce_q[c][v];
        const int resto = ore_rimaste_voce(c, p, q);
        if (resto <= 0) continue;
        int massimo = 0;  // oggi: solo se resta qualcosa da decidere
        if (da < PRIME_N_ORE_OBBLIGATORIE) {
            massimo = MIN(resto, massimo_oggi(o, c, p));
            if (q >= 0) massimo = MIN(massimo, massimo_oggi(o, c, q));
        }
        const int entra = MAX(ora_di_entrata_prof[p], q >= 0 ? ora_di_entrata_prof[q] : 0);
        int ore = 0;  // le lezioni della voce oggi, fino all'ora h
        for (int h = 0; h < PRIME_N_ORE_OBBLIGATORIE; h++) {
            if (h >= da && ore < massimo && libero(o, h, p) && (q < 0 || libero(o, h, q))) ore++;
            const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, h + 1 - entra));
            solo_oggi[h] += MIN(resto, ore);
            con_dopo[h] += MIN(resto, ore + al_giorno * dopo);
        }
    }
    for (int h = da; h < PRIME_N_ORE_OBBLIGATORIE; h++)
        if (solo_oggi[h] < h - da + 1) return cella(o->giorno, h, c);
    for (int h = 0; h < PRIME_N_ORE_OBBLIGATORIE; h++)
        if (con_dopo[h] < MAX(0, h - da + 1) + (h + 1) * dopo) return cella(o->giorno + (h >= da ? 0 : 1), h, c);
    return -1;
}

// -- tutto insieme --
int motivo_blocco = 0;  // perche' la scelta si scarta: 0 una cella senza docenti, 1 non bastano le lezioni

int cella_bloccata_dopo_la_scelta(const int pos) {
    static Oggi o;
    calcola_oggi(pos, &o);
    motivo_blocco = 0;
    int q = cella_senza_docenti(pos, &o);
    if (q >= 0) return q;
    motivo_blocco = 1;
    const int dopo = actual_giorni_settimana - 1 - o.giorno;
    for (int c = 0; c < actual_classi_sezione && q < 0; c++) q = conteggio_lezioni(&o, c, ultima_ora(&o, c) + 1, dopo);
    return q;
}

// ---- verifiche di fattibilita' (sui soli dati, prima della ricerca) ----
// Condizioni necessarie che si vedono senza cercare: se una non vale, l'orario non esiste e la ricerca non parte (il
// test finisce con «errore nei dati» e il motivo). G giorni, O ore al giorno, M ore obbligatorie, al massimo
// MAX_ORE_PROF_IN_UNA_CLASSE (3) ore al giorno di un docente in una classe e MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO
// (3) classi al giorno; un docente che entra all'ora k ha O - k ore al giorno.
//  1. docente in una classe: ore <= G x min(3, O - k)              (entra alla quarta ora: al massimo 6 x 3 = 18)
//  2. docente in tutto: ore <= G x (O - k)
//  3. docente, classi al giorno: ogni classe gli prende almeno ore/3 giorni (arrotondato in su), al massimo 3 classi al
//     giorno: la somma <= 3 x G
//  4. classe, ore obbligatorie: per ogni ora h < M, le lezioni possibili tra la prima ora e h (per voce: un docente da
//     solo o una coppia, dall'ingresso, al massimo 3 al giorno e le ore della voce) bastano per (h + 1) x G
//  5. ora per ora: alle ore obbligatorie i docenti gia' entrati (con lezioni) sono almeno quante le classi
// (Per una coppia non serve un controllo a parte: le ore insieme sono anche ore di chi entra piu' tardi, e per lui
// vale gia' la 1.) Ritorna 1 se tutto va, se no 0 con il motivo in errore.
static int verifica_fattibilita(char *errore, const size_t dimensione) {
    const int G = actual_giorni_settimana, O = actual_ore_giornaliere;
    const int maxo = MAX_ORE_PROF_IN_UNA_CLASSE, maxc = MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO;
    const int M = MIN(PRIME_N_ORE_OBBLIGATORIE, O);
    for (int p = 0; p < actual_insegnanti; p++) {
        const int k = ora_di_entrata_prof[p], al_giorno = O - k;
        int totale = 0, giorni_classe = 0;
        for (int c = 0; c < actual_classi_sezione; c++) {
            const int ore = ore_classi_docenti[c * INSEGNANTE_SZ + p];
            if (!ore) continue;
            totale += ore;
            giorni_classe += (ore + maxo - 1) / maxo;
            if (ore > G * MIN(maxo, al_giorno)) {  // 1
                snprintf(errore, dimensione, "%s in %s: %d ore, ma %s%s ne puo' fare al massimo %d (%d al giorno per "
                         "%d giorni)", insegnanti[p], classe_sezione[c], ore, k ? "entrando alle " : "",
                         k ? ore_giornaliere[k] : "", G * MIN(maxo, al_giorno), MIN(maxo, al_giorno), G);
                return 0;
            }
        }
        if (totale > G * al_giorno) {  // 2
            snprintf(errore, dimensione, "%s: %d ore in tutto, ma %s%s ne puo' fare al massimo %d (%d al giorno per %d "
                     "giorni)", insegnanti[p], totale, k ? "entrando alle " : "", k ? ore_giornaliere[k] : "",
                     G * al_giorno, al_giorno, G);
            return 0;
        }
        if (giorni_classe > maxc * G) {  // 3
            snprintf(errore, dimensione, "%s: le sue classi gli prendono almeno %d giornate-classe (al massimo %d ore al "
                     "giorno in una classe), ma con %d classi al giorno ne ha %d", insegnanti[p], giorni_classe, maxo,
                     maxc, maxc * G);
            return 0;
        }
    }
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int h = 0; h < M; h++) {  // 4
            int possibili = 0;
            for (int v = 0; v < n_voci[c]; v++) {
                const int p = voce_p[c][v], q = voce_q[c][v];
                const int entra = MAX(ora_di_entrata_prof[p], q >= 0 ? ora_di_entrata_prof[q] : 0);
                possibili += MIN(ore_rimaste_voce(c, p, q), G * MIN(maxo, MAX(0, h + 1 - entra)));
            }
            if (possibili < (h + 1) * G) {
                snprintf(errore, dimensione, "%s: dalle %s alle %s servono %d lezioni in una settimana (%d al giorno), "
                         "ma con gli ingressi dei suoi docenti ce ne possono essere al massimo %d", classe_sezione[c],
                         ore_giornaliere[0], ore_giornaliere[h], (h + 1) * G, h + 1, possibili);
                return 0;
            }
        }
    }
    for (int h = 0; h < M; h++) {  // 5
        int presenti = 0;
        for (int p = 0; p < actual_insegnanti; p++) presenti += ore_docenti[p] > 0 && ora_di_entrata_prof[p] <= h;
        if (presenti < actual_classi_sezione) {
            snprintf(errore, dimensione, "alle %s sono entrati solo %d docenti, ma le classi sono %d (e l'ora e' "
                     "obbligatoria)", ore_giornaliere[h], presenti, actual_classi_sezione);
            return 0;
        }
    }
    return 1;
}

// ---- la ricerca ----
// print_orario(pos) decide la cella pos. Se la classe puo' fare lezione, prova i docenti della coda in ordine: per
// ognuno che passa i controlli base prova le coppie con i docenti dopo di lui nella coda, poi lui da solo; ogni scelta
// che passa anche il controllo in avanti scende alla cella dopo. Se nessuna scelta e' possibile e' un backtrack (scritto
// in orario.txt e nella traccia). Poi, se si puo', prova l'ora buca.
void print_orario(const int pos) {
    if (n_gen_max <= n_gen) return;
    // traccia: solo conteggio dei nodi e, ogni tanto, il progresso (non cambia la ricerca)
    if (!(++traccia_nodi & 0xFFFF)) traccia_progresso(pos);
    if (traccia_con_nodi) traccia_nodo(pos);

    if (pos == orario_len) {  // tutte le celle decise: un orario
        n_gen++;
        if (f) print(orario, pos, n_gen, f);
        traccia_orario_trovato();
        return;
    }

    const int giorno = g_giorno(pos), ora = g_ora(pos), cla = g_classe(pos);
    const int base2d_ci = cla * INSEGNANTE_SZ;                                         // classe, docente
    const int base3d_goi = giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + ora * INSEGNANTE_SZ;  // giorno, ora, docente
    const int base4d_queue = ((giorno * ORA_GIORNALIERA_SZ + ora) * CLASSE_SEZIONE_SZ + cla) * INSEGNANTE_SZ;  // la coda

    sort_queue_mul(base4d_queue, queue_sz);
    if (activities_for_this_class_and_hour(pos, giorno, ora, cla)) {
        int found_prof = 0;
        ore_classe_svolte[cla]++;
        const int fine_coda = base4d_queue + queue_sz;
        for (int ii = base4d_queue; ii < fine_coda; ii++) {
            QueueItem *it = queue_mul[ii];
            const int p1 = it->teacher;
            // i controlli base del docente
            if (!(prof_è_disponibile(base3d_goi, p1)
                  && deve_ancora_svolgere_ore_in_questa_classe(base2d_ci, p1)
                  && prof_fa_al_max_n_ore(giorno, ora, cla, p1)
                  && prof_fa_ore_contigue(giorno, ora, cla, p1)
                  && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p1)
                  && verifica_ora_ingresso(p1, ora)))
                continue;
            // in coppia, con i docenti dopo di lui nella coda
            for (int jj = ii + 1; jj < fine_coda; jj++) {
                QueueItem *jt = queue_mul[jj];
                const int p2 = jt->teacher;
                if (!(prof_è_disponibile(base3d_goi, p2)
                      && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(cla, p1, p2)
                      && prof_fa_al_max_n_ore(giorno, ora, cla, p2)
                      && prof_fa_ore_contigue(giorno, ora, cla, p2)
                      && sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, p1, p2)
                      && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p2)
                      && verifica_ora_ingresso(p2, ora)))
                    continue;
                prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
                orario[pos] = (teaching_slot){p1, p2};
                if (cella_bloccata_dopo_la_scelta(pos) < 0) {  // controllo in avanti
                    found_prof = 1;
                    print_orario(pos + 1);
                }
                post_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
            }
            // da solo
            if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p1)
                && sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p1)) {
                prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
                orario[pos] = (teaching_slot){p1, -1};
                if (cella_bloccata_dopo_la_scelta(pos) < 0) {  // controllo in avanti
                    found_prof = 1;
                    print_orario(pos + 1);
                }
                post_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, p1);
            }
        }
        ore_classe_svolte[cla]--;
        if (!found_prof) {  // backtrack: nessuna scelta possibile in questa cella
            if (f) {  // in orario.txt tutta la griglia: e' la parte piu' lenta, per questo con --txt /dev/null si salta
                fprintf(f, "Backtrack, non si è potuto scegiere alcun insegnante nel giorno: %s, ora: %s, classe: %s\n",
                        giorni_settimana[giorno], ore_giornaliere[ora], classe_sezione[cla]);
                print(orario, pos, n_gen, f);
            }
            traccia_backtrack(pos, giorno, ora, cla, base2d_ci, base3d_goi, base4d_queue,
                              ora_buca_possibile(giorno, ora, cla));
        }
    } else if (ora < PRIME_N_ORE_OBBLIGATORIE) {
        // lo slot non puo' avere lezione e l'ora buca non e' ammessa: la ricerca torna indietro (solo traccia)
        traccia_slot_chiuso(pos, giorno, ora, cla);
    }

    if (ora_buca_possibile(giorno, ora, cla)) {
        orario[pos] = (teaching_slot){-1, -1};
        n_ore_buche[giorno * CLASSE_SEZIONE_SZ + cla]++;
        print_orario(pos + 1);
        n_ore_buche[giorno * CLASSE_SEZIONE_SZ + cla]--;
    }
}

// Uso: solutore [--input FILE] [--traccia FILE] [-n N] [--txt FILE] [--max-backtrack N] [--max-nodi N] [--progresso]
//   senza opzioni: i dati di inizializza_dati() e gli orari in orario.txt
//   --input FILE:       i dati da un file JSON (vedi ingresso_uscita/dati_json.h) invece che da inizializza_dati()
//   --traccia FILE:     la traccia della ricerca per il sito, un JSON per riga (vedi ingresso_uscita/traccia.h)
//   -n N:               quanti orari cercare al massimo (n_gen_max; senza: SCHEDULES_PER_GB / 5)
//   --txt FILE:         dove scrivere orario.txt (con /dev/null non si scrive per niente: piu' veloce)
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

    time_t adesso;
    time(&adesso);
    const clock_t inizio = clock();
    printf("Program started at: %s\n", ctime(&adesso));

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
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    prepara_controllo_in_avanti();

    traccia_dati();
    {
        char errore[512];
        if (!verifica_fattibilita(errore, sizeof errore)) {  // l'orario non esiste: la ricerca non parte
            traccia_errore(errore);
            traccia_chiudi();
            return 2;
        }
    }
    // con --txt /dev/null orario.txt non si scrive per niente: la ricerca e' la stessa, ma non perde tempo a formattare
    // la griglia a ogni backtrack (dal sito e' quasi tutto il tempo)
    f = strcmp(file_txt, "/dev/null") ? fopen(file_txt, "w") : NULL;
    print_orario(0);

    const double secondi = (clock() - inizio) / (double) CLOCKS_PER_SEC;
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) secondi / 60, (int) secondi % 60,
           (double) n_gen / secondi);
    fflush(stdout);
    traccia_fine(secondi);
    traccia_chiudi();
    if (f) fclose(f);
    return 0;
}
