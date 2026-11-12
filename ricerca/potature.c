#include "potature.h"

#include <assert.h>
#include <stdlib.h>

// Le potature: vedi potature.h. Una funzione per potatura, con lo stesso nome del controllo della verifica dei dati
// che rifa'.

// Una lezione che la classe deve fare: un docente da solo (q < 0) o una coppia (p < q).
typedef struct {
    int p, q;
} Voce;

// Per ogni classe, i suoi docenti e le sue voci: non cambiano durante la ricerca.
struct Potature {
    int *docenti, n_docenti[CLASSE_SEZIONE_SZ];  // docenti[k * INSEGNANTE_SZ + i]: l'i-esimo docente della classe k
    Voce *voci;                                  // voci[inizio_voci[k] + i]: l'i-esima voce della classe k
    int inizio_voci[CLASSE_SEZIONE_SZ + 1];
};

void prepara_potature(Ricerca *r) {
    const Scuola *s = r->scuola;
    const int C = s->actual_classi_sezione, T = s->actual_insegnanti;
    struct Potature *t = calloc(1, sizeof *t);
    assert(t);
    t->docenti = malloc(sizeof(int) * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ);
    assert(t->docenti);
    int n_voci = 0;
    for (int giro = 0; giro < 2; giro++) {  // prima si contano le voci, poi si scrivono
        if (giro) {
            t->voci = malloc(sizeof(Voce) * (n_voci ? n_voci : 1));
            assert(t->voci);
        }
        n_voci = 0;
        for (int k = 0; k < C; k++) {
            t->inizio_voci[k] = n_voci;
            t->n_docenti[k] = 0;
            for (int p = 0; p < T; p++) {
                if (!get_ore_insegnante_in_una_classe(s, k, p)) continue;
                t->docenti[k * INSEGNANTE_SZ + t->n_docenti[k]++] = p;
                if (get_ore_insegnante_da_solo_in_una_classe(s, k, p)) {
                    if (giro) t->voci[n_voci] = (Voce){p, -1};
                    n_voci++;
                }
                for (int q = p + 1; q < T; q++) {
                    if (!get_ore_insegnanti_codocenza_in_una_classe(s, k, p, q)) continue;
                    if (giro) t->voci[n_voci] = (Voce){p, q};
                    n_voci++;
                }
            }
        }
        t->inizio_voci[C] = n_voci;
    }
    r->potature = t;
}

void libera_potature(Ricerca *r) {
    if (!r->potature) return;
    free(r->potature->docenti);
    free(r->potature->voci);
    free(r->potature);
    r->potature = NULL;
}

// ---- il punto della ricerca: la cella pos e quello che serve a tutte le potature ----

typedef struct {
    const Ricerca *r;
    const Scuola *s;
    int giorno, ora, cla;  // la cella pos, la prima non decisa
    int G, O, C, M;        // giorni, ore al giorno, classi, ore obbligatorie
    int dopo;              // i giorni dopo oggi
} Punto;

// la prima ora di oggi non ancora decisa nella classe k
static int prima_ora_libera(const Punto *x, const int k) {
    return x->ora + (k < x->cla);
}

static int ingresso(const Punto *x, const int p) {
    return x->s->ora_di_entrata_prof[p];
}

// le ore che restano al docente p nella classe k
static int ore_rimaste_in_classe(const Punto *x, const int k, const int p) {
    return x->s->ore_classi_docenti[k * INSEGNANTE_SZ + p] - x->r->ore_classi_docenti_svolte[k * INSEGNANTE_SZ + p];
}

// le ore che restano al docente p in tutte le classi: e' la sua priorita' nella coda
static int ore_rimaste_in_tutto(const Punto *x, const int p) {
    return x->r->queue_items[p].priority;
}

