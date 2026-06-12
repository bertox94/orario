//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_END_OF_BLOCK_CONSTRAINTS_H
#define UNTITLED_END_OF_BLOCK_CONSTRAINTS_H


int verifica_ore_classe_materia(const elem orario[], const long tot) {
    int ore_insegnanti_classe_materia_curr[CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ];
    memset(ore_insegnanti_classe_materia_curr, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ*sizeof(int));

    for (int i = 0; i < tot; i++) {
        if (orario[i].insegnante1 != -1)
            ore_insegnanti_classe_materia_curr[g_classe(i) * MATERIA_SZ * INSEGNANTE_SZ
                                               + orario[i].materia * INSEGNANTE_SZ
                                               + orario[i].insegnante1]++;
        if (orario[i].insegnante2 != -1)
            ore_insegnanti_classe_materia_curr[g_classe(i) * MATERIA_SZ * INSEGNANTE_SZ
                                               + orario[i].materia * INSEGNANTE_SZ
                                               + orario[i].insegnante2]++;
    }

    for (int i = 0; i < CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ; i++)
        if (ore_insegnanti_classe_materia[i] != ore_insegnanti_classe_materia_curr[i]) {
            printf("Wrong: %d, %d but %d\n",i, ore_insegnanti_classe_materia[i], ore_insegnanti_classe_materia_curr[i]);
            return 0;
        }
    return 1;
}
#endif //UNTITLED_END_OF_BLOCK_CONSTRAINTS_H
