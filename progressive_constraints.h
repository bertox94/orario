//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_PROGRESSIVE_CONSTRAINTS_H
#define UNTITLED_PROGRESSIVE_CONSTRAINTS_H

int prof_in_classe(const int prof1, const int prof2, int prof_in_classe[]);

int insegnanti_materia_adeguata(const int cla, const int mat, const int prof1, const int prof2);

int codocente_deve_essere_in_lab_no_aula(const int aula, int prof1, int prof2);

int controllo_prof_insieme(const int prof1, const int prof2);
#endif //UNTITLED_PROGRESSIVE_CONSTRAINTS_H
