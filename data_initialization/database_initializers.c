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

int p_insegnante(const char in[]) {
    for (int i = 0; i < actual_insegnanti; i++) {
        if (!strcmp(insegnanti[i], in))
            return i;
    }
    //assert(0);
    return -1;
}

int p_classe_sezione(char cla[]) {
    for (int i = 0; i < actual_classi_sezione; i++) {
        if (!strcmp(classe_sezione[i], cla))
            return i;
    }
    return -1;
}

int p_materia(char mat[]) {
    for (int i = 0; i < actual_materie; i++) {
        if (!strcmp(materie[i], mat))
            return i;
    }
    return -1;
}

int p_aula(char a[]) {
    for (int i = 0; i < actual_aula; i++) {
        if (!strcmp(aule[i], a))
            return i;
    }
    assert(0);
    return -1;
}

void set_giorno_settimana(char giorno[], const int pos) {
    assert(pos < GIORNO_SETTIMANA_SZ);
    assert(strlen(giorno) < GIORNO_SETTIMANA_ITEM_SZ);
    strcpy(giorni_settimana[pos], giorno);
}

void set_ora_giornaliera(char ora[], const int pos) {
    assert(pos < ORA_GIORNALIERA_SZ);
    assert(strlen(ora) < ORA_GIORNALIERA_ITEM_SZ);
    strcpy(ora_giornaliera[pos], ora);
}

void set_classe_sezione(char cla[]) {
    if (p_classe_sezione(cla) >= 0) return;
    assert(actual_classi_sezione < CLASSE_SEZIONE_SZ);
    assert(strlen(cla) < CLASSE_SEZIONE_ITEM_SZ);
    strcpy(classe_sezione[actual_classi_sezione++], cla);
}

void set_aula(char aul[], const int pos) {
    assert(pos < AULA_SZ);
    assert(strlen(aul) < AULA_ITEM_SZ);
    strcpy(aule[pos], aul);
}

void set_laboratorio(char lab[], const int pos) {
    set_aula(lab, pos);
    laboratori[pos] = 1;
}

void set_materia(char mat[]) {
    if (p_materia(mat) >= 0) return;
    assert(actual_materie < MATERIA_SZ);
    assert(strlen(mat) < MATERIA_ITEM_SZ);
    strcpy(materie[actual_materie++], mat);
}

void set_insegnante(char in[]) {
    if (p_insegnante(in) >= 0) return;
    assert(actual_insegnanti < INSEGNANTE_SZ);
    assert(strlen(in) < INSEGNANTE_ITEM_SZ);
    strcpy(insegnanti[actual_insegnanti++], in);
}

void set_solo_lez_in_lab(char cod[]) {
    solo_in_lab[p_insegnante(cod)] = 1;
}

void set_classe_materia_ore_prof(char cla[], char mat[], const int ore, const char prof[]) {
    set_classe_sezione(cla);
    set_materia(mat);
    set_insegnante(prof);
    const int p_cla = p_classe_sezione(cla);
    const int p_mat = p_materia(mat);
    const int p_prof = p_insegnante(prof);
    const int base = p_cla * MATERIA_SZ * INSEGNANTE_SZ + p_mat * INSEGNANTE_SZ;
    ore_insegnanti_classe_materia_lonely[base + p_prof] = ore;
    ore_insegnanti_classe_materia[base + p_prof] = ore;
    ore_classe_settimanali[p_cla] += ore;
}

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
    ore_insegnanti_classe_materia[base + p_prof] = ore;
    ore_insegnanti_classe_materia[base + p_cod] = ore_cod;

    assert(ore>=ore_cod);
    ore_insegnanti_classe_materia_lonely[base + p_prof] = ore - ore_cod;
    ore_classe_settimanali[p_cla] += ore;
}

