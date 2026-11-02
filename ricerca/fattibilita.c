#include "fattibilita.h"

#include <stdio.h>

#include "../data_initialization/database_initializers.h"

// Quante lezioni (ore di una classe) possono essere gia' cominciate all'ora k: quelle da solo di un docente entrato
// entro k e quelle in coppia con entrambi i docenti entrati entro k.
static int lezioni_possibili_entro_ora(const int classe, const int k) {
    int n = 0;
    for (int p = 0; p < actual_insegnanti; p++) {
        if (docente_info[p].ora_entrata <= k) n += classe_docente[classe * INSEGNANTE_SZ + p].alone;
        for (int q = p + 1; q < actual_insegnanti; q++) {
            const int ore = codocenza[classe * INSEGNANTE_SZ * INSEGNANTE_SZ + p * INSEGNANTE_SZ + q].ore;
            const int ingresso = docente_info[p].ora_entrata > docente_info[q].ora_entrata
                                     ? docente_info[p].ora_entrata
                                     : docente_info[q].ora_entrata;
            if (ore > 0 && ingresso <= k) n += ore;
        }
    }
    return n;
}

// Ogni classe deve avere una lezione in ognuna delle ore obbligatorie di ogni giorno: alla fine dell'ora k gli slot da
// riempire sono (k + 1) * giorni, e si riempiono solo con lezioni di docenti gia' entrati.
static int classi_hanno_lezioni_a_sufficienza(char *motivo, const size_t dim) {
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int k = 0; k < PRIME_N_ORE_OBBLIGATORIE && k < actual_ore_giornaliere; k++) {
            const int servono = (k + 1) * actual_giorni_settimana;
            const int ci_sono = lezioni_possibili_entro_ora(c, k);
            if (ci_sono < servono) {
                snprintf(motivo, dim, "classe %s: fino all'ora %s servono %d lezioni ma solo %d sono di docenti "
                                      "(o coppie) gia' entrati", classe_sezione[c], ore_giornaliere[k], servono, ci_sono);
                return 0;
            }
        }
    }
    return 1;
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
