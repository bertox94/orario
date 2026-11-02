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

int ore_rimaste_stanno_nel_tempo_rimasto(const int giorno, const int ora) {
    int totale[INSEGNANTE_SZ] = {0};
    return ore_in_classe_stanno_nel_tempo_rimasto(giorno, ora, totale)
           && ore_totali_stanno_nel_tempo_rimasto(giorno, ora, totale);
}