void inizializza_dati() {
    actual_giorni_settimana = 0;
    set_giorno_settimana(Lun, actual_giorni_settimana++);
    set_giorno_settimana(Mar, actual_giorni_settimana++);
    set_giorno_settimana(Mer, actual_giorni_settimana++);
    //set_giorno_settimana(Gio, actual_giorni_settimana++);
    //set_giorno_settimana(Ven, actual_giorni_settimana++);
    //set_giorno_settimana(Sab, actual_giorni_settimana++);
    for (int i = actual_giorni_settimana; i < GIORNO_SETTIMANA_SZ; i++)
        set_giorno_settimana("", i);

    actual_ore_giornaliere = 0;
    set_ora_giornaliera(Ora_08, actual_ore_giornaliere++);
    set_ora_giornaliera(Ora_09, actual_ore_giornaliere++);
    set_ora_giornaliera(Ora_10, actual_ore_giornaliere++);
    set_ora_giornaliera(Ora_11, actual_ore_giornaliere++);
    set_ora_giornaliera(Ora_12, actual_ore_giornaliere++);
    set_ora_giornaliera(Ora_13, actual_ore_giornaliere++);
    for (int i = actual_ore_giornaliere; i < ORA_GIORNALIERA_SZ; i++)
        set_ora_giornaliera("", i);

    actual_aula = 0;
    memset(laboratori, 0, AULA_SZ*sizeof(int));
    //set_aula(PS15, actual_aula++);
    //set_aula(PS16, actual_aula++);
    set_laboratorio(INFO0, actual_aula++);
    //set_laboratorio(INFO1, actual_aula++);
    //set_laboratorio(INFO2, actual_aula++);
}

void inizializza_vincoli() {
    actual_classi_sezione = 0;
    actual_materie = 0;
    actual_insegnanti = 0;
    memset(solo_in_lab, 0, INSEGNANTE_SZ*sizeof(int));
    memset(ore_insegnanti_classe_materia, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ*sizeof(int));
    memset(ore_insegnanti_classe_materia_lonely, 0, CLASSE_SEZIONE_SZ*MATERIA_SZ*INSEGNANTE_SZ*sizeof(int));
    set_classe_materia_ore_prof_codocenza(Classe_3H, Informatica, 6, Tiraboschi, 3, Benedetto);
    set_classe_materia_ore_prof_codocenza(Classe_3H, TPSIT, 3, Manganelli, 1, Rizzi);
    set_classe_materia_ore_prof_codocenza(Classe_3H, Sistemi_e_Reti, 2, Tiraboschi, 2, Gagliano);
    set_classe_materia_ore_prof_codocenza(Classe_3H, Matematica, 1, Tosi, 1, Sannino);
    set_classe_materia_ore_prof(Classe_3H, Complementi_Matematica, 1, Tosi);

    set_solo_lez_in_lab(Benedetto);
    set_solo_lez_in_lab(Rizzi);
    set_solo_lez_in_lab(Gagliano);

    /*set_classe_materia_ore_prof(Classe_3H, Lingua_Letteratura, 4, Guerra);
    set_classe_materia_ore_prof(Classe_3H, Storia_Cittadinanza_Costituzione, 2, Guerra);
    set_classe_materia_ore_prof(Classe_3H, Inglese, 3, Lancellotti);
    set_classe_materia_ore_prof(Classe_3H, Telecomunicazioni, 3, Trivini);
    set_classe_materia_ore_prof(Classe_3H, ScienzeMotorie, 2, Nicoletti);
    set_classe_materia_ore_prof(Classe_3H, Religione_Cattolica, 1, Quaini);
    set_classe_materia_ore_prof_codocenza(Classe_3F, Sistemi_e_Reti, 4, Tiraboschi, 2, Gagliano);
    set_classe_materia_ore_prof_codocenza(Classe_3F, TPSIT, 4, Ricciardi, 2, Rizzi);
    set_classe_materia_ore_prof_codocenza(Classe_3F, Informatica, 6, Kushi, 3, Paghera);
    set_classe_materia_ore_prof_codocenza(Classe_3E, TPSIT, 4, Ricciardi, 2, Rizzi);
    set_classe_materia_ore_prof_codocenza(Classe_3E, Sistemi_e_Reti, 4, Tiraboschi, 2, Barosi);
    set_classe_materia_ore_prof_codocenza(Classe_3E, Informatica, 6, Masetti, 3, Barosi);
    */
    /*
        const int div = MATERIA_SZ / INSEGNANTE_SZ;
        for (int i = 0; i < CLASSE_SEZIONE_SZ * MATERIA_SZ * INSEGNANTE_SZ; i++) {
            if (ore_insegnanti_classe_materia_lonely[i]==0)
                ore_classe_settimanali[i / div] += ore_insegnanti_classe_materia[i];
            ore_classe_settimanali[i / div] +=
                    (ore_insegnanti_classe_materia[i] - ore_insegnanti_classe_materia_lonely[i]) / 2 +
                    ore_insegnanti_classe_materia_lonely[i];
        }*/
    assert(ore_classe_settimanali[0]<=actual_giorni_settimana*actual_ore_giornaliere);
    //improve print orario one each class in recurs
}
