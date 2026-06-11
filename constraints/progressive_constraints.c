//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include <assert.h>

#include "../database_initializers.h"
#include "../professor_functions.h"

int g_giorno(const int p) { return p / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana; }

int g_ora(const int p) { return p / actual_classi_sezione % actual_ore_giornaliere; }

int g_classe(const int p) {
    return p % actual_classi_sezione;
}

int still_activities_for_today(const elem *orario, const int pos) {
    if (pos - actual_classi_sezione >= 0) {
        if (g_giorno(pos) == g_giorno(pos - actual_classi_sezione))
            return orario[pos - actual_classi_sezione].insegnante1 != -1
                   || orario[pos - actual_classi_sezione].insegnante2 != -1;
    }
    return 1;
}

int prof_in_classe(elem orario[], const long pos, int prof) {
    if (prof != -1) {
        for (int i = pos - pos % actual_classi_sezione; i < pos; i++) {
            if (orario[i].insegnante1 == prof || orario[i].insegnante2 == prof)
                return 0;
        }
    }
    return 1;
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

int is_teacher_eligible_for_class(elem orario[], const int pos, const int mat, const int prof) {
    if (prof != -1) {
        int sum = 0;
        for (int i = 0; i < pos; i++) {
            sum += orario[i].insegnante1 == prof && orario[i].materia == mat && g_classe(i) == g_classe(pos);
            sum += orario[i].insegnante2 == prof && orario[i].materia == mat && g_classe(i) == g_classe(pos);
        }
        return sum < ore_insegnanti_classe_materia[
                   g_classe(pos) * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof];
    }
    return 1;
    /*    return ore_insegnanti_classe_materia_curr[g_classe(pos) * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof] <
               ore_insegnanti_classe_materia[g_classe(pos) * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof];*/
}
