//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include <assert.h>
#include <stdio.h>

#include "../data_initialization/database_initializers.h"

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

int prof_in_classe(const elem orario[], const long pos, const int prof) {
    // 1. Compute range exactly once
    const int start_index = pos - (pos % actual_classi_sezione);
    const elem *const base = &orario[0];

    for (int i = start_index; i < pos; ++i) {
        if (base[i].insegnante1 == prof | base[i].insegnante2 == prof) {
            return 0; // Collision found
        }
    }

    return 1; // Both professors are free!
}

int controllo_prof_insieme(const int prof1, const int prof2) {
    return !(is_codocente(prof1) && is_codocente(prof2)) && !(is_docente(prof1) && is_docente(prof2));
}

int verify_teacher_hours(elem orario[], const int pos, const int class, const int mat, const int prof1,
                         const int prof2) {
    int linear3dbase = class * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ;

    // 1. Cache limits in local variables to avoid array lookups and enable early exit
    int max1 = ore_insegnanti_classe_materia[linear3dbase + prof2];
    int max2 = ore_insegnanti_classe_materia[linear3dbase + prof1];

    // Quick check: if the allowed hours are already 0, we can skip the loop entirely
    if (max1 <= 0 || max2 <= 0) {
        return 0;
    }

    int sum1 = 0;
    int sum2 = 0;

    // 2. Merged loop: processes both teachers in a single pass over the array
    for (int i = class; i < pos; i += actual_classi_sezione) {
        // Cache array element to minimize pointer arithmetic and maximize CPU cache usage
        const elem slot = orario[i];

        if (slot.materia == mat) {
            sum1 += (slot.insegnante1 == prof2 || slot.insegnante2 == prof2);
            sum2 += (slot.insegnante1 == prof1 || slot.insegnante2 == prof1);

            // 3. Early Exit: Stop the loop the exact moment a threshold is breached
            if (sum1 >= max1 || sum2 >= max2) {
                return 0;
            }
        }
    }

    return 1;
}
