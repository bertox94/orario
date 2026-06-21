//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include "../data_initialization/database_initializers.h"
#include "../constants/days.h"
#include "../constants/hours.h"
#include "../constants/professors.h"
#include "../constants/classes.h"
#include "../constants/subjects.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

// automatically initialized to zero since they are global
int actual_giorni_settimana;
int actual_ore_giornaliere;
int actual_classi_sezione;
int actual_insegnanti;

int ore_classi_docenti[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_docenti_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi[CLASSE_SEZIONE_SZ];
int ore_classe_svolte[CLASSE_SEZIONE_SZ];
int ore_docenti[INSEGNANTE_SZ];
int ore_docenti_svolte[INSEGNANTE_SZ];

int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];
int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];
int ore_prof_giorno_classe[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

/*
 * Conversions from string to id
 */
int p_giorno(char g[]) {
    for (int i = 0; i < actual_giorni_settimana; i++)
        if (!strcmp(giorni_settimana[i], g))
            return i;
    return -1;
}

int p_ora(char o[]) {
    for (int i = 0; i < actual_ore_giornaliere; i++)
        if (!strcmp(ore_giornaliere[i], o))
            return i;
    return -1;
}

int p_classe_sezione(char cla[]) {
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

/*
 * Append string to its array and increment respective size.
 */

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

void set_classe_sezione(char cla[]) {
    if (p_classe_sezione(cla) >= 0) return;
    assert(actual_classi_sezione < CLASSE_SEZIONE_SZ);
    assert(strlen(cla) < MAX_LEN_WORDS);
    strcpy(classe_sezione[actual_classi_sezione++], cla);
}

void set_insegnante(char in[]) {
    if (p_insegnante(in) >= 0) return;
    assert(actual_insegnanti < INSEGNANTE_SZ);
    assert(strlen(in) < MAX_LEN_WORDS);
    strcpy(insegnanti[actual_insegnanti++], in);
}

/*
 * Access and set constraints
 */
void add_ore_classi_insegnante(int classe, int insegnante, int ore) {
    ore_classi_docenti[classe * INSEGNANTE_SZ + insegnante] += ore;
}

void add_ore_classi_settimanale(int classe, int ore) {
    ore_classi[classe] += ore;
}

void add_ore_insegnante_settimanale(int insegnante, int ore) {
    ore_docenti[insegnante] += ore;
}

int get_ore_classi_insegnante(int classe, int insegnante) {
    return ore_classi_docenti[classe * INSEGNANTE_SZ + insegnante];
}

int get_ore_classi_settimanale(int classe) {
    return ore_classi[classe];
}

int get_ore_insegnante_settimanale(int insegnante) {
    return ore_docenti[insegnante];
}

/*
 * Set constraints from user
 */
void set_classe_insegnante_ore(char cla[], char prof[], const int ore) {
    set_classe_sezione(cla);
    set_insegnante(prof);
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);
    add_ore_insegnante_settimanale(p_insegnante(prof), ore);
}

/*
void set_classe_materia_ore_prof_codocenza(char cla[], char mat[], const int ore, const char prof[], const int ore_cod,
                                           const char cod[]) {
    set_classe_sezione(cla);
    set_materia(mat);
    set_insegnante(prof);
    set_insegnante(cod);
    const int p_cla = p_classe_sezione(cla);
    const int p_mat = p_materia(mat);
    const int p_prof = p_insegnante(prof);
    const int p_cod = p_insegnante(cod);
    assert(p_cla!=-1);
    assert(p_mat!=-1);
    assert(p_prof!=-1);
    assert(p_cod!=-1);
    const int base = p_cla * MATERIA_SZ * INSEGNANTE_SZ + p_mat * INSEGNANTE_SZ;
    ore_insegnanti_classe_materia[base + p_prof] += ore;
    ore_insegnanti_classe_materia[base + p_cod] += ore_cod;

    assert(ore>=ore_cod);
    ore_insegnanti_classe_materia_lonely[base + p_prof] += ore - ore_cod;
    ore_classe_settimanali[p_cla] += ore;
}
*/

void inizializza_dati() {
    set_giorno_settimana(Lun);
    set_giorno_settimana(Mar);
    set_giorno_settimana(Mer);
    set_giorno_settimana(Gio);
    set_giorno_settimana(Ven);
    set_giorno_settimana(Sab);

    set_ora_giornaliera(Ora_08);
    set_ora_giornaliera(Ora_09);
    set_ora_giornaliera(Ora_10);
    set_ora_giornaliera(Ora_11);
    set_ora_giornaliera(Ora_12);
    set_ora_giornaliera(Ora_13);


    set_classe_insegnante_ore(Classe_3E, Berno, 4);
    set_classe_insegnante_ore(Classe_3E, DalZovo, 6);
    set_classe_insegnante_ore(Classe_3E, Sciambarella, 3);
    set_classe_insegnante_ore(Classe_3E, Masetti, 6);
    set_classe_insegnante_ore(Classe_3E, Nicoletti, 2);
    set_classe_insegnante_ore(Classe_3E, Trivini, 3);
    set_classe_insegnante_ore(Classe_3E, Quaini, 1);
    set_classe_insegnante_ore(Classe_3E, Tiraboschi, 4);
    set_classe_insegnante_ore(Classe_3E, Alves, 3);

    set_classe_insegnante_ore(Classe_4E, Berno, 4);
    set_classe_insegnante_ore(Classe_4E, Pizzatti, 6);
    set_classe_insegnante_ore(Classe_4E, Sciambarella, 7);
    set_classe_insegnante_ore(Classe_4E, Masetti, 6);
    set_classe_insegnante_ore(Classe_4E, Torbol, 2);
    set_classe_insegnante_ore(Classe_4E, Trivini, 3);
    set_classe_insegnante_ore(Classe_4E, Quaini, 1);
    set_classe_insegnante_ore(Classe_4E, Alves, 3);

    set_classe_insegnante_ore(Classe_3F, Sannino, 4);
    set_classe_insegnante_ore(Classe_3F, Bortolotti, 6);
    set_classe_insegnante_ore(Classe_3F, Ricciardi, 3);
    set_classe_insegnante_ore(Classe_3F, Kushi, 6);
    set_classe_insegnante_ore(Classe_3F, Nicoletti, 2);
    set_classe_insegnante_ore(Classe_3F, Trivini, 3);
    set_classe_insegnante_ore(Classe_3F, Quaini, 1);
    set_classe_insegnante_ore(Classe_3F, Tiraboschi, 4);
    set_classe_insegnante_ore(Classe_3F, Comelli, 3);

    set_classe_insegnante_ore(Classe_3H, Tosi, 4);
    set_classe_insegnante_ore(Classe_3H, Guerra, 6);
    set_classe_insegnante_ore(Classe_3H, Manganelli, 3);
    set_classe_insegnante_ore(Classe_3H, Tiraboschi, 6);
    set_classe_insegnante_ore(Classe_3H, Nicoletti, 2);
    set_classe_insegnante_ore(Classe_3H, Trivini, 3);
    set_classe_insegnante_ore(Classe_3H, Quaini, 1);
    set_classe_insegnante_ore(Classe_3H, Tiraboschi, 4);
    set_classe_insegnante_ore(Classe_3H, Lancellotti, 3);


    for (int i = 0; i < actual_classi_sezione; i++) {
        assert(ore_classi[i] <= actual_giorni_settimana * actual_ore_giornaliere);
        assert(ore_classi[i] >= PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
    }
}

void stampa_dati() {
    printf("Giorni settimana: ");
    for (int i = 0; i < actual_giorni_settimana - 1; i++)
        printf("%s, ", giorni_settimana[i]);
    if (actual_giorni_settimana - 1 > 0)
        printf("%s\n", giorni_settimana[actual_giorni_settimana - 1]);
    printf("Ore giornaliere: ");
    for (int i = 0; i < actual_ore_giornaliere - 1; i++)
        printf("%s, ", ore_giornaliere[i]);
    if (actual_ore_giornaliere - 1 > 0)
        printf("%s\n", ore_giornaliere[actual_ore_giornaliere - 1]);
    printf("Ore classe:\n");
    for (int i = 0; i < actual_classi_sezione; i++)
        printf("Classe %s: %d ore\n", classe_sezione[i], ore_classi[i]);
    printf("-----------\n");
    printf("Ore insegnanti:\n");
    for (int i = 0; i < actual_insegnanti; i++)
        printf("Ins. %s: %d ore\n", insegnanti[i], ore_docenti[i]);
    printf("-----------\n");
    printf("Ore insegnanti in classe:\n");
    for (int i = 0; i < actual_classi_sezione; i++)
        for (int j = 0; j < actual_insegnanti; j++)
            if (get_ore_classi_insegnante(i, j) > 0)
                printf("In %s %s\tha %d ore\n", classe_sezione[i], insegnanti[j], get_ore_classi_insegnante(i, j));
    printf("-----------\n");
}
