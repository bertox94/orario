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

static int lezioni_fino_a(const int ore, const int entra, const int h) {
    return MIN(ore, actual_giorni_settimana * MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, h + 1 - entra)));
}

static int classe_ore_obbligatorie(void) {
    const int G = actual_giorni_settimana;
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int h = 0; h < MIN(PRIME_N_ORE_OBBLIGATORIE, actual_ore_giornaliere); h++) {
            int possibili = 0;
            for (int p = 0; p < actual_insegnanti; p++) {
                if (!ore_in_classe(c, p)) continue;
                possibili += lezioni_fino_a(ore_classi_docenti_alone[c * INSEGNANTE_SZ + p], ingresso(p), h);
                for (int q = p + 1; q < actual_insegnanti; q++)
                    possibili += lezioni_fino_a(get_ore_insegnanti_codocenza_in_una_classe(c, p, q),
                                                MAX(ingresso(p), ingresso(q)), h);
            }
            if (possibili >= (h + 1) * G) continue;
            char quando[2 * MAX_LEN_WORDS + 20];
            if (h) snprintf(quando, sizeof quando, "dalle %s alle %s", ore_giornaliere[0], ore_giornaliere[h]);
            else snprintf(quando, sizeof quando, "alle %s", ore_giornaliere[0]);
            snprintf(errore, dimensione, "%s: %s servono %d lezioni in una settimana (%d al giorno), ma con le ore e "
                     "gli ingressi dei suoi docenti ce ne possono essere al massimo %d", classe_sezione[c], quando,
                     (h + 1) * G, h + 1, possibili);
            return 0;
        }
    }
    return 1;
}

static int docenti_entrati_a_ogni_ora(void) {
    for (int h = 0; h < MIN(PRIME_N_ORE_OBBLIGATORIE, actual_ore_giornaliere); h++) {
        int entrati = 0;
        for (int p = 0; p < actual_insegnanti; p++) entrati += ingresso(p) <= h && ore_in_tutto(p) > 0;
        if (entrati >= actual_classi_sezione) continue;
        snprintf(errore, dimensione, "alle %s sono entrati solo %d docenti, ma le classi sono %d e l'ora è obbligatoria",
                 ore_giornaliere[h], entrati, actual_classi_sezione);
        return 0;
    }
    return 1;
}

int verifica_dati(char *testo, const size_t lunghezza) {
    errore = testo;
    dimensione = lunghezza;
    return docente_in_una_classe()
           && docente_in_tutto()
           && docente_classi_al_giorno()
           && classe_ore_obbligatorie()
           && docenti_entrati_a_ogni_ora();
}
