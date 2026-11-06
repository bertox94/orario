#include "potature.h"

#include "../data_initialization/database_initializers.h"
#include "../controllo_in_avanti/controllo_in_avanti.h"

// Le potature (vedi potature.h: cosa sono, perche' sono necessarie, quando avvengono). Sono scritte sullo stato di
// questo solver e per qualunque ordine delle celle: per ogni classe conta la sua prima cella non decisa (giorno, ora).
// Le celle di una classe si decidono sempre in ordine di tempo, quindi prima di quella cella e' tutto deciso e da li' in
// poi niente.

extern teaching_slot orario[];
extern int orario_len;
int g_giorno(int pos);
int g_ora(int pos);
int g_classe(int pos);

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define N_CELLE (GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ)
static int rango[N_CELLE];      // a quale passo della ricerca si decide ogni cella
static const int *ordine;       // la cella decisa a ogni passo (NULL: la cella s al passo s)
// l'ordine dei giorni (giorni-liberi): il giorno che si riempie per r-esimo e il posto di ogni giorno (NULL: calendario)
static const int *giorno_al_posto, *posto_del_giorno;

// le lezioni di ogni classe ("voci"): un docente da solo (q = -1) o una coppia; ingresso: quando possono cominciare
#define MAX_VOCI_CLASSE (GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ)
static int voce_p[CLASSE_SEZIONE_SZ][MAX_VOCI_CLASSE], voce_q[CLASSE_SEZIONE_SZ][MAX_VOCI_CLASSE];
static int voce_ingresso[CLASSE_SEZIONE_SZ][MAX_VOCI_CLASSE], n_voci[CLASSE_SEZIONE_SZ];
static int voci_complete;       // 0 se una classe ha piu' voci di MAX_VOCI_CLASSE: allora le lezioni non si controllano

Potatura potatura;

static int cella(const int g, const int h, const int c) {
    return (g * actual_ore_giornaliere + h) * actual_classi_sezione + c;
}

static int cella_al_passo(const int s) {
    return ordine ? ordine[s] : s;
}

static int giorno_n(const int r) { return giorno_al_posto ? giorno_al_posto[r] : r; }

// quanti giorni si riempiono dopo il giorno g
static int giorni_dopo_il(const int g) {
    return actual_giorni_settimana - 1 - (posto_del_giorno ? posto_del_giorno[g] : g);
}

void potature_ordine_giorni(const int *giorno_di_posto, const int *posto_di_giorno) {
    giorno_al_posto = giorno_di_posto;
    posto_del_giorno = posto_di_giorno;
    controllo_in_avanti_ordine_giorni(giorno_di_posto, posto_di_giorno);
}

static int con_in_avanti = 1;  // A, il controllo in avanti (in-avanti lo fa da se', prima di scendere)
void potature_in_avanti(const int attivo) { con_in_avanti = attivo; }

static int docente_nella_cella(const int pos, const int p) {
    return orario[pos].insegnante1 == p || orario[pos].insegnante2 == p;
}

void prepara_potature(const int *cella_di) {
    ordine = cella_di;
    for (int s = 0; s < orario_len; s++) rango[cella_al_passo(s)] = s;
    prepara_controllo_in_avanti(cella_di, cella_di ? rango : NULL);
    voci_complete = 1;
    for (int c = 0; c < actual_classi_sezione; c++) {
        n_voci[c] = 0;
        for (int p = 0; p < actual_insegnanti; p++)
            for (int q = -1; q < actual_insegnanti; q++) {
                if (q >= 0 && q <= p) continue;
                const int ore = q < 0 ? ore_classi_docenti_alone[c * INSEGNANTE_SZ + p]
                                      : get_ore_insegnanti_codocenza_in_una_classe(c, p, q);
                if (!ore) continue;
                if (n_voci[c] == MAX_VOCI_CLASSE) {
                    voci_complete = 0;
                    continue;
                }
                const int v = n_voci[c]++;
                voce_p[c][v] = p;
                voce_q[c][v] = q;
                voce_ingresso[c][v] = MAX(ora_di_entrata_prof[p], q >= 0 ? ora_di_entrata_prof[q] : 0);
            }
    }
}

// -- la classe c dopo s passi: la sua prima cella non decisa --
typedef struct {
    int finita;      // tutte le celle della classe sono decise
    int giorno, ora; // la prima cella non decisa
    int chiusa;      // oggi la classe ha gia' avuto un'ora buca: oggi non fa piu' lezione
} Classe;

static Classe stato_classe(const int c, const int s) {
    Classe k = {1, 0, 0, 0};
    for (int r = 0; r < actual_giorni_settimana; r++)
        for (int g = giorno_n(r), h = 0; h < actual_ore_giornaliere; h++)
            if (rango[cella(g, h, c)] >= s) {
                k = (Classe){0, g, h, n_ore_buche[g * CLASSE_SEZIONE_SZ + c] > 0};
                return k;
            }
    return k;
}

