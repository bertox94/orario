//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

// quanti giorni, ore, classi e docenti ci sono davvero (gli array sono grandi per il massimo)
int actual_giorni_settimana;
int actual_ore_giornaliere;
int actual_classi_sezione;
int actual_insegnanti;

int ore_classi_docenti[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_docenti_alone[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_docenti_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_docenti_alone_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi[CLASSE_SEZIONE_SZ];
int ore_classe_svolte[CLASSE_SEZIONE_SZ];
int ore_classi_codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
int ore_classi_codocenza_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

int ora_di_entrata_prof[INSEGNANTE_SZ];
int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];
int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];
// una coppia di docenti sempre nello stesso ordine: prima l'indice minore
static void swap_min_first(int *p1, int *p2) {
    if (*p1 > *p2) {
        const int temp = *p1;
        *p1 = *p2;
        *p2 = temp;
    }
}

// ---- dal nome all'indice (-1 se non c'e') ----
static int p_giorno(char g[]) {
    for (int i = 0; i < actual_giorni_settimana; i++)
        if (!strcmp(giorni_settimana[i], g))
            return i;
    return -1;
}

static int p_ora(char o[]) {
    for (int i = 0; i < actual_ore_giornaliere; i++)
        if (!strcmp(ore_giornaliere[i], o))
            return i;
    return -1;
}

static int p_classe_sezione(char cla[]) {
    for (int i = 0; i < actual_classi_sezione; i++)
        if (!strcmp(classe_sezione[i], cla))
            return i;
    return -1;
}

int p_insegnante(const char in[]) {
    for (int i = 0; i < actual_insegnanti; i++)
        if (!strcmp(insegnanti[i], in))
            return i;
    return -1;
}

// ---- registrare un nome: lo aggiunge in fondo se non c'e' ancora (l'ordine di registrazione da' gli indici) ----

void set_giorno_settimana(char giorno[]) {
    if (p_giorno(giorno) >= 0) return;
    assert(actual_giorni_settimana < GIORNO_SETTIMANA_SZ);
    assert(strlen(giorno) < MAX_LEN_WORDS);
    strcpy(giorni_settimana[actual_giorni_settimana++], giorno);
}

void set_ora_giornaliera(char ora[]) {
    if (p_ora(ora) >= 0) return;
    assert(actual_ore_giornaliere < ORA_GIORNALIERA_SZ);
    assert(strlen(ora) < MAX_LEN_WORDS);
    strcpy(ore_giornaliere[actual_ore_giornaliere++], ora);
}

static void set_classe_sezione(char cla[]) {
    if (p_classe_sezione(cla) >= 0) return;
    assert(actual_classi_sezione < CLASSE_SEZIONE_SZ);
    assert(strlen(cla) < MAX_LEN_WORDS);
    strcpy(classe_sezione[actual_classi_sezione++], cla);
}

static void set_insegnante(char in[]) {
    if (p_insegnante(in) >= 0) return;
    assert(actual_insegnanti < INSEGNANTE_SZ);
    assert(strlen(in) < MAX_LEN_WORDS);
    strcpy(insegnanti[actual_insegnanti++], in);
}

// ---- le ore ----
static void add_ore_classi_insegnante(int classe, int insegnante, int ore) {
    ore_classi_docenti[classe * INSEGNANTE_SZ + insegnante] += ore;
}

static void add_ore_classi_insegnante_lonely(int classe, int insegnante, int ore) {
    ore_classi_docenti_alone[classe * INSEGNANTE_SZ + insegnante] += ore;
}

static void add_ore_classi_settimanale(int classe, int ore) {
    ore_classi[classe] += ore;
}

static void add_ore_classi_codocenza(int cla, int p1, int p2, int ore) {
    swap_min_first(&p1, &p2);
    ore_classi_codocenza[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2] += ore;
}

void increment_ore_classi_codocenza_svolte(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    ore_classi_codocenza_svolte[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2]++;
}

void decrement_ore_classi_codocenza_svolte(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    ore_classi_codocenza_svolte[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2]--;
}

int get_ore_insegnanti_codocenza_svolte_in_una_classe(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    return ore_classi_codocenza_svolte[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2];
}

int get_ore_insegnanti_codocenza_in_una_classe(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    return ore_classi_codocenza[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2];
}

// ---- le lezioni ----
// Una lezione: il docente prof fa `ore` ore nella classe cla, tutte da solo. Registra classe e docente e lo mette in
// coda (la sua priorita' cresce delle ore).

void set_classe_insegnante_ore(char cla[], char prof[], const int ore) {
    set_classe_sezione(cla);
    set_insegnante(prof);
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_insegnante_lonely(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);
    add_to_queue(p_insegnante(prof), ore);
}

// Una lezione con codocente: prof fa `ore` ore nella classe, di cui ore_cod insieme a cod (e ore - ore_cod da solo).
void set_classe_insegnante_ore_cod(char cla[], char prof[], const int ore, char cod[], const int ore_cod) {
    set_classe_sezione(cla);
    set_insegnante(prof);
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_insegnante_lonely(p_classe_sezione(cla), p_insegnante(prof), ore - ore_cod);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);

    set_insegnante(cod);
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(cod), ore_cod);
    add_ore_classi_codocenza(p_classe_sezione(cla), p_insegnante(prof), p_insegnante(cod), ore_cod);
    add_to_queue(p_insegnante(cod), ore_cod);
    add_to_queue(p_insegnante(prof), ore);
}
