//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include <assert.h>

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

int prof_in_classe(elem orario[], const long pos, int prof1, int prof2) {
    //assert(!is_vuoto(prof1) || !is_vuoto(prof2));
    // 2. Compute range exactly once
    const int start_index = pos - (pos % actual_classi_sezione);
    const elem *const base = &orario[0];

    // 3. Handle cases where one of the two slots is empty
    if (prof1 == -1 || prof2 == -1) {
        const int valid_prof = prof1 != -1 ? prof1 : prof2;

        for (int i = start_index; i < pos; ++i) {
            if (base[i].insegnante1 == valid_prof | base[i].insegnante2 == valid_prof) {
                return 0; // Collision found
            }
        }
        return 1;
    }

    // 4. THE HOT PATH: Both professors are valid.
    // We check both professors simultaneously in a single memory sweep.
    for (int i = start_index; i < pos; ++i) {
        const int t1 = base[i].insegnante1;
        const int t2 = base[i].insegnante2;

        // Flattened bitwise OR check for maximum ILP (Instruction-Level Parallelism)
        if (t1 == prof1 | t2 == prof1 | t1 == prof2 | t2 == prof2) {
            return 0; // At least one professor is already busy in this hour block
        }
    }

    return 1; // Both professors are free!
}

int insegnanti_ore_materia_adeguata(const int cla, const int mat, const int prof1, const int prof2) {
    assert(prof2 != -1);
    return (prof1 == -1 || ore_insegnanti_classe_materia[
                cla * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof1]) > 0
           && ore_insegnanti_classe_materia[cla * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof2] > 0;
}


int controllo_prof_insieme(const int prof1, const int prof2) {
    const int d1 = is_docente(prof1), c1 = is_codocente(prof1);
    const int d2 = is_docente(prof2), c2 = is_codocente(prof2);

    // Returns 1 (true) if the pair is allowed, 0 (false) if it's a conflict
    return (! (d1 & d2)) & ((c1 + c2) <= (d1 + d2));
}

int is_teacher_eligible_for_class(elem orario[], const int pos, const int class, const int mat, const int prof) {
    if (prof != -1) {
        int sum = 0;
        for (int i = class; i < pos; i += actual_classi_sezione) {
            if (orario[i].materia == mat)
                sum += orario[i].insegnante1 == prof || orario[i].insegnante2 == prof;
        }
        return sum < ore_insegnanti_classe_materia[
                   class * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ + prof];
    }

    return 1;
}

int docente_necessita_compresenza(elem orario[], const int pos, const int class, const int mat, const int prof1,
                                  const int prof2) {
    //se questa funzione viene chiamata vuol dire che prof1 è abilitsto a insegnare e prof2 è assente, viceversa,
    //oppure sono uno il docente e l'altro il codocente
    //in ogni caso, quello diverso da -1 è il docente, perchè il codocente non può essere da solo per il test precedente
    if (prof1 != -1 && prof2 != -1) return 1;

    const int prof = prof1 != -1 ? prof1 : prof2;
    const int classe_materia = class * MATERIA_SZ * INSEGNANTE_SZ + mat * INSEGNANTE_SZ;
    int ore_cod = 0;

    int sum = 0;
    for (int i = 0; i < pos; i++) {
        if (orario[i].materia == mat && g_classe(i) == class)
            sum += orario[i].insegnante1 == prof || orario[i].insegnante2 == prof;
    }

    // trova le ore di compresenza
    for (int i = 0; i < INSEGNANTE_SZ; i++) {
        if (i != prof && ore_insegnanti_classe_materia[classe_materia + i] > 0)
            ore_cod = ore_insegnanti_classe_materia[classe_materia + i];
    }
    return sum < ore_insegnanti_classe_materia[classe_materia + prof] - ore_cod;
}
