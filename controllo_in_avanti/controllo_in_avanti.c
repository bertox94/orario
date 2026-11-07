#include "controllo_in_avanti.h"

#include <stdio.h>
#include <string.h>

#include "../data_initialization/database_initializers.h"

// Il controllo in avanti e la verifica dei dati: vedi controllo_in_avanti.h.

#include "../controlli/controlli.h"

// ---- l'ordine della ricerca ----
static const int *cella_al_passo, *passo_della_cella;     // NULL: la cella s al passo s
static const int *giorno_al_posto, *posto_del_giorno;     // NULL: i giorni del calendario

static int cella_di_passo(const int s) { return cella_al_passo ? cella_al_passo[s] : s; }
static int passo(const int c) { return passo_della_cella ? passo_della_cella[c] : c; }
static int posto(const int g) { return posto_del_giorno ? posto_del_giorno[g] : g; }

// il giorno che si riempie dopo g (per indicare una cella del giorno dopo nella traccia)
static int giorno_dopo(const int g) {
    const int r = posto(g) + 1;
    return giorno_al_posto ? giorno_al_posto[r] : r;
}

void controllo_in_avanti_ordine_giorni(const int *giorno_di_posto, const int *posto_di_giorno) {
    giorno_al_posto = giorno_di_posto;
    posto_del_giorno = posto_di_giorno;
}

static int cella(const int giorno, const int ora, const int cla) {
    return (giorno * actual_ore_giornaliere + ora) * actual_classi_sezione + cla;
}

static int idx_oggi(const int giorno, const int cla, const int p) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + p;
}

static int ore_rimaste(const int c, const int p) {
    return ore_classi_docenti[c * INSEGNANTE_SZ + p] - ore_classi_docenti_svolte[c * INSEGNANTE_SZ + p];
}

// -- i dati che servono, preparati una volta: i docenti e le voci di ogni classe --
// una voce e' una lezione che la classe deve fare: un docente da solo (q < 0) o una coppia (p < q)
#define MAX_VOCI 64
static int docenti_di_classe[CLASSE_SEZIONE_SZ][INSEGNANTE_SZ], n_docenti_di_classe[CLASSE_SEZIONE_SZ];
static int voce_p[CLASSE_SEZIONE_SZ][MAX_VOCI], voce_q[CLASSE_SEZIONE_SZ][MAX_VOCI], n_voci[CLASSE_SEZIONE_SZ];

