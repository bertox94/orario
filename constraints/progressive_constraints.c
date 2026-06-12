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

int prof_in_classe(elem orario[], const long pos, int prof) {
    // 1. Move the check to the absolute top. If prof is -1, skip everything immediately.
    if (prof == -1) return 1;

    // 2. Optimization: Calculate start_index using bitwise operations if actual_classi_sezione is a power of 2.
    // Otherwise, standard subtraction is fine, but compute it exactly once outside the loop.
    const int start_index = pos - pos % actual_classi_sezione;

    // 3. Pointer-arithmetic & Local caching loop
    const elem* const base = &orario[0]; // Helps compiler realize the base array pointer is invariant
    for (int i = start_index; i < pos; ++i) {
        // Bitwise OR eliminates the short-circuiting branch of logical ||
        if ((base[i].insegnante1 == prof) | (base[i].insegnante2 == prof)) {
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


int codocente_deve_essere_in_lab_no_aula(const int aula, int prof) {
    return !is_codocente(prof) || laboratori[aula];
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