// Quante ore il docente p puo' ancora fare oggi nella classe c, dalla prima cella non decisa della classe.
static int ore_oggi_in_classe(const Classe *k, const int c, const int p) {
    if (k->chiusa) return 0;
    const int fatte = ore_fatte_da_un_prof_in_una_classe_in_giorno[
        k->giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + c * INSEGNANTE_SZ + p];
    // le ore in una classe sono contigue: chi oggi ne ha gia' fatte ma non c'era nell'ora prima ha chiuso il blocco
    if (fatte && !docente_nella_cella(cella(k->giorno, k->ora - 1, c), p)) return 0;
    return MAX(0, MIN(MAX_ORE_PROF_IN_UNA_CLASSE - fatte,
                      actual_ore_giornaliere - MAX(k->ora, ora_di_entrata_prof[p])));
}

// ---- C. ore dei docenti in una classe (a ogni cella per la sua classe; a ogni ora chiusa e ora buca per tutte) ----
// Le ore di ogni docente nella classe c stanno nel tempo che resta? Oggi quante ne puo' fare li', poi al massimo 3 al
// giorno dal suo ingresso. Necessaria: in nessun orario un docente fa piu' di 3 ore al giorno in una classe, prima del
// suo ingresso o dopo aver chiuso il suo blocco di oggi.
static int ore_in_classe_stanno(const Classe *k, const int c) {
    if (k->finita) return 1;
    const int giorni_dopo = giorni_dopo_il(k->giorno);
    for (int p = 0; p < actual_insegnanti; p++) {
        const int restano = ore_classi_docenti[c * INSEGNANTE_SZ + p] - ore_classi_docenti_svolte[c * INSEGNANTE_SZ + p];
        if (!restano) continue;
        const int possibili = ore_oggi_in_classe(k, c, p) + giorni_dopo *
                              MIN(MAX_ORE_PROF_IN_UNA_CLASSE, actual_ore_giornaliere - ora_di_entrata_prof[p]);
        if (restano > possibili) {
            potatura = (Potatura){POTATURA_ORE_DOCENTI, p, c, restano, possibili, -1, k->giorno, k->ora};
            return 0;
        }
    }
    return 1;
}

// ---- T. ore dei docenti in tutto (a ogni ora chiusa e dopo ogni ora buca) ----
// Le ore di ogni docente, in tutto, stanno nelle ore libere che gli restano? Oggi (il giorno della prossima cella
// della ricerca): le ore dal suo ingresso in cui non insegna gia' e in cui qualche classe non e' ancora decisa (e non
// ha chiuso la giornata); poi tutte le ore dal suo ingresso dei giorni dopo. Necessaria: un docente fa al massimo una
// lezione per ora.
static int ore_totali_stanno(const Classe k[], const int s) {
    const int oggi = g_giorno(cella_al_passo(s));
    const int giorni_dopo = giorni_dopo_il(oggi);
    int aperta[ORA_GIORNALIERA_SZ] = {0};  // l'ora h di oggi ha ancora una cella da decidere in una classe aperta
    for (int c = 0; c < actual_classi_sezione; c++)
        if (!k[c].finita && k[c].giorno == oggi && !k[c].chiusa)
            for (int h = k[c].ora; h < actual_ore_giornaliere; h++) aperta[h] = 1;
    for (int p = 0; p < actual_insegnanti; p++) {
        int restano = 0;
        for (int c = 0; c < actual_classi_sezione; c++)
            restano += ore_classi_docenti[c * INSEGNANTE_SZ + p] - ore_classi_docenti_svolte[c * INSEGNANTE_SZ + p];
        if (!restano) continue;
        int possibili = giorni_dopo * (actual_ore_giornaliere - ora_di_entrata_prof[p]);
        for (int h = ora_di_entrata_prof[p]; h < actual_ore_giornaliere; h++)
            possibili += aperta[h] && !prof_gia_impegnato_giorno_ora[
                oggi * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + h * INSEGNANTE_SZ + p];
        if (restano > possibili) {
            potatura = (Potatura){POTATURA_ORE_DOCENTI, p, -1, restano, possibili, -1, oggi, -1};
            return 0;
        }
    }
    return 1;
}