void prepara_controllo_in_avanti(const int *cella_di, const int *indice_di) {
    cella_al_passo = cella_di;
    passo_della_cella = indice_di;
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

// i numeri dei motivi 2 e 3
typedef struct { int docente, classe, restano, possibili; } Blocco;
static Blocco blocco;

// -- lo stato di oggi dopo le prime s celle --
typedef struct {
    int giorno, inizio;                // il giorno della cella appena decisa e la sua prima cella
    int da[CLASSE_SEZIONE_SZ];         // per ogni classe la prima ora non ancora decisa oggi (O: tutte decise)
    int classi_oggi[INSEGNANTE_SZ];    // in quante classi diverse ha gia' insegnato oggi ogni docente
} Oggi;

static void calcola_oggi(const int s, Oggi *o) {
    static int timbri[INSEGNANTE_SZ][CLASSE_SEZIONE_SZ], timbro = 0;  // per contare ogni classe una volta sola
    const int O = actual_ore_giornaliere, C = actual_classi_sezione;
    o->giorno = g_giorno(cella_di_passo(s - 1));
    o->inizio = cella(o->giorno, 0, 0);
    timbro++;
    memset(o->classi_oggi, 0, sizeof(int) * actual_insegnanti);
    for (int c = 0; c < C; c++) {
        int h = 0;
        while (h < O && passo(cella(o->giorno, h, c)) < s) h++;
        o->da[c] = h;
    }
    for (int i = o->inizio; i < o->inizio + O * C; i++) {
        const int c = g_classe(i);
        if (g_ora(i) >= o->da[c]) continue;  // non ancora decisa
        const int pp[2] = {orario[i].insegnante1, orario[i].insegnante2};
        for (int k = 0; k < 2; k++) {
            if (pp[k] < 0 || timbri[pp[k]][c] == timbro) continue;
            timbri[pp[k]][c] = timbro;
            o->classi_oggi[pp[k]]++;
        }
    }
}

// quante ore puo' ancora dare oggi il docente p nella classe c: 0 se il suo blocco e' chiuso (ha gia' insegnato oggi
// nella classe ma non nell'ultima ora decisa) o se non ci ha insegnato e ha gia' il massimo di classi oggi
static int massimo_oggi(const Oggi *o, const int c, const int p) {
    const int fatte = ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(o->giorno, c, p)];
    if (fatte) {
        const int u = o->da[c] - 1;
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

// -- 1. le celle di oggi: la prima cella obbligatoria ancora da decidere senza nessun docente possibile, o -1 --
static int cella_senza_docenti(const Oggi *o) {
    const int fine = o->inizio + PRIME_N_ORE_OBBLIGATORIE * actual_classi_sezione;  // dopo: ore che possono restare vuote
    for (int q = o->inizio; q < fine; q++) {
        const int h = g_ora(q), c = g_classe(q);
        if (h < o->da[c]) continue;  // gia' decisa
        int possibile = 0;
        for (int k = 0; k < n_docenti_di_classe[c] && !possibile; k++) {
            const int p = docenti_di_classe[c][k];
            possibile = ore_rimaste(c, p) > 0 && libero(o, h, p) && massimo_oggi(o, c, p) > 0;
        }
        if (!possibile) return q;
    }
    return -1;
}

// -- 2. il conteggio delle lezioni --
// Per la classe c, dalla sua prima ora non ancora decisa oggi (da) alle ore obbligatorie: fino a ogni ora h servono
// (h - da + 1) lezioni oggi (se h >= da) e (h + 1) in ognuno dei giorni dopo (dopo). Una voce ne puo' dare: oggi,
// quante ne entrano tra da e h nelle ore in cui i suoi docenti sono liberi (al massimo massimo_oggi di ognuno); in
// ognuno dei giorni dopo, al massimo MAX_ORE_PROF_IN_UNA_CLASSE tra le 0 e h dalle ore in cui sono entrati; in tutto,
// non piu' delle ore che le restano. Poi (2b) fino all'ultima ora: tutte le ore che restano alla classe. Ritorna la
// cella dell'ora per cui non bastano, o -1.
static int ultima_ora_non_basta;

static int conteggio_lezioni(const Oggi *o, const int c, const int dopo) {
    int solo_oggi[ORA_GIORNALIERA_SZ] = {0}, con_dopo[ORA_GIORNALIERA_SZ] = {0};  // lezioni possibili fino a ogni ora
    const int O = actual_ore_giornaliere, da = o->da[c];
    const int chiusa = n_ore_buche[o->giorno * CLASSE_SEZIONE_SZ + c] > 0;        // oggi ha gia' avuto un'ora buca
    int tutto_oggi = 0, tutto_giorno = 0, tutto = 0;                               // 2b: fino all'ultima ora
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
        // 2b: fino all'ultima ora
        int oggi = 0;
        if (!chiusa && da < O) {
            int m = massimo_oggi(o, c, p);
            if (q >= 0) m = MIN(m, massimo_oggi(o, c, q));
            for (int h = da; h < O && oggi < m; h++) oggi += libero(o, h, p) && (q < 0 || libero(o, h, q));
        }
        const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, O - entra));
        tutto_oggi += MIN(resto, oggi);
        tutto_giorno += MIN(resto, al_giorno);
        tutto += MIN(resto, oggi + al_giorno * dopo);
    }
    for (int h = da; h < PRIME_N_ORE_OBBLIGATORIE; h++)
        if (solo_oggi[h] < h - da + 1) return cella(o->giorno, h, c);
    for (int h = 0; h < PRIME_N_ORE_OBBLIGATORIE; h++)
        if (con_dopo[h] < MAX(0, h - da + 1) + (h + 1) * dopo)
            return cella(h >= da ? o->giorno : giorno_dopo(o->giorno), h, c);
    const int celle_oggi = chiusa ? 0 : MAX(0, O - da);
    const int possibili = MIN(tutto, MIN(tutto_oggi, celle_oggi) + dopo * MIN(tutto_giorno, O));
    const int restano = ore_classi[c] - ore_classe_svolte[c];
    if (possibili < restano) {
        ultima_ora_non_basta = 1;
        blocco = (Blocco){-1, c, restano, possibili};
        return cella(o->giorno, O - 1, c);
    }
    return -1;
}

// -- 3. le ore dei docenti: 1 se a un docente non basta il tempo (con i numeri in blocco), se no 0 --
static int docente_senza_tempo(const Oggi *o, const int dopo) {
    const int O = actual_ore_giornaliere;
    int aperta[ORA_GIORNALIERA_SZ] = {0};  // l'ora h di oggi ha ancora una cella da decidere in una classe aperta
    for (int c = 0; c < actual_classi_sezione; c++) {
        const int da = o->da[c];
        if (n_ore_buche[o->giorno * CLASSE_SEZIONE_SZ + c]) continue;
        for (int h = da; h < O; h++) aperta[h] = 1;
        for (int k = 0; k < n_docenti_di_classe[c]; k++) {  // in ogni classe
            const int p = docenti_di_classe[c][k];
            const int restano = ore_rimaste(c, p);
            if (!restano) continue;
            int oggi = 0;
            for (int h = da, m = massimo_oggi(o, c, p); h < O && oggi < m; h++) oggi += libero(o, h, p);
            const int possibili = oggi + dopo * MIN(MAX_ORE_PROF_IN_UNA_CLASSE, O - ora_di_entrata_prof[p]);
            if (restano > possibili) {
                blocco = (Blocco){p, c, restano, possibili};
                return 1;
            }
        }
    }
    // (vale anche per le classi chiuse oggi da un'ora buca: li' oggi non si fa piu' niente)
    for (int c = 0; c < actual_classi_sezione; c++) {
        if (!n_ore_buche[o->giorno * CLASSE_SEZIONE_SZ + c]) continue;
        for (int k = 0; k < n_docenti_di_classe[c]; k++) {
            const int p = docenti_di_classe[c][k];
            const int restano = ore_rimaste(c, p);
            const int possibili = dopo * MIN(MAX_ORE_PROF_IN_UNA_CLASSE, O - ora_di_entrata_prof[p]);
            if (restano > possibili) {
                blocco = (Blocco){p, c, restano, possibili};
                return 1;
            }
        }
    }
    for (int p = 0; p < actual_insegnanti; p++) {  // in tutto
        int restano = 0;
        for (int c = 0; c < actual_classi_sezione; c++) restano += ore_rimaste(c, p);
        if (!restano) continue;
        int possibili = dopo * (O - ora_di_entrata_prof[p]);
        for (int h = 0; h < O; h++) possibili += aperta[h] && libero(o, h, p);
        if (restano > possibili) {
            blocco = (Blocco){p, -1, restano, possibili};
            return 1;
        }
    }
    return 0;
}

