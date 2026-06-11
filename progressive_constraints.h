//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_PROGRESSIVE_CONSTRAINTS_H
#define UNTITLED_PROGRESSIVE_CONSTRAINTS_H

int g_classe(int p);


int no_lessons_after_end_activities_for_today(const elem *orario, const int pos) ;

int prof_in_classe(elem orario[], const long pos, int prof);

int insegnanti_ore_materia_adeguata(int cla, int mat, int prof1, int prof2);

int codocente_deve_essere_in_lab_no_aula(int aula, int prof1, int prof2);

int controllo_prof_insieme(int prof1, int prof2);

int is_teacher_eligible_for_class(elem orario[], const int pos, const int mat, const int prof);
#endif //UNTITLED_PROGRESSIVE_CONSTRAINTS_H
