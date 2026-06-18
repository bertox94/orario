//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_END_OF_BLOCK_CONSTRAINTS_H
#define UNTITLED_END_OF_BLOCK_CONSTRAINTS_H


int verifica_ore_classe_materia(int ora_classi_materie_effettuate[]) {
    for (int i = 0; i < CLASSE_SEZIONE_SZ * INSEGNANTE_SZ; i++)
        if (ore_insegnante_classi[i] != ora_classi_materie_effettuate[i]) {
            /*printf("Wrong: Classe: %s, Materia: %s, Teacher: %s %d but got %d\n",
                   classe_sezione[i / MATERIA_SZ / INSEGNANTE_SZ % CLASSE_SEZIONE_SZ],
                   materie[i / INSEGNANTE_SZ % MATERIA_SZ],
                   insegnanti[i % INSEGNANTE_SZ],
                   ore_insegnanti_classe_materia[i], ora_classi_materie_effettuate[i]);*/
            return 0;
        }
    return 1;
}
#endif //UNTITLED_END_OF_BLOCK_CONSTRAINTS_H