// -- tutto insieme --
int motivo_in_avanti = 0;

const char *nome_motivo_in_avanti(const int m) {
    return m == 0 ? "cella" : m == 1 ? "ore" : m == 2 ? "ore_classe" : "ore_docente";
}

void numeri_in_avanti(int *docente, int *classe, int *restano, int *possibili) {
    *docente = blocco.docente;
    *classe = blocco.classe;
    *restano = blocco.restano;
    *possibili = blocco.possibili;
}

int docenti_della_cella(const int s, const int q, DocenteCella *dc, const int n) {
    static Oggi o;
    calcola_oggi(s, &o);
    const int h = g_ora(q), c = g_classe(q), C = actual_classi_sezione;
    int k = 0;
    for (int i = 0; i < n_docenti_di_classe[c] && k < n; i++) {
        const int p = docenti_di_classe[c][i];
        DocenteCella *x = &dc[k++];
        x->docente = p;
        x->restano = ore_rimaste(c, p);
        x->ingresso = ora_di_entrata_prof[p];
        x->occupato = -1;
        if (prof_gia_impegnato_giorno_ora[o.giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + h * INSEGNANTE_SZ + p])
            for (int cc = 0; cc < C; cc++) {
                const teaching_slot *t = &orario[cella(o.giorno, h, cc)];
                if (h < o.da[cc] && (t->insegnante1 == p || t->insegnante2 == p)) x->occupato = cc;
            }
        x->fatte_oggi = ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(o.giorno, c, p)];
        x->massimo_oggi = massimo_oggi(&o, c, p);
        x->blocco_chiuso = x->fatte_oggi > 0 && x->massimo_oggi == 0;
        x->classi_oggi = o.classi_oggi[p];
    }
    return k;
}

int controllo_in_avanti(const int s) {
    static Oggi o;
    calcola_oggi(s, &o);
    motivo_in_avanti = 0;
    int q = cella_senza_docenti(&o);
    if (q >= 0) return q;
    const int dopo = actual_giorni_settimana - 1 - posto(o.giorno);
    ultima_ora_non_basta = 0;
    for (int c = 0; c < actual_classi_sezione && q < 0; c++) q = conteggio_lezioni(&o, c, dopo);
    if (q >= 0) {
        motivo_in_avanti = ultima_ora_non_basta ? 2 : 1;
        return q;
    }
    motivo_in_avanti = 3;
    return docente_senza_tempo(&o, dopo) ? cella_di_passo(MIN(s, orario_len - 1)) : -1;
}

// ---- la verifica dei dati (prima della ricerca) ----
// G giorni, O ore al giorno, M ore obbligatorie, al massimo MAX_ORE_PROF_IN_UNA_CLASSE (3) ore al giorno di un docente
// in una classe e MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO (3) classi al giorno; un docente che entra all'ora k ha
// O - k ore al giorno.
//  1. docente in una classe: ore <= G x min(3, O - k)              (entra alla quarta ora: al massimo 6 x 3 = 18)
//  2. docente in tutto: ore <= G x (O - k)
//  3. docente, classi al giorno: ogni classe gli prende almeno ore/3 giorni (arrotondato in su), al massimo 3 classi al
//     giorno: la somma <= 3 x G
//  4. classe, ore obbligatorie: per ogni ora h < M, le lezioni possibili tra la prima ora e h (per voce: un docente da
//     solo o una coppia, dall'ingresso, al massimo 3 al giorno e le ore della voce) bastano per (h + 1) x G
//  5. ora per ora: alle ore obbligatorie i docenti gia' entrati (con lezioni) sono almeno quante le classi
// (Per una coppia non serve un controllo a parte: le ore insieme sono anche ore di chi entra piu' tardi, e per lui
// vale gia' la 1.)
int verifica_fattibilita(char *errore, const size_t dimensione) {
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
