//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include "../data_initialization/database_initializers.h"
#include "../constants/days.h"
#include "../constants/hours.h"
#include "../constants/rooms.h"
#include "../constants/professors.h"
#include "../constants/classes.h"
#include "../constants/subjects.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void verify(int cond, char *msg) {
    if (!cond) {
        perror(msg);
        exit(-1);
    }
}

void fill_giorni_settimana(void) {
    char str[] = "";
    for (int i = actual_giorni_settimana; i < GIORNO_SETTIMANA_SZ; i++)
        set_giorno_settimana(str);
}

void fill_ore_giornaliere(void) {
    char str[] = "";
    for (int i = actual_ore_giornaliere; i < ORA_GIORNALIERA_SZ; i++)
        set_ora_giornaliera(str);
}

void fill_classe_sezione(void) {
    char str[] = "";
    for (int i = actual_classi_sezione; i < CLASSE_SEZIONE_SZ; i++)
        set_classe_sezione(str);
}

void fill_materie(void) {
    char str[] = "";
    for (int i = actual_materie; i < MATERIA_SZ; i++)
        set_materia(str);
}

void fill_insegnanti(void) {
    char str[] = "";
    for (int i = actual_insegnanti; i < INSEGNANTE_SZ; i++)
        set_insegnante(str);
}

int p_insegnante(const char in[]) {
    for (int i = 0; i < actual_insegnanti; i++)
        if (!strcmp(insegnanti[i], in))
            return i;
    return -1;
}

int p_classe_sezione(char cla[]) {
    for (int i = 0; i < actual_classi_sezione; i++)
        if (!strcmp(classe_sezione[i], cla))
            return i;
    return -1;
}

int p_materia(char mat[]) {
    for (int i = 0; i < actual_materie; i++)
        if (!strcmp(materie[i], mat))
            return i;
    return -1;
}

int p_ora(char o[]) {
    for (int i = 0; i < actual_ore_giornaliere; i++)
        if (!strcmp(ore_giornaliere[i], o))
            return i;
    return -1;
}

int p_giorno(char g[]) {
    for (int i = 0; i < actual_giorni_settimana; i++)
        if (!strcmp(giorni_settimana[i], g))
            return i;
    return -1;
}

void set_giorno_settimana(char giorno[]) {
    if (p_giorno(giorno) >= 0) return;
    verify(actual_giorni_settimana < GIORNO_SETTIMANA_SZ, "actual_giorni_settimana < GIORNO_SETTIMANA_SZ");
    verify(strlen(giorno) < GIORNO_SETTIMANA_ITEM_SZ, "strlen(giorno) < GIORNO_SETTIMANA_ITEM_SZ");
    strcpy(giorni_settimana[actual_giorni_settimana++], giorno);
}

void set_ora_giornaliera(char ora[]) {
    if (p_ora(ora) >= 0) return;
    verify(actual_ore_giornaliere < ORA_GIORNALIERA_SZ, "actual_ore_giornaliere < ORA_GIORNALIERA_SZ");
    verify(strlen(ora) < ORA_GIORNALIERA_ITEM_SZ, "strlen(ora)<ORA_GIORNALIERA_ITEM_SZ");
    strcpy(ore_giornaliere[actual_ore_giornaliere++], ora);
}

void set_classe_sezione(char cla[]) {
    if (p_classe_sezione(cla) >= 0) return;
    verify(actual_classi_sezione < CLASSE_SEZIONE_SZ, "actual_classi_sezione<CLASSE_SEZIONE_SZ");
    verify(strlen(cla) < CLASSE_SEZIONE_ITEM_SZ, "strlen(cla)<CLASSE_SEZIONE_ITEM_SZ");
    strcpy(classe_sezione[actual_classi_sezione++], cla);
}


void set_materia(char mat[]) {
    if (p_materia(mat) >= 0) return;
    verify(actual_materie < MATERIA_SZ, "actual_materie < MATERIA_SZ");
    verify(strlen(mat) < MATERIA_ITEM_SZ, "strlen(mat) < MATERIA_ITEM_SZ");
    strcpy(materie[actual_materie++], mat);
}

void set_insegnante(char in[]) {
    if (p_insegnante(in) >= 0) return;
    verify(actual_insegnanti < INSEGNANTE_SZ, "actual_insegnanti < INSEGNANTE_SZ");
    verify(strlen(in) < INSEGNANTE_ITEM_SZ, "strlen(in) < INSEGNANTE_ITEM_SZ");
    strcpy(insegnanti[actual_insegnanti++], in);
}

