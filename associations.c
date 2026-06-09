#include "database_initializers.h"
#include "find_positions.h"
//
// Created by Alberto Tiraboschi on 09/06/2026.
//
/*
void set_classe_materia_ore_prof(char cla[], char mat[], const int ore, const char prof[]) {
    ore_classe_materia[p_classe_sezione(cla) * MATERIA_SZ + p_materia(mat)] = ore;
    ore_insegnanti_classe_materia[p_classe_sezione(cla) * MATERIA_SZ * INSEGNANTE_SZ + p_materia(mat) *
                                  INSEGNANTE_SZ + p_insegnante(prof)] = ore;
}

void set_classe_materia_ore_prof_codocenza(char cla[], char mat[], const int ore, const char prof[], const int ore_cod,
                                           const char cod[]) {
    set_classe_materia_ore_prof(cla, mat, ore, prof);
    set_classe_materia_ore_prof(cla, mat, ore_cod, cod);
}
*/