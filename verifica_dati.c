#include "verifica_dati.h"
#include "data_initialization/database_initializers.h"

#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static char *errore;
static size_t dimensione;

static int ingresso(const int p) {
    return ora_di_entrata_prof[p];
}

static int ore_in_classe(const int c, const int p) {
    return ore_classi_docenti[c * INSEGNANTE_SZ + p];
}

static int ore_in_tutto(const int p) {
    int ore = 0;
    for (int c = 0; c < actual_classi_sezione; c++) ore += ore_in_classe(c, p);
    return ore;
}

static const char *entrando(const int p) {
    static char testo[MAX_LEN_WORDS + 20];
    if (ingresso(p)) snprintf(testo, sizeof testo, "entrando alle %s ", ore_giornaliere[ingresso(p)]);
    else testo[0] = 0;
    return testo;
}

static int docente_in_una_classe(void) {
    const int G = actual_giorni_settimana;
    for (int p = 0; p < actual_insegnanti; p++) {
        const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, actual_ore_giornaliere - ingresso(p));
        for (int c = 0; c < actual_classi_sezione; c++) {
            if (ore_in_classe(c, p) <= G * al_giorno) continue;
            snprintf(errore, dimensione, "%s in %s: %d ore, ma %sne può fare al massimo %d (%d al giorno per %d giorni)",
                     insegnanti[p], classe_sezione[c], ore_in_classe(c, p), entrando(p), G * al_giorno, al_giorno, G);
            return 0;
        }
    }
    return 1;
}

static int docente_in_tutto(void) {
    const int G = actual_giorni_settimana;
    for (int p = 0; p < actual_insegnanti; p++) {
        const int al_giorno = actual_ore_giornaliere - ingresso(p);
        if (ore_in_tutto(p) <= G * al_giorno) continue;
        snprintf(errore, dimensione, "%s: %d ore in tutto, ma %sne può fare al massimo %d (%d al giorno per %d giorni)",
                 insegnanti[p], ore_in_tutto(p), entrando(p), G * al_giorno, al_giorno, G);
        return 0;
    }
    return 1;
}

static int docente_classi_al_giorno(void) {
    const int disponibili = MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO * actual_giorni_settimana;
    for (int p = 0; p < actual_insegnanti; p++) {
        int servono = 0;
        for (int c = 0; c < actual_classi_sezione; c++)
            servono += (ore_in_classe(c, p) + MAX_ORE_PROF_IN_UNA_CLASSE - 1) / MAX_ORE_PROF_IN_UNA_CLASSE;
        if (servono <= disponibili) continue;
        snprintf(errore, dimensione, "%s: con al massimo %d ore al giorno in una classe, le sue classi gli prendono in "
                 "tutto almeno %d giorni, ma con %d classi al giorno per %d giorni ne ha %d", insegnanti[p],
                 MAX_ORE_PROF_IN_UNA_CLASSE, servono, MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO,
                 actual_giorni_settimana, disponibili);
        return 0;
    }
    return 1;
}

int verifica_dati(char *testo, const size_t lunghezza) {
    errore = testo;
    dimensione = lunghezza;
    return docente_in_una_classe()
           && docente_in_tutto()
           && docente_classi_al_giorno();
}
