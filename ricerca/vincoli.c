#include "vincoli.h"

// Potatura. Si scartano i rami in cui le ore che mancano a un docente non stanno piu' nel tempo rimasto:
//  - in una classe un docente fa al massimo MAX_ORE_PROF_IN_UNA_CLASSE ore al giorno (oggi: quelle che gli restano);
//  - in totale puo' fare al massimo una lezione per ora, dalla sua ora di ingresso.
// Sono condizioni necessarie: scartano solo rami senza soluzioni.

// Quante ore al massimo il docente p puo' ancora fare oggi nella classe c, dall'ora `ora` (non ancora cominciata).
static int capienza_oggi_in_classe(const int giorno, const int ora, const int c, const int p) {
    const int fatte_oggi = ore_fatte_da_un_prof_in_una_classe_in_giorno[
        giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + c * INSEGNANTE_SZ + p];
    const int ore_oggi = actual_ore_giornaliere - ora;
    int cap = MAX_ORE_PROF_IN_UNA_CLASSE - fatte_oggi;
    if (cap > ore_oggi) cap = ore_oggi;
    // le ore in una classe in un giorno sono consecutive: se ne ha gia' fatte oggi ma nell'ora precedente
    // non c'era, il suo blocco e' finito e oggi in questa classe non ne fa altre
    if (fatte_oggi > 0 && !docente_nello_slot(slot_precedente(giorno, ora, c), p)) cap = 0;
    return cap;
}

// Per ogni classe e docente: le ore rimaste stanno nei giorni rimasti? In totale[p] somma le ore rimaste di p.
static int ore_in_classe_stanno_nel_tempo_rimasto(const int giorno, const int ora, int totale[]) {
    const int giorni_dopo = actual_giorni_settimana - giorno - 1;
    for (int c = 0; c < actual_classi_sezione; c++) {
        const classe_docente_t *cd = &classe_docente[c * INSEGNANTE_SZ];
        for (int p = 0; p < actual_insegnanti; p++) {
            const int rimaste = cd[p].ore - cd[p].svolte;
            if (!rimaste) continue;
            if (rimaste > capienza_oggi_in_classe(giorno, ora, c, p) + MAX_ORE_PROF_IN_UNA_CLASSE * giorni_dopo)
                return 0;
            totale[p] += rimaste;
        }
    }
    return 1;
}

// Le ore rimaste di ogni docente (somma sulle classi) stanno nelle ore che gli restano, dalla sua ora di ingresso?
static int ore_totali_stanno_nel_tempo_rimasto(const int giorno, const int ora, const int totale[]) {
    const int giorni_dopo = actual_giorni_settimana - giorno - 1;
    for (int p = 0; p < actual_insegnanti; p++) {
        if (!totale[p]) continue;
        const int da = docente_info[p].ora_entrata > ora ? docente_info[p].ora_entrata : ora;
        const int ore_disponibili = (actual_ore_giornaliere - da)
                                    + (actual_ore_giornaliere - docente_info[p].ora_entrata) * giorni_dopo;
        if (totale[p] > ore_disponibili) return 0;
    }
    return 1;
}

/* ---- lezioni delle classi: bastano per gli slot che restano? ---- */

// Ogni slot pieno di una classe contiene una "lezione": un docente da solo o una coppia in codocenza.
typedef struct {
    int p;
    int q;        // -1: p insegna da solo
    int ingresso; // prima ora in cui la lezione puo' cadere (l'ingresso del docente entrato piu' tardi)
} lezione_t;

// Ogni lezione ha almeno un'ora, quindi una classe che ci sta nella settimana ne ha al massimo tante quanti gli slot.
#define MAX_LEZIONI_CLASSE (GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ)
static lezione_t lezioni_classe[CLASSE_SEZIONE_SZ][MAX_LEZIONI_CLASSE];
static int n_lezioni_classe[CLASSE_SEZIONE_SZ];
static int lezioni_complete = 0; // 0 se una classe ha piu' lezioni di MAX_LEZIONI_CLASSE: allora non si verifica

