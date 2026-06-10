//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include <assert.h>

#include "database_initializers.h"
#include "professor_functions.h"

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

int prof_in_classe(const int prof1, const int prof2, const int prof_in_classe[]) {
    assert(prof2 != -1);
    return (is_vuoto(prof1) || !prof_in_classe[prof1])
           && !prof_in_classe[prof2];
}

int insegnanti_ore_materia_adeguata(const int cla, const int mat, const int prof1, const int prof2) {
    assert(prof2 != -1);
    return (prof1 == -1 || ore_insegnanti_classe_materia[
                cla * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof1]) > 0
           && ore_insegnanti_classe_materia[cla * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof2] > 0;
}


int codocente_deve_essere_in_lab_no_aula(const int aula, int prof1, int prof2) {
    assert(prof2 != -1);
    if (is_codocente(prof1) || is_codocente(prof2))
        return laboratori[aula];
    return 1;
}

int controllo_prof_insieme(const int prof1, const int prof2) {
    int n_docenti = is_docente(prof1) + is_docente(prof2);
    int n_codocenti = is_codocente(prof1) + is_codocente(prof2);
    // docente e codocente
    return (n_docenti == 1 && n_codocenti == 1)
           // docente solo
           || (n_docenti == 1 && n_codocenti == 0)
           //ora buca
           || (n_docenti == 0 && n_codocenti == 0);
}

int is_teacher_eligible_for_class(const int pos, const int *ore_insegnanti_classe_materia_curr, const int mat, const int prof) {
    return ore_insegnanti_classe_materia_curr[g_classe(pos) * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof] <
           ore_insegnanti_classe_materia[g_classe(pos) * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof];
}