void set_classe_materia_ore_prof(char cla[], char mat[], const int ore, const char prof[]) {
    set_classe_sezione(cla);
    set_materia(mat);
    set_insegnante(prof);
    const int p_cla = p_classe_sezione(cla);
    const int p_mat = p_materia(mat);
    const int p_prof = p_insegnante(prof);
    const int base = p_cla * MATERIA_SZ * INSEGNANTE_SZ + p_mat * INSEGNANTE_SZ;
    ore_insegnanti_classe_materia[base + p_prof] += ore;
    ore_classe_settimanali[p_cla] += ore;
}
/*
void set_classe_materia_ore_prof_codocenza(char cla[], char mat[], const int ore, const char prof[], const int ore_cod,
                                           const char cod[]) {
    set_classe_sezione(cla);
    set_materia(mat);
    set_insegnante(prof);
    set_insegnante(cod);
    const int p_cla = p_classe_sezione(cla);
    const int p_mat = p_materia(mat);
    const int p_prof = p_insegnante(prof);
    const int p_cod = p_insegnante(cod);
    assert(p_cla!=-1);
    assert(p_mat!=-1);
    assert(p_prof!=-1);
    assert(p_cod!=-1);
    const int base = p_cla * MATERIA_SZ * INSEGNANTE_SZ + p_mat * INSEGNANTE_SZ;
    ore_insegnanti_classe_materia[base + p_prof] += ore;
    ore_insegnanti_classe_materia[base + p_cod] += ore_cod;

    assert(ore>=ore_cod);
    ore_insegnanti_classe_materia_lonely[base + p_prof] += ore - ore_cod;
    ore_classe_settimanali[p_cla] += ore;
}
*/

void inizializza_dati() {
    actual_ore_giornaliere = 0;
    actual_giorni_settimana = 0;
    actual_classi_sezione = 0;
    actual_materie = 0;
    actual_insegnanti = 0;

    memset(ore_classe_settimanali, 0, CLASSE_SEZIONE_SZ*sizeof(int));
    memset(ore_insegnanti_classe_materia, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ*sizeof(int));


    set_giorno_settimana(Lun);
    set_giorno_settimana(Mar);
    set_giorno_settimana(Mer);
    set_giorno_settimana(Gio);
    set_giorno_settimana(Ven);
    set_giorno_settimana(Sab);


    set_ora_giornaliera(Ora_08);
    set_ora_giornaliera(Ora_09);
    set_ora_giornaliera(Ora_10);
    set_ora_giornaliera(Ora_11);
    set_ora_giornaliera(Ora_12);
    set_ora_giornaliera(Ora_13);


    //set_aula(PS15);
    //set_aula(PS16);
    //set_aula(PS14);
    //set_laboratorio(INFO3);


    set_classe_materia_ore_prof(Classe_3E, Matematica, 4, Berno);
    set_classe_materia_ore_prof(Classe_3E, Lingua_Letteratura, 4, DalZovo);
    set_classe_materia_ore_prof(Classe_3E, Storia_Cittadinanza_Costituzione, 2, DalZovo);
    set_classe_materia_ore_prof(Classe_3E, Inglese, 3, Alves);
    set_classe_materia_ore_prof(Classe_3E, ScienzeMotorie, 2, Nicoletti);
    set_classe_materia_ore_prof(Classe_3E, Religione_Cattolica, 1, Quaini);

    set_classe_materia_ore_prof(Classe_3H, Matematica, 2, Tosi);
    set_classe_materia_ore_prof(Classe_3H, Complementi_Matematica, 1, Tosi);
    set_classe_materia_ore_prof(Classe_3H, Lingua_Letteratura, 4, Guerra);
    set_classe_materia_ore_prof(Classe_3H, Storia_Cittadinanza_Costituzione, 2, Guerra);
    set_classe_materia_ore_prof(Classe_3H, Inglese, 3, Lancellotti);
    set_classe_materia_ore_prof(Classe_3H, ScienzeMotorie, 2, Nicoletti);
    set_classe_materia_ore_prof(Classe_3H, Religione_Cattolica, 1, Quaini);


    set_classe_materia_ore_prof(Classe_3F, Matematica, 4, Sannino);
    set_classe_materia_ore_prof(Classe_3F, Lingua_Letteratura, 4, Bortolotti);
    set_classe_materia_ore_prof(Classe_3F, Storia_Cittadinanza_Costituzione, 2, Bortolotti);
    set_classe_materia_ore_prof(Classe_3F, Inglese, 3, Comelli);
    set_classe_materia_ore_prof(Classe_3F, ScienzeMotorie, 2, Calace);
    set_classe_materia_ore_prof(Classe_3F, Religione_Cattolica, 1, Marchione);



    /**/
    /*
        const int div = MATERIA_SZ / INSEGNANTE_SZ;
        for (int i = 0; i < CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ; i++) {
            if (ore_insegnanti_classe_materia_lonely[i]==0)
                ore_classe_settimanali[i / div] += ore_insegnanti_classe_materia[i];
            ore_classe_settimanali[i / div] +=
                    (ore_insegnanti_classe_materia[i] - ore_insegnanti_classe_materia_lonely[i]) / 2 +
                    ore_insegnanti_classe_materia_lonely[i];
        }*/


    for (int i = 0; i < CLASSE_SEZIONE_SZ; i++)
        verify(ore_classe_settimanali[i] <= actual_giorni_settimana * actual_ore_giornaliere,
               "ore_classe_settimanali[i]<=actual_giorni_settimana*actual_ore_giornaliere");

    fill_giorni_settimana();
    fill_ore_giornaliere();
    fill_materie();
    fill_insegnanti();

    //improve print orario one each class in recurs
}