// Aggiunge alla classe c la lezione di p da solo (q = -1) o della coppia (p, q).
static void aggiungi_lezione(const int c, const int p, const int q) {
    if (n_lezioni_classe[c] == MAX_LEZIONI_CLASSE) {
        lezioni_complete = 0;
        return;
    }
    int ingresso = docente_info[p].ora_entrata;
    if (q >= 0 && docente_info[q].ora_entrata > ingresso) ingresso = docente_info[q].ora_entrata;
    lezioni_classe[c][n_lezioni_classe[c]++] = (lezione_t){p, q, ingresso};
}

// Le lezioni dipendono solo dai dati, non dalla ricerca: si elencano una volta, cosi' la verifica scorre solo le lezioni
// della classe (una dozzina) e non tutte le coppie di docenti possibili.
void prepara_lezioni_delle_classi(void) {
    lezioni_complete = 1;
    for (int c = 0; c < actual_classi_sezione; c++) {
        n_lezioni_classe[c] = 0;
        for (int p = 0; p < actual_insegnanti; p++) {
            if (classe_docente[c * INSEGNANTE_SZ + p].alone > 0) aggiungi_lezione(c, p, -1);
            for (int q = p + 1; q < actual_insegnanti; q++)
                if (codocenza[idx_codocenza(c, p, q)].ore > 0) aggiungi_lezione(c, p, q);
        }
    }
}

// Ore della lezione ancora da mettere nell'orario (da solo: alone - alone_svolte; in coppia: ore - svolte della coppia).
static int ore_rimaste_della_lezione(const int c, const lezione_t *l) {
    if (l->q < 0) {
        const classe_docente_t *cd = &classe_docente[c * INSEGNANTE_SZ + l->p];
        return cd->alone - cd->alone_svolte;
    }
    const codocenza_t *co = &codocenza[idx_codocenza(c, l->p, l->q)];
    return co->ore - co->svolte;
}

// Quante ore la lezione puo' ancora fare oggi nella classe, dall'ora `ora` (non ancora cominciata).
static int capienza_oggi_della_lezione(const int giorno, const int ora, const int c, const lezione_t *l) {
    const int cap = capienza_oggi_in_classe(giorno, ora, c, l->p);
    return l->q < 0 ? cap : MIN(cap, capienza_oggi_in_classe(giorno, ora, c, l->q));
}

// Quanti slot dall'ora `da` all'ora k (comprese) la lezione puo' occupare in un giorno: quelli dopo il suo ingresso.
static int slot_tra_le_ore(const lezione_t *l, const int da, const int k) {
    const int prima = l->ingresso > da ? l->ingresso : da;
    return k < prima ? 0 : k - prima + 1;
}