// il docente p oggi all'ora h insegna gia' (in una cella decisa)
static int impegnato(const Punto *x, const int h, const int p) {
    return x->r->prof_gia_impegnato_giorno_ora[(x->giorno * ORA_GIORNALIERA_SZ + h) * INSEGNANTE_SZ + p];
}

static int ha_ora_buca_oggi(const Punto *x, const int k) {
    return x->r->n_ore_buche[x->giorno * CLASSE_SEZIONE_SZ + k] > 0;
}

// Quante ore il docente p puo' ancora fare oggi nella classe k, dalla prima ora libera della classe fino all'ora
// `fino_a` compresa (vedi potature.h).
static int puo_fare_oggi(const Punto *x, const int k, const int p, const int fino_a) {
    const Ricerca *r = x->r;
    const int da = prima_ora_libera(x, k);
    if (da > fino_a || ha_ora_buca_oggi(x, k)) return 0;
    const int fatte = r->ore_fatte_da_un_prof_in_una_classe_in_giorno[
        (x->giorno * CLASSE_SEZIONE_SZ + k) * INSEGNANTE_SZ + p];
    if (fatte) {  // oggi qui ha gia' insegnato: solo continuando subito, dall'ora prima
        const teaching_slot prima = r->orario[(x->giorno * x->O + da - 1) * x->C + k];
        if (prima.insegnante1 != p && prima.insegnante2 != p) return 0;  // il suo blocco si e' chiuso
        if (impegnato(x, da, p)) return 0;                               // e adesso e' in un'altra classe
        return MIN(MAX_ORE_PROF_IN_UNA_CLASSE - fatte, fino_a - da + 1);
    }
    if (r->classi_fatte_da_un_prof_in_giorno[x->giorno * INSEGNANTE_SZ + p]
        >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return 0;  // oggi una classe nuova non la puo' piu' prendere
    int comincia = MAX(da, ingresso(x, p));
    if (comincia <= fino_a && impegnato(x, comincia, p)) comincia++;  // a quell'ora e' in un'altra classe
    return MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, fino_a - comincia + 1));
}

// ---- le potature ----

// 1. Un docente in una classe: le ore che gli restano li' stanno in quelle che ci puo' ancora fare, oggi
// (puo_fare_oggi) e nei giorni dopo (al massimo 3 al giorno, e meno se entra tardi).
static int le_ore_del_docente_in_una_classe_stanno_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int k = 0; k < x->C; k++) {
        for (int i = 0; i < t->n_docenti[k]; i++) {
            const int p = t->docenti[k * INSEGNANTE_SZ + i], restano = ore_rimaste_in_classe(x, k, p);
            if (!restano) continue;
            const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, x->O - ingresso(x, p));
            if (restano > x->dopo * al_giorno + puo_fare_oggi(x, k, p, x->O - 1)) return 0;
        }
    }
    return 1;
}

// 2. Un docente in tutto: le ore che gli restano stanno nelle ore libere che ha ancora, oggi (dal suo ingresso, da
// quest'ora, dove non insegna gia') e nei giorni dopo (tutte le ore dal suo ingresso).
static int le_ore_del_docente_stanno_nel_resto(const Punto *x) {
    for (int p = 0; p < x->s->actual_insegnanti; p++) {
        const int restano = ore_rimaste_in_tutto(x, p);
        if (!restano) continue;
        const int da = MAX(x->ora, ingresso(x, p));
        int libere = x->dopo * (x->O - ingresso(x, p)) + MAX(0, x->O - da);
        if (da == x->ora && impegnato(x, x->ora, p)) libere--;
        if (restano > libere) return 0;
    }
    return 1;
}

