//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_END_OF_BLOCK_CONSTRAINTS_H
#define UNTITLED_END_OF_BLOCK_CONSTRAINTS_H


int verifica_ore_classe_materia(int ora_classi_materie_effettuate[]) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ; i++)
        if (ore_insegnanti_classe_materia[i] != ora_classi_materie_effettuate[i]) {
            //printf("Wrong: %d, %d but got %d\n", i, ore_insegnanti_classe_materia[i], ora_classi_materie_effettuate[i]);
            return 0;
        }
    return 1;
}
#endif //UNTITLED_END_OF_BLOCK_CONSTRAINTS_H
