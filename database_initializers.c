//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include "database_initializers.h"
#include "find_positions.h"
#include "days.h"
#include "hours.h"
#include "rooms.h"
#include "professors.h"
#include "classes.h"
#include "subjects.h"
#include <assert.h>
#include <string.h>

#include "associations.h"

void set_giorno_settimana(char giorno[], const int pos) {
    assert(pos < GIORNO_SETTIMANA_SZ);
    assert(strlen(giorno) < GIORNO_SETTIMANA_ITEM_SZ);
    strcpy(giorni_settimana[pos], giorno);
}

void set_ora_giornaliera(char ora[], const int pos) {
    assert(pos < ORA_GIORNALIERA_SZ);
    assert(strlen(ora) < ORA_GIORNALIERA_ITEM_SZ);
    strcpy(ora_giornaliera[pos], ora);
}

void set_classe_sezione(char cla[], const int pos) {
    assert(pos < CLASSE_SEZIONE_SZ);
    assert(strlen(cla) < CLASSE_SEZIONE_ITEM_SZ);
    strcpy(classe_sezione[pos], cla);
}

void set_aula(char aul[], const int pos) {
    assert(pos < AULA_SZ);
    assert(strlen(aul) < AULA_ITEM_SZ);
    strcpy(aule[pos], aul);
}

void set_materia(char mat[], const int pos) {
    assert(pos < MATERIA_SZ);
    assert(strlen(mat) < MATERIA_ITEM_SZ);
    strcpy(materie[pos], mat);
}

void set_insegnante(char in[], const int pos) {
    assert(pos < INSEGNANTE_SZ);
    assert(strlen(in) < INSEGNANTE_ITEM_SZ);
    strcpy(insegnanti[pos], in);
}

void set_laboratorio(char lab[]) {
    laboratori[p_aula(lab)] = 1;
}

void set_codocente(char cod[]) {
    codocenti[p_insegnante(cod)] = 1;
}

void inizializza_dati() {
    int pos = 0;
    set_giorno_settimana(Lun, pos++);
    set_giorno_settimana(Mar, pos++);
    for (int i = pos; i < GIORNO_SETTIMANA_SZ; i++)
        set_giorno_settimana("", i);
    actual_giorni_settimana = pos;

    pos = 0;
    set_ora_giornaliera(Ora_08, pos++);
    set_ora_giornaliera(Ora_09, pos++);
    set_ora_giornaliera(Ora_10, pos++);
    set_ora_giornaliera(Ora_11, pos++);
    set_ora_giornaliera(Ora_12, pos++);
    set_ora_giornaliera(Ora_13, pos++);
    for (int i = pos; i < ORA_GIORNALIERA_SZ; i++)
        set_ora_giornaliera("", i);
    actual_ore_giornaliere = pos;

    pos = 0;
    set_classe_sezione(Classe_3H, pos++);
    //set_classe_sezione(Classe_3E, pos++);
    //set_classe_sezione("3F", pos++);
    for (int i = pos; i < CLASSE_SEZIONE_SZ; i++)
        set_classe_sezione("", i);
    actual_classi_sezione = pos;

    pos = 0;
    set_aula(PS16, pos++);
    set_aula(INFO1, pos++);
    //set_aula("INFO0", pos++);
    for (int i = pos; i < AULA_SZ; i++)
        set_aula("", i);
    actual_aula = pos;

    pos = 0;
    set_materia(Informatica, pos++);
    set_materia(Sistemi_e_Reti, pos++);
    for (int i = pos; i < MATERIA_SZ; i++)
        set_materia("", i);
    actual_materie = pos;

    pos = 0;
    set_insegnante(Ricciardi, pos++);
    set_insegnante(Tiraboschi, pos++);
    set_insegnante(Gatti, pos++);
    set_insegnante(Barosi, pos++);
    set_insegnante(Rizzi, pos++);
    for (int i = pos; i < INSEGNANTE_SZ; i++)
        set_insegnante("", i);
    actual_insegnanti = pos;
}

void inizializza_vincoli() {
    memset(laboratori, 0, AULA_SZ*sizeof(int));
    //set_laboratorio("INFO0");
    set_laboratorio(INFO1);

    memset(codocenti, 0, INSEGNANTE_SZ*sizeof(int));
    set_codocente(Rizzi);
    set_codocente(Barosi);

/*
    memset(ore_classe_materia, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*sizeof(int));
    memset(ore_insegnanti_classe_materia, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ*sizeof(int));
    set_classe_materia_ore_prof_codocenza(Classe_3H, Sistemi_e_Reti, 2, Ricciardi, 1, Barosi);
    set_classe_materia_ore_prof_codocenza(Classe_3H, Informatica, 1, Gatti, 1, Barosi);
    //set_classe_materia_ore_prof(Classe_3E, Sistemi_e_Reti, 2, Ricciardi, Rizzi);
    //set_classe_materia_ore_prof(Classe_3E, Informatica, 1, Gatti, Rizzi);
    */

    memset(ore_settimana_prof, 0, INSEGNANTE_SZ*sizeof(int));
    ore_settimana_prof[p_insegnante(Ricciardi)] = 2;
    ore_settimana_prof[p_insegnante(Gatti)] = 1;
    //ore_settimana_prof[p_insegnante(Tiraboschi)] = 2;
    ore_settimana_prof[p_insegnante(Barosi)] = 2;
}
