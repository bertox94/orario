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
    const int d1 = is_docente(prof1), c1 = is_codocente(prof1);
    const int d2 = is_docente(prof2), c2 = is_codocente(prof2);

    // Returns 1 (true) if the pair is allowed, 0 (false) if it's a conflict
    return (!(d1 & d2)) & ((c1 + c2) <= (d1 + d2));
}

int verify_lonely_teacher_hours(elem orario[], const int pos, const int class, const int mat, const int prof) {
    int sum = 0;
    int linear3d = class * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof;
    for (int i = class; i < pos; i += actual_classi_sezione) {
        if (orario[i].materia == mat)
            sum += orario[i].insegnante1 == prof || orario[i].insegnante2 == prof;
        if (sum >= ore_insegnanti_classe_materia_lonely[linear3d]) return 0;
    }
    if (sum >= ore_insegnanti_classe_materia_lonely[linear3d])
        printf("sum: %d\n", sum);
    return 1;
}

int verify_teacher_hours(elem orario[], const int pos, const int class, const int mat, const int prof1,
                         const int prof2) {
    int linear3dbase = class * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ;

    int sum1 = 0;
    for (int i = class; i < pos; i += actual_classi_sezione) {
        if (orario[i].materia == mat)
            sum1 += orario[i].insegnante1 == prof2 || orario[i].insegnante2 == prof2;
    }
    if (sum1 >= ore_insegnanti_classe_materia[linear3dbase + prof2])
        return 0;

    //c'è il codocente
    int sum2 = 0;
    for (int i = class; i < pos; i += actual_classi_sezione) {
        if (orario[i].materia == mat)
            sum2 += orario[i].insegnante1 == prof1 || orario[i].insegnante2 == prof1;
    }
    if (sum2 >= ore_insegnanti_classe_materia[linear3dbase + prof1])
        return 0;


    return 1;
}
