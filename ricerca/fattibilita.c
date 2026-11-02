#include "fattibilita.h"

#include <stdio.h>

#include "../data_initialization/database_initializers.h"
#include "vincoli.h"

// Ogni classe deve avere una lezione in ognuna delle ore obbligatorie di ogni giorno, e tutte le sue ore devono stare
// nella settimana. Fino all'ora k una lezione (docente da solo o coppia) cade solo dopo il suo ingresso, e in una classe
// al massimo MAX_ORE_PROF_IN_UNA_CLASSE volte al giorno (vedi lezioni_bastano_per_le_classi).
static int classi_hanno_lezioni_a_sufficienza(char *motivo, const size_t dim) {
    int c, k, servono, possibili;
    if (lezioni_bastano_per_le_classi(0, 0, &c, &k, &servono, &possibili)) return 1;
    if (k < PRIME_N_ORE_OBBLIGATORIE)
        snprintf(motivo, dim, "classe %s: fino all'ora %s servono %d lezioni ma, con le ore di ingresso dei docenti e "
                              "al massimo %d ore al giorno per docente nella classe, ne possono cadere al massimo %d",
                 classe_sezione[c], ore_giornaliere[k], servono, MAX_ORE_PROF_IN_UNA_CLASSE, possibili);
    else
        snprintf(motivo, dim, "classe %s: %d ore ma, con le ore di ingresso dei docenti e al massimo %d ore al giorno "
                              "per docente nella classe, ne possono stare nella settimana al massimo %d",
                 classe_sezione[c], servono, MAX_ORE_PROF_IN_UNA_CLASSE, possibili);
    return 0;
}

// A ogni ora obbligatoria tutte le classi fanno lezione insieme, con docenti diversi: servono almeno tanti docenti
// quante sono le classi, gia' entrati a quell'ora.
static int docenti_bastano_per_le_classi(char *motivo, const size_t dim) {
    for (int k = 0; k < PRIME_N_ORE_OBBLIGATORIE && k < actual_ore_giornaliere; k++) {
        int entrati = 0;
        for (int p = 0; p < actual_insegnanti; p++)
            if (docente_info[p].ore > 0 && docente_info[p].ora_entrata <= k) entrati++;
        if (entrati < actual_classi_sezione) {
            snprintf(motivo, dim, "all'ora %s fanno lezione %d classi insieme ma i docenti gia' entrati sono %d",
                     ore_giornaliere[k], actual_classi_sezione, entrati);
            return 0;
        }
    }
    return 1;
}

// Fino all'ora k tutte le classi hanno (k + 1) * giorni slot da riempire, e ogni slot ha almeno un docente. Un docente puo'
// dare al massimo una lezione per ora, dalla sua ora di ingresso, e comunque non piu' delle sue ore settimanali.
static int ore_dei_docenti_bastano_per_gli_slot(char *motivo, const size_t dim) {
    for (int k = 0; k < PRIME_N_ORE_OBBLIGATORIE && k < actual_ore_giornaliere; k++) {
        const int servono = (k + 1) * actual_giorni_settimana * actual_classi_sezione;
        int possono = 0;
        for (int p = 0; p < actual_insegnanti; p++) {
            int capienza = (k - docente_info[p].ora_entrata + 1) * actual_giorni_settimana;
            if (capienza < 0) capienza = 0;
            possono += capienza < docente_info[p].ore ? capienza : docente_info[p].ore;
        }
        if (possono < servono) {
            snprintf(motivo, dim, "fino all'ora %s servono %d lezioni (tutte le classi) ma i docenti possono darne al "
                                  "massimo %d", ore_giornaliere[k], servono, possono);
            return 0;
        }
    }
    return 1;
}

// Le ore settimanali di un docente devono stare nelle ore che gli restano, dalla sua ora di ingresso.
static int ore_dei_docenti_stanno_nella_settimana(char *motivo, const size_t dim) {
    for (int p = 0; p < actual_insegnanti; p++) {
        const int disponibili = (actual_ore_giornaliere - docente_info[p].ora_entrata) * actual_giorni_settimana;
        if (docente_info[p].ore > disponibili) {
            snprintf(motivo, dim, "docente %s: %d ore a settimana ma entrando all'ora %s ne ha al massimo %d",
                     insegnanti[p], docente_info[p].ore, ore_giornaliere[docente_info[p].ora_entrata], disponibili);
            return 0;
        }
    }
    return 1;
}

int verifica_fattibilita(char *motivo, const size_t dimensione) {
    return ore_dei_docenti_stanno_nella_settimana(motivo, dimensione)
           && docenti_bastano_per_le_classi(motivo, dimensione)
           && ore_dei_docenti_bastano_per_gli_slot(motivo, dimensione)
           && classi_hanno_lezioni_a_sufficienza(motivo, dimensione);
}
