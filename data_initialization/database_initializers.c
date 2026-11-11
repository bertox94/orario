//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include "../data_initialization/database_initializers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Scuola *nuova_scuola(void) {
    Scuola *s = calloc(1, sizeof(Scuola));
    assert(s);
    return s;
}

void libera_scuola(Scuola *s) {
    free(s);
}

void swap_min_first(int *p1, int *p2) {
    if (*p1 > *p2) {
        const int temp = *p1;
        *p1 = *p2;
        *p2 = temp;
    }
}

// ---- i nomi ----

// l'indice di un nome in un elenco di n nomi (-1 se non c'e')
static int indice(char nomi[][MAX_LEN_WORDS], const int n, const char nome[]) {
    for (int i = 0; i < n; i++)
        if (!strcmp(nomi[i], nome))
            return i;
    return -1;
}

// Registra un nome: lo aggiunge in fondo all'elenco se non c'e' ancora (l'ordine di registrazione da' gli indici).
// Ritorna il suo indice.
static int registra(char nomi[][MAX_LEN_WORDS], int *n, const int massimo, const char nome[]) {
    const int i = indice(nomi, *n, nome);
    if (i >= 0) return i;
    assert(*n < massimo);
    assert(strlen(nome) < MAX_LEN_WORDS);
    strcpy(nomi[*n], nome);
    return (*n)++;
}

int p_insegnante(const Scuola *s, const char in[]) {
    return indice((char (*)[MAX_LEN_WORDS]) s->insegnanti, s->actual_insegnanti, in);
}

void set_giorno_settimana(Scuola *s, const char giorno[]) {
    registra(s->giorni_settimana, &s->actual_giorni_settimana, GIORNO_SETTIMANA_SZ, giorno);
}

void set_ora_giornaliera(Scuola *s, const char ora[]) {
    registra(s->ore_giornaliere, &s->actual_ore_giornaliere, ORA_GIORNALIERA_SZ, ora);
}

static int set_classe_sezione(Scuola *s, const char cla[]) {
    return registra(s->classe_sezione, &s->actual_classi_sezione, CLASSE_SEZIONE_SZ, cla);
}

static int set_insegnante(Scuola *s, const char in[]) {
    return registra(s->insegnanti, &s->actual_insegnanti, INSEGNANTE_SZ, in);
}

// ---- le ore in coppia ----

// dove stanno le ore della coppia p1, p2 nella classe
static int *ore_codocenza(const Scuola *s, const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    return (int *) &s->ore_classi_codocenza[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2];
}

int get_ore_insegnanti_codocenza_in_una_classe(const Scuola *s, const int cla, const int p1, const int p2) {
    return *ore_codocenza(s, cla, p1, p2);
}

// ---- le lezioni ----

void set_classe_insegnante_ore(Scuola *s, const char cla[], const char prof[], const int ore) {
    set_classe_insegnante_ore_cod(s, cla, prof, ore, NULL, 0);
}

// Con cod == NULL la lezione e' tutta da solo. Classe e docenti si registrano in quest'ordine: classe, docente,
// codocente (l'ordine decide gli indici, e a parita' di ore l'ordine in cui la ricerca prova i docenti).
void set_classe_insegnante_ore_cod(Scuola *s, const char cla[], const char prof[], const int ore, const char cod[],
                                   const int ore_cod) {
    const int c = set_classe_sezione(s, cla), p = set_insegnante(s, prof);
    s->ore_classi_docenti[c * INSEGNANTE_SZ + p] += ore;
    s->ore_classi_docenti_alone[c * INSEGNANTE_SZ + p] += ore - ore_cod;
    s->ore_classi[c] += ore;
    if (!cod) return;
    const int q = set_insegnante(s, cod);
    s->ore_classi_docenti[c * INSEGNANTE_SZ + q] += ore_cod;
    *ore_codocenza(s, c, p, q) += ore_cod;
}