// 3. Un docente e le classi al giorno: tolto quello che puo' ancora fare oggi in ogni classe, quello che resta lo
// porta in quella classe in almeno (ore / 3, arrotondato in su) dei giorni dopo; in ognuno di quei giorni insegna in
// al massimo 3 classi.
static int le_classi_del_docente_stanno_in_tre_al_giorno_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    int servono[INSEGNANTE_SZ] = {0};
    for (int k = 0; k < x->C; k++) {
        for (int i = 0; i < t->n_docenti[k]; i++) {
            const int p = t->docenti[k * INSEGNANTE_SZ + i], restano = ore_rimaste_in_classe(x, k, p);
            if (!restano) continue;
            const int dopo_oggi = restano - puo_fare_oggi(x, k, p, x->O - 1);
            if (dopo_oggi > 0) servono[p] += (dopo_oggi + MAX_ORE_PROF_IN_UNA_CLASSE - 1) / MAX_ORE_PROF_IN_UNA_CLASSE;
        }
    }
    for (int p = 0; p < x->s->actual_insegnanti; p++)
        if (servono[p] > MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO * x->dopo) return 0;
    return 1;
}

// le ore che restano a una voce della classe k
static int ore_rimaste_alla_voce(const Punto *x, const int k, const Voce v) {
    const Ricerca *r = x->r;
    if (v.q < 0)
        return get_ore_insegnante_da_solo_in_una_classe(x->s, k, v.p)
               - r->ore_classi_docenti_alone_svolte[k * INSEGNANTE_SZ + v.p];
    return get_ore_insegnanti_codocenza_in_una_classe(x->s, k, v.p, v.q)
           - r->ore_classi_codocenza_svolte[k * INSEGNANTE_SZ * INSEGNANTE_SZ + v.p * INSEGNANTE_SZ + v.q];
}

// 4. Una classe in tutto: le ore che le restano stanno nelle lezioni che puo' ancora avere, oggi e nei giorni dopo.
// Oggi: non piu' delle ore che le restano oggi (dalla sua prima ora libera, se non ha gia' un'ora buca) e non piu' di
// quelle che le sue voci possono ancora fare oggi (puo_fare_oggi: al massimo 3 ore per docente, solo chi e' libero).
// Nei giorni dopo: non piu' delle ore di quei giorni, e ogni voce al massimo 3 al giorno dall'ingresso e non piu' delle
// ore che le restano. Esempio: l'ultimo giorno alla prima ora, una classe con una coppia che ha ancora 4 ore e due
// docenti con un'ora ha 6 ore da fare in 6 ore, ma la coppia oggi ne puo' fare 3: 5 lezioni, la sesta ora resta vuota.
static int le_ore_della_classe_stanno_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int k = 0; k < x->C; k++) {
        const int restano = x->s->ore_classi[k] - x->r->ore_classe_svolte[k];
        if (!restano) continue;
        int lezioni_oggi = 0, lezioni_dopo = 0, lezioni_un_giorno = 0;
        for (int i = t->inizio_voci[k]; i < t->inizio_voci[k + 1]; i++) {
            const Voce v = t->voci[i];
            const int ore = ore_rimaste_alla_voce(x, k, v);
            if (!ore) continue;
            int oggi = puo_fare_oggi(x, k, v.p, x->O - 1), entra = ingresso(x, v.p);
            if (v.q >= 0) {
                oggi = MIN(oggi, puo_fare_oggi(x, k, v.q, x->O - 1));
                entra = MAX(entra, ingresso(x, v.q));
            }
            const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, x->O - entra);
            lezioni_oggi += MIN(ore, oggi);
            lezioni_dopo += MIN(ore, x->dopo * al_giorno);
            lezioni_un_giorno += MIN(ore, al_giorno);
        }
        const int posti_oggi = ha_ora_buca_oggi(x, k) ? 0 : x->O - prima_ora_libera(x, k);
        const int dopo = MIN(lezioni_dopo, x->dopo * MIN(x->O, lezioni_un_giorno));
        if (restano > MIN(posti_oggi, lezioni_oggi) + dopo) return 0;
    }
    return 1;
}