// ---- L. lezioni della classe (a ogni cella, per la sua classe) ----
// Le lezioni che restano alla classe c bastano? Per ogni ora obbligatoria k: fino all'ora k la classe ha lezione in
// ogni cella, oggi (dalla sua prima cella non decisa) e in ogni giorno dopo; per l'ultima ora: fino a fine giornata
// devono stare tutte le ore che le restano. Fino all'ora k, in un giorno, una lezione occupa al massimo le ore dopo il
// suo ingresso e 3 ore (oggi: quante ne possono ancora fare i suoi docenti nella classe); in tutto, al massimo le ore
// che le restano. (Come lezioni_della_classe_bastano del solver precedente.) Necessaria: ogni cella piena e' una
// lezione, e nessuna lezione supera questi massimi.
static int lezioni_bastano(const Classe *k, const int c) {
    if (k->finita || !voci_complete) return 1;
    const int giorni_dopo = giorni_dopo_il(k->giorno);
    const int ultima = actual_ore_giornaliere - 1;
    const int ultima_oggi = k->chiusa ? k->ora - 1 : ultima;  // dopo un'ora buca oggi non c'e' piu' lezione
    int restano[MAX_VOCI_CLASSE], oggi_max[MAX_VOCI_CLASSE];
    for (int v = 0; v < n_voci[c]; v++) {
        const int p = voce_p[c][v], q = voce_q[c][v];
        restano[v] = q < 0
            ? ore_classi_docenti_alone[c * INSEGNANTE_SZ + p] - ore_classi_docenti_alone_svolte[c * INSEGNANTE_SZ + p]
            : get_ore_insegnanti_codocenza_in_una_classe(c, p, q) - get_ore_insegnanti_codocenza_svolte_in_una_classe(c, p, q);
        oggi_max[v] = !restano[v] ? 0 : q < 0 ? ore_oggi_in_classe(k, c, p)
                                               : MIN(ore_oggi_in_classe(k, c, p), ore_oggi_in_classe(k, c, q));
    }
    for (int h = 0; h <= ultima; h++) {
        if (h >= PRIME_N_ORE_OBBLIGATORIE && h != ultima) continue;  // le ore facoltative in mezzo non obbligano
        const int fino_oggi = MIN(h, ultima_oggi);
        int oggi = 0, in_un_giorno = 0, in_tutto = 0;
        for (int v = 0; v < n_voci[c]; v++) {
            if (!restano[v]) continue;
            const int ing = voce_ingresso[c][v];
            const int da_oggi = MAX(ing, k->ora);
            const int o = fino_oggi < da_oggi ? 0 : MIN(oggi_max[v], fino_oggi - da_oggi + 1);
            const int g = h < ing ? 0 : MIN(MAX_ORE_PROF_IN_UNA_CLASSE, h - ing + 1);
            oggi += MIN(restano[v], o);
            in_un_giorno += MIN(restano[v], g);
            in_tutto += MIN(restano[v], o + giorni_dopo * g);
        }
        const int celle_oggi = fino_oggi >= k->ora ? fino_oggi - k->ora + 1 : 0;
        const int possibili = MIN(in_tutto, MIN(oggi, celle_oggi) + giorni_dopo * MIN(in_un_giorno, h + 1));
        const int da_riempire = h == ultima ? ore_classi[c] - ore_classe_svolte[c]
                                            : giorni_dopo * (h + 1) + (h >= k->ora ? h - k->ora + 1 : 0);
        if (possibili < da_riempire) {
            potatura = (Potatura){POTATURA_LEZIONI_CLASSE, -1, c, da_riempire, possibili, h, k->giorno, k->ora};
            return 0;
        }
    }
    return 1;
}

// ---- quando: tutte insieme all'ingresso del passo s ----
int potatura_dopo(const int s) {
    if (s <= 0 || s >= orario_len) return 0;
    const int decisa = cella_al_passo(s - 1);
    const int c = g_classe(decisa), g = g_giorno(decisa), h = g_ora(decisa);
    const Classe k = stato_classe(c, s);
    // a ogni cella: L e C per la classe della cella appena decisa
    if (!lezioni_bastano(&k, c)) return POTATURA_LEZIONI_CLASSE;
    if (!ore_in_classe_stanno(&k, c)) return POTATURA_ORE_DOCENTI;
    // a ogni ora chiusa e dopo ogni ora buca: C per tutte le classi e T (il tempo passa per tutti; dopo un'ora buca la
    // classe perde le ore di oggi)
    int ora_chiusa = 1;
    for (int cc = 0; cc < actual_classi_sezione && ora_chiusa; cc++) ora_chiusa = rango[cella(g, h, cc)] < s;
    const int buca = orario[decisa].insegnante1 < 0 && orario[decisa].insegnante2 < 0;
    if (ora_chiusa || buca) {
        Classe tutte[CLASSE_SEZIONE_SZ];
        for (int cc = 0; cc < actual_classi_sezione; cc++) {
            tutte[cc] = stato_classe(cc, s);
            if (!ore_in_classe_stanno(&tutte[cc], cc)) return POTATURA_ORE_DOCENTI;
        }
        if (!ore_totali_stanno(tutte, s)) return POTATURA_ORE_DOCENTI;
    }
    // a ogni cella, alla fine: A, il controllo in avanti
    if (con_in_avanti) {
        const int q = controllo_in_avanti(s);
        if (q >= 0) {
            potatura = (Potatura){POTATURA_IN_AVANTI, -1, -1, 0, 0, -1, g_giorno(q), g_ora(q)};
            numeri_in_avanti(&potatura.docente, &potatura.classe, &potatura.restano, &potatura.possibili);
            potatura.cella = q;
            potatura.motivo = motivo_in_avanti;
            return POTATURA_IN_AVANTI;
        }
    }
    return 0;
}
