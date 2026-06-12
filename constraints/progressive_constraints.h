//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_PROGRESSIVE_CONSTRAINTS_H
#define UNTITLED_PROGRESSIVE_CONSTRAINTS_H

int g_giorno(const int p);

int g_ora(const int p);

int g_classe(int p);


int still_activities_for_today(const elem *orario, const int pos);

int prof_in_classe(elem orario[], const long pos, int prof);

int insegnanti_ore_materia_adeguata(int cla, int mat, int prof1, int prof2);

int controllo_prof_insieme(int prof1, int prof2);

int is_teacher_eligible_for_class(elem orario[], const int pos, const int class, const int mat, const int prof);

int verify_lonely_teacher_hours(elem orario[], const int pos, const int class, const int mat, const int prof,
                                int ora_classi_docenti_effettuate[]);

int verify_teacher_hours(elem orario[], const int pos, const int class, const int mat, const int prof1,
                         const int prof2);
#endif //UNTITLED_PROGRESSIVE_CONSTRAINTS_H