// 5. Una classe e le ore obbligatorie: per ogni ora obbligatoria h, le celle che la classe deve ancora riempire tra la
// prima ora e h (oggi dalla sua prima ora libera, e tutti i giorni dopo) non sono piu' delle lezioni che le sue voci
// possono ancora fare in quelle ore: ogni voce, oggi quello che possono fare i suoi docenti (puo_fare_oggi), nei
// giorni dopo al massimo 3 al giorno dall'ingresso (per una coppia: di chi entra piu' tardi), e in tutto non piu' delle
// ore che le restano.
static int le_lezioni_bastano_per_le_ore_obbligatorie_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int k = 0; k < x->C; k++) {
        const int da = prima_ora_libera(x, k);
        for (int h = 0; h < x->M; h++) {
            int servono = x->dopo * (h + 1) + MAX(0, h - da + 1);
            for (int i = t->inizio_voci[k]; i < t->inizio_voci[k + 1] && servono > 0; i++) {
                const Voce v = t->voci[i];
                const int restano = ore_rimaste_alla_voce(x, k, v);
                if (!restano) continue;
                int oggi = puo_fare_oggi(x, k, v.p, h), entra = ingresso(x, v.p);
                if (v.q >= 0) {
                    oggi = MIN(oggi, puo_fare_oggi(x, k, v.q, h));
                    entra = MAX(entra, ingresso(x, v.q));
                }
                const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, h + 1 - entra));
                servono -= MIN(restano, oggi + x->dopo * al_giorno);
            }
            if (servono > 0) return 0;
        }
    }
    return 1;
}

// 6. Ora per ora: a un'ora obbligatoria ogni classe ha lezione e ogni lezione ha almeno un docente tutto suo. I
// docenti che hanno ancora ore e a quell'ora sono gia' entrati devono essere almeno quante le classi: a quest'ora di
// oggi le classi ancora da decidere e i docenti non gia' impegnati; nelle ore dopo di oggi, e nei giorni dopo, tutte
// le classi.
static int i_docenti_liberi_bastano_per_le_classi_nel_resto(const Punto *x) {
    int entrati_entro[ORA_GIORNALIERA_SZ + 1] = {0}, liberi_adesso = 0;  // con ore da fare, per ora di ingresso
    for (int p = 0; p < x->s->actual_insegnanti; p++) {
        if (!ore_rimaste_in_tutto(x, p)) continue;
        entrati_entro[ingresso(x, p)]++;
        liberi_adesso += ingresso(x, p) <= x->ora && !impegnato(x, x->ora, p);
    }
    if (x->ora < x->M && liberi_adesso < x->C - x->cla) return 0;
    for (int h = 0, entrati = 0; h < x->M; h++) {
        entrati += entrati_entro[h];
        if (entrati < x->C && (x->dopo > 0 || h > x->ora)) return 0;
    }
    return 1;
}

int potatura(const Ricerca *r, const int pos) {
    const Scuola *s = r->scuola;
    const int G = s->actual_giorni_settimana, O = s->actual_ore_giornaliere, C = s->actual_classi_sezione;
    const Punto x = {.r = r, .s = s, .giorno = pos / C / O, .ora = pos / C % O, .cla = pos % C, .G = G, .O = O, .C = C,
                     .M = MIN(PRIME_N_ORE_OBBLIGATORIE, O), .dopo = G - 1 - pos / C / O};
    int (*const controlli[N_POTATURE])(const Punto *) = {
        le_ore_del_docente_in_una_classe_stanno_nel_resto, le_ore_del_docente_stanno_nel_resto,
        le_classi_del_docente_stanno_in_tre_al_giorno_nel_resto, le_ore_della_classe_stanno_nel_resto,
        le_lezioni_bastano_per_le_ore_obbligatorie_nel_resto, i_docenti_liberi_bastano_per_le_classi_nel_resto,
    };
    for (int i = 0; i < N_POTATURE; i++)
        if (r->potature_attive >> i & 1 && !controlli[i](&x)) return i;
    return -1;
}