// Per la classe c, all'inizio dell'ora (giorno, ora): le lezioni che restano bastano per gli slot da riempire per forza?
//  - ore obbligatorie (k < PRIME_N_ORE_OBBLIGATORIE): fino all'ora k la classe ha lezione in ogni slot, ogni giorno;
//  - ultima ora: fino a fine giornata devono stare tutte le ore che restano alla classe.
// Fino all'ora k, in un giorno, una lezione occupa al massimo gli slot dopo il suo ingresso e
// MAX_ORE_PROF_IN_UNA_CLASSE slot (oggi: la capienza rimasta dei suoi docenti); in tutto, al massimo le sue ore rimaste.
// Senza questa verifica le lezioni dei docenti che entrano presto si spendono nelle ore tarde dei primi giorni e la
// ricerca se ne accorge solo giorni dopo, quando nella prima ora restano solo docenti non ancora entrati.
// Condizioni necessarie. Ritorna 1 se valgono; altrimenti 0, con in *ora_ko, *servono, *possibili l'ora k violata,
// gli slot da riempire fino a quell'ora e le lezioni che al massimo possono riempirli.
static int lezioni_della_classe_bastano(const int giorno, const int ora, const int c, int *ora_ko, int *servono,
                                        int *possibili) {
    const int giorni_dopo = actual_giorni_settimana - giorno - 1;
    const int ultima = actual_ore_giornaliere - 1;
    // dopo un'ora buca la classe oggi non fa piu' lezione
    const int ultima_oggi = n_ore_buche[giorno * CLASSE_SEZIONE_SZ + c] ? ora - 1 : ultima;
    const int n = n_lezioni_classe[c];
    const lezione_t *lez = lezioni_classe[c];
    int rimaste[MAX_LEZIONI_CLASSE], cap_oggi[MAX_LEZIONI_CLASSE];
    // cio' che non dipende da k: ore rimaste e capienza di oggi di ogni lezione
    for (int i = 0; i < n; i++) {
        rimaste[i] = ore_rimaste_della_lezione(c, &lez[i]);
        cap_oggi[i] = rimaste[i] ? capienza_oggi_della_lezione(giorno, ora, c, &lez[i]) : 0;
    }
    // k = ore obbligatorie (0..4) e ultima ora; le ore facoltative intermedie non obbligano a nulla
    for (int k = 0; k <= ultima; k++) {
        if (k >= PRIME_N_ORE_OBBLIGATORIE && k != ultima) continue;
        // per ogni lezione: quante ne possono cadere fino all'ora k oggi, in un giorno dopo, in tutto
        int oggi = 0, in_un_giorno = 0, in_tutto = 0;
        for (int i = 0; i < n; i++) {
            if (!rimaste[i]) continue;
            // o: oggi, dall'ora corrente fino a k (e non oltre un'eventuale buca); g: in un giorno intero dopo oggi
            const int o = ultima_oggi < ora ? 0 : MIN(cap_oggi[i], slot_tra_le_ore(&lez[i], ora, MIN(k, ultima_oggi)));
            const int g = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, slot_tra_le_ore(&lez[i], 0, k));
            oggi += MIN(rimaste[i], o);
            in_un_giorno += MIN(rimaste[i], g);
            in_tutto += MIN(rimaste[i], o + giorni_dopo * g);
        }
        // ...e comunque non piu' degli slot fino all'ora k
        const int slot_oggi = MIN(k, ultima_oggi) >= ora ? MIN(k, ultima_oggi) - ora + 1 : 0;
        const int max_lezioni = MIN(in_tutto, MIN(oggi, slot_oggi) + giorni_dopo * MIN(in_un_giorno, k + 1));
        // slot da riempire per forza: fino all'ultima ora tutte le ore della classe; fino a un'ora obbligatoria k,
        // (k + 1) slot per ogni giorno dopo oggi piu' quelli di oggi dall'ora corrente a k
        const int da_riempire = k == ultima
                                    ? classe_info[c].ore - classe_info[c].svolte
                                    : giorni_dopo * (k + 1) + (k >= ora ? k - ora + 1 : 0);
        if (max_lezioni < da_riempire) {
            *ora_ko = k;
            *servono = da_riempire;
            *possibili = max_lezioni;
            return 0;
        }
    }
    return 1;
}

// Verifica tutte le classi (vedi vincoli.h); si ferma alla prima che non va.
int lezioni_bastano_per_le_classi(const int giorno, const int ora, int *classe_ko, int *ora_ko, int *servono,
                                  int *possibili) {
    if (!lezioni_complete) return 1;
    for (int c = 0; c < actual_classi_sezione; c++) {
        if (!lezioni_della_classe_bastano(giorno, ora, c, ora_ko, servono, possibili)) {
            *classe_ko = c;
            return 0;
        }
    }
    return 1;
}

// Dopo lo slot (giorno, ora) della classe si verifica dall'ora dopo: quella successiva, o la prima del giorno dopo.
// Dopo l'ultimo slot della settimana non c'e' piu' nulla da verificare.
int lezioni_della_classe_bastano_dopo(const teaching_slot *slot) {
    if (!lezioni_complete) return 1;
    int giorno = slot->giorno, ora = slot->ora + 1;
    if (ora == actual_ore_giornaliere) {
        if (++giorno == actual_giorni_settimana) return 1;
        ora = 0;
    }
    int k, servono, possibili;
    return lezioni_della_classe_bastano(giorno, ora, slot->classe, &k, &servono, &possibili);
}

// Le due condizioni sui docenti (vedi sopra); la prima calcola in totale[] le ore rimaste di ogni docente per la seconda.
int ore_rimaste_stanno_nel_tempo_rimasto(const int giorno, const int ora) {
    int totale[INSEGNANTE_SZ] = {0};
    return ore_in_classe_stanno_nel_tempo_rimasto(giorno, ora, totale)
           && ore_totali_stanno_nel_tempo_rimasto(giorno, ora, totale);
}
