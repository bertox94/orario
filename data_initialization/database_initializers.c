//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include "../data_initialization/database_initializers.h"
#include "../constants/days.h"
#include "../constants/hours.h"
#include "../constants/professors.h"
#include "../constants/classes.h"
#include "../constants/subjects.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

#include "../printer/printer.h"

char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

// automatically initialized to zero since they are global
int actual_giorni_settimana;
int actual_ore_giornaliere;
int actual_classi_sezione;
int actual_insegnanti;

int ore_classi_docenti[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_docenti_alone[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_docenti_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_docenti_alone_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi[CLASSE_SEZIONE_SZ];
int ore_classe_svolte[CLASSE_SEZIONE_SZ];
int ore_docenti[INSEGNANTE_SZ];
int ore_docenti_svolte[INSEGNANTE_SZ];
int classi_docenti_teorici[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
int ore_classi_codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
int ore_classi_codocenza_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];

int ora_di_entrata_prof[INSEGNANTE_SZ];
int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];
int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];
/*
 * Conversions from string to id
 */
int p_giorno(char g[]) {
    for (int i = 0; i < actual_giorni_settimana; i++)
        if (!strcmp(giorni_settimana[i], g))
            return i;
    return -1;
}

int p_ora(char o[]) {
    for (int i = 0; i < actual_ore_giornaliere; i++)
        if (!strcmp(ore_giornaliere[i], o))
            return i;
    return -1;
}

int p_classe_sezione(char cla[]) {
    for (int i = 0; i < actual_classi_sezione; i++)
        if (!strcmp(classe_sezione[i], cla))
            return i;
    return -1;
}

int p_insegnante(const char in[]) {
    for (int i = 0; i < actual_insegnanti; i++)
        if (!strcmp(insegnanti[i], in))
            return i;
    return -1;
}

/*
 * Append string to its array and increment respective size.
 */

void set_giorno_settimana(char giorno[]) {
    if (p_giorno(giorno) >= 0) return;
    assert(actual_giorni_settimana < GIORNO_SETTIMANA_SZ);
    assert(strlen(giorno) < MAX_LEN_WORDS);
    strcpy(giorni_settimana[actual_giorni_settimana++], giorno);
}

void set_ora_giornaliera(char ora[]) {
    if (p_ora(ora) >= 0) return;
    assert(actual_ore_giornaliere < ORA_GIORNALIERA_SZ);
    assert(strlen(ora) < MAX_LEN_WORDS);
    strcpy(ore_giornaliere[actual_ore_giornaliere++], ora);
}

void set_classe_sezione(char cla[]) {
    if (p_classe_sezione(cla) >= 0) return;
    assert(actual_classi_sezione < CLASSE_SEZIONE_SZ);
    assert(strlen(cla) < MAX_LEN_WORDS);
    strcpy(classe_sezione[actual_classi_sezione++], cla);
}

void set_insegnante(char in[]) {
    if (p_insegnante(in) >= 0) return;
    assert(actual_insegnanti < INSEGNANTE_SZ);
    assert(strlen(in) < MAX_LEN_WORDS);
    strcpy(insegnanti[actual_insegnanti++], in);
}

/*
 * Access and set constraints
 */
void add_ore_classi_insegnante(int classe, int insegnante, int ore) {
    ore_classi_docenti[classe * INSEGNANTE_SZ + insegnante] += ore;
}

void add_ore_classi_insegnante_lonely(int classe, int insegnante, int ore) {
    ore_classi_docenti_alone[classe * INSEGNANTE_SZ + insegnante] += ore;
}

void add_ore_classi_settimanale(int classe, int ore) {
    ore_classi[classe] += ore;
}

void add_ore_insegnante_settimanale(int insegnante, int ore) {
    ore_docenti[insegnante] += ore;
}

void add_ore_classi_codocenza(int cla, int insegnante1, int insegnante2, int ore) {
    int p1 = fmin(insegnante1, insegnante2);
    int p2 = fmax(insegnante2, insegnante1);
    ore_classi_codocenza[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2] += ore;
}

int get_ore_classi_insegnante(int classe, int insegnante) {
    return ore_classi_docenti[classe * INSEGNANTE_SZ + insegnante];
}

int get_ore_classi_insegnante_svolte(int classe, int insegnante) {
    return ore_classi_docenti_svolte[classe * INSEGNANTE_SZ + insegnante];
}

int get_ore_classi_settimanale(int classe) {
    return ore_classi[classe];
}

int get_ore_insegnante_settimanale(int insegnante) {
    return ore_docenti[insegnante];
}

/*
 * Set constraints from user
*/

void add_to_queue(int prof, int priority) {
    for (int i = 0; i < queue_sz; i++) {
        if (queue_mul[i]->teacher == prof) {
            queue_mul[i]->priority += priority;
            return;
        }
    }
    QueueItem *new = malloc(sizeof(QueueItem));
    new->teacher = prof;
    new->priority = priority;
    queue_mul[queue_sz++] = new;
    for (int i = 1;
         i < GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ; i++) {
        queue_mul[i * INSEGNANTE_SZ * INSEGNANTE_SZ + queue_sz - 1] = queue_mul[queue_sz - 1];
    }
}

void set_classe_insegnante_ore(char cla[], char prof[], const int ore) {
    set_classe_sezione(cla);
    set_insegnante(prof);
    classi_docenti_teorici[p_classe_sezione(cla) * INSEGNANTE_SZ + p_insegnante(prof)] = 1;
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_insegnante_lonely(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);
    add_ore_insegnante_settimanale(p_insegnante(prof), ore);
    add_to_queue(p_insegnante(prof), ore);
}

void set_classe_insegnante_ore_cod(char cla[], char prof[], const int ore, char cod[], const int ore_cod) {
    set_classe_sezione(cla);
    set_insegnante(prof);
    classi_docenti_teorici[p_classe_sezione(cla) * INSEGNANTE_SZ + p_insegnante(prof)] = 1;
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_insegnante_lonely(p_classe_sezione(cla), p_insegnante(prof), ore - ore_cod);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);
    add_ore_insegnante_settimanale(p_insegnante(prof), ore);

    set_insegnante(cod);
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(cod), ore_cod);
    add_ore_insegnante_settimanale(p_insegnante(cod), ore_cod);
    add_ore_classi_codocenza(p_classe_sezione(cla), p_insegnante(prof), p_insegnante(cod), ore_cod);
    add_to_queue(p_insegnante(cod), ore_cod);
    add_to_queue(p_insegnante(prof), ore);
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

    queue_mul = malloc(
        sizeof(QueueItem *) * GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ *
        INSEGNANTE_SZ);

    set_classe_insegnante_ore(Classe_3A, Saredn, 2);
    set_classe_insegnante_ore(Classe_3A, sdjbkl, 3);
    set_classe_insegnante_ore(Classe_3A, dskds, 6);
    set_classe_insegnante_ore_cod(Classe_3A, sdljjbl, 5, hsdlo, 3);
    set_classe_insegnante_ore(Classe_3A, sfdgdfs, 4);
    set_classe_insegnante_ore_cod(Classe_3A, dfkkjosd, 4, dskjlo, 2);
    set_classe_insegnante_ore_cod(Classe_3A, ksjd, 4, hjdlso, 2);
    set_classe_insegnante_ore(Classe_3A, sdjhkjlo, 1);
    set_classe_insegnante_ore_cod(Classe_3A, kjhbusdkl, 3, kfdsoa, 3);

    set_classe_insegnante_ore_cod(Classe_3B, kjhbusdkl, 5, hksld, 4);
    set_classe_insegnante_ore(Classe_3B, fdkjo, 6);
    set_classe_insegnante_ore_cod(Classe_3B, jcxliosd, 4, dskjlo, 2);
    set_classe_insegnante_ore(Classe_3B, Saredn, 2);
    set_classe_insegnante_ore_cod(Classe_3B, ksjd, 3, hsdlo, 3);
    set_classe_insegnante_ore(Classe_3B, sdjhkjlo, 1);
    set_classe_insegnante_ore_cod(Classe_3B, fudc, 4, jknsosd, 1);
    set_classe_insegnante_ore_cod(Classe_3B, dfkkjosd, 4, hjdlso, 2);
    set_classe_insegnante_ore(Classe_3B, fviosoda, 3);


    set_classe_insegnante_ore(Classe_3C, ksdmolf, 3);
    set_classe_insegnante_ore(Classe_3C, lkjdns, 6);
    set_classe_insegnante_ore_cod(Classe_3C, fdhls, 5, sdkkkj, 2);
    set_classe_insegnante_ore_cod(Classe_3C, kjsdlnkn, 4, jknsosd, 1);
    set_classe_insegnante_ore_cod(Classe_3C, ocsfg, 3, kjlidsola, 2);
    set_classe_insegnante_ore(Classe_3C, shdlnk, 1);
    set_classe_insegnante_ore(Classe_3C, sdils, 2);
    set_classe_insegnante_ore_cod(Classe_3C, fdkjl, 4, hldsofa, 2);
    set_classe_insegnante_ore_cod(Classe_3C, yhosio, 4, sdlioa, 2);

    set_classe_insegnante_ore_cod(Classe_3D, jsdoo, 5, sdlioa, 2);
    set_classe_insegnante_ore_cod(Classe_3D, jksdklo, 4, hldsofa, 2);
    set_classe_insegnante_ore_cod(Classe_3D, ocsfg, 3, kjlidsola, 2);
    set_classe_insegnante_ore_cod(Classe_3D, fdjl, 4, fudc, 1);
    set_classe_insegnante_ore(Classe_3D, dfjiloa, 3);
    set_classe_insegnante_ore(Classe_3D, dfjo, 6);
    set_classe_insegnante_ore(Classe_3D, shdlnk, 1);
    set_classe_insegnante_ore_cod(Classe_3D, lodfas, 4, hldsofa, 2);
    set_classe_insegnante_ore(Classe_3D, jkvklmosdo, 2);

    set_classe_insegnante_ore(Classe_3E, dshb, 4);
    set_classe_insegnante_ore(Classe_3E, sdhyk, 6);
    set_classe_insegnante_ore_cod(Classe_3E, sdhusdc, 3, jhsbdhjkl, 1);
    set_classe_insegnante_ore_cod(Classe_3E, jhkssdf, 6, hbdskj, 3);
    set_classe_insegnante_ore(Classe_3E, sdils, 2);
    set_classe_insegnante_ore_cod(Classe_3E, jndslnk, 3, sdkkkj, 2);
    set_classe_insegnante_ore(Classe_3E, shdlnk, 1);
    set_classe_insegnante_ore_cod(Classe_3E, jhbksdjl, 4, jhsbdhjkl, 2);
    set_classe_insegnante_ore(Classe_3E, hgsdj, 3); //130 KHz

    set_classe_insegnante_ore(Classe_3F, jknsosd, 4);
    set_classe_insegnante_ore(Classe_3F, sdhbhkl, 6);
    set_classe_insegnante_ore_cod(Classe_3F, hjjsdlj, 3, jhdsnlkllds, 1);
    set_classe_insegnante_ore_cod(Classe_3F, kjnsdu, 6, hjksdd, 3);
    set_classe_insegnante_ore(Classe_3F, sdils, 2);
    set_classe_insegnante_ore_cod(Classe_3F, jndslnk, 3, sdkkkj, 2);
    set_classe_insegnante_ore(Classe_3F, shdlnk, 1);
    set_classe_insegnante_ore_cod(Classe_3F, jhbksdjl, 4, jhsbdhjkl, 2);
    set_classe_insegnante_ore(Classe_3F, jdslk, 3); //91 KHz

    set_classe_insegnante_ore(Classe_3G, kjjkds, 2);
    set_classe_insegnante_ore(Classe_3G, sdljjbl, 5);
    set_classe_insegnante_ore_cod(Classe_3G, sdjklol, 4, sdlioa, 2);
    set_classe_insegnante_ore(Classe_3G, fsghs, 6);
    set_classe_insegnante_ore_cod(Classe_3G, lljksdo, 3, fudc, 2);
    set_classe_insegnante_ore_cod(Classe_3G, ljosd, 4, hjdlso, 3);
    set_classe_insegnante_ore(Classe_3G, sdjhkjlo, 1);
    set_classe_insegnante_ore(Classe_3G, joflo, 5);
    set_classe_insegnante_ore(Classe_3G, jkvklmosdo, 2); //65 KHz

    set_classe_insegnante_ore(Classe_3K, kojsdfoa, 4);
    set_classe_insegnante_ore(Classe_3K, khjbdklsko, 4);
    set_classe_insegnante_ore(Classe_3K, kmjsdoa, 6);
    set_classe_insegnante_ore(Classe_3K, fdj, 3);
    set_classe_insegnante_ore(Classe_3K, mkjsdolo, 6);
    set_classe_insegnante_ore(Classe_3K, jkvklmosdo, 2);
    set_classe_insegnante_ore(Classe_3K, hjgasdkbl, 2);
    set_classe_insegnante_ore(Classe_3K, fdsj, 4);
    set_classe_insegnante_ore(Classe_3K, khsdubkhl, 1); //440 Hz

    set_classe_insegnante_ore_cod(Classe_3H, kjsdlnkn, 4, jknsosd, 1);
    set_classe_insegnante_ore(Classe_3H, lkjdns, 6);
    set_classe_insegnante_ore_cod(Classe_3H, jhbdsjksd, 3, jhdsnlkllds, 1);
    set_classe_insegnante_ore_cod(Classe_3H, jhbksdjl, 6, kjhbdksl, 3);
    set_classe_insegnante_ore(Classe_3H, sdils, 2);
    set_classe_insegnante_ore_cod(Classe_3H, jndslnk, 3, sdkkkj, 2);
    set_classe_insegnante_ore(Classe_3H, shdlnk, 1);
    set_classe_insegnante_ore_cod(Classe_3H, jhbksdjl, 4, hsdl, 2);
    set_classe_insegnante_ore(Classe_3H, ksdmolf, 3); //38 KHz

    set_classe_insegnante_ore(Classe_3Q, hsdblkjl, 6);
    set_classe_insegnante_ore_cod(Classe_3Q, jsdoo, 7, hldsofa, 4);
    set_classe_insegnante_ore_cod(Classe_3Q, ocsfg, 4, hldsofa, 2);
    set_classe_insegnante_ore_cod(Classe_3Q, ubsdf, 6, jhlfoda, 2);
    set_classe_insegnante_ore(Classe_3Q, jhdksdf, 5);
    set_classe_insegnante_ore(Classe_3Q, khdusf, 4);
    set_classe_insegnante_ore(Classe_3Q, hjdkls, 2);
    set_classe_insegnante_ore(Classe_3Q, khsdubkhl, 1); //28 KHz

    set_classe_insegnante_ore(Classe_4A, fdkjo, 6);
    set_classe_insegnante_ore_cod(Classe_4A, djhsljofa, 3, dskjlo, 2);
    set_classe_insegnante_ore_cod(Classe_4A, djhsljofa, 4, hjdlso, 2);
    set_classe_insegnante_ore(Classe_4A, sdjbkl, 3);
    set_classe_insegnante_ore_cod(Classe_4A, ksjd, 5, kfdsoa, 5);
    set_classe_insegnante_ore(Classe_4A, kojsdfoa, 4);
    set_classe_insegnante_ore(Classe_4A, ljosd, 4);
    set_classe_insegnante_ore(Classe_4A, sdjhkjlo, 1);
    set_classe_insegnante_ore(Classe_4A, Saredn, 2); //20 KHz

    set_classe_insegnante_ore(Classe_4B, jhklodfsd, 4);
    set_classe_insegnante_ore_cod(Classe_4B, djhsljofa, 3, dskjlo, 2);
    set_classe_insegnante_ore(Classe_4B, sdljjbl, 4);
    set_classe_insegnante_ore(Classe_4B, sdhyk, 6);
    set_classe_insegnante_ore_cod(Classe_4B, jcxliosd, 5, hksld, 5);
    set_classe_insegnante_ore_cod(Classe_4B, dfkkjosd, 4, hjdlso, 2);
    set_classe_insegnante_ore(Classe_4B, fviosoda, 3);
    set_classe_insegnante_ore(Classe_4B, sdjhkjlo, 1);
    set_classe_insegnante_ore(Classe_4B, Saredn, 2); //23 Khz

    set_classe_insegnante_ore(Classe_4C, lljksdo, 4);
    set_classe_insegnante_ore(Classe_4C, jhdkslof, 6);
    set_classe_insegnante_ore(Classe_4C, dfjiloa, 3);
    set_classe_insegnante_ore(Classe_4C, fdkjl, 5);
    set_classe_insegnante_ore(Classe_4C, yhosio, 5);
    set_classe_insegnante_ore(Classe_4C, jklds, 6);
    set_classe_insegnante_ore(Classe_4C, khsdubkhl, 1);
    set_classe_insegnante_ore(Classe_4C, sdils, 2); //20 KHz

    set_classe_insegnante_ore(Classe_4D, fdkjl, 6);
    set_classe_insegnante_ore(Classe_4D, jhklodfsd, 3);
    set_classe_insegnante_ore(Classe_4D, fdjl, 4);
    set_classe_insegnante_ore(Classe_4D, sdjklol, 5);
    set_classe_insegnante_ore(Classe_4D, kghsdljof, 6);
    set_classe_insegnante_ore(Classe_4D, ocsfg, 5);
    set_classe_insegnante_ore(Classe_4D, sdjhkjlo, 1);
    set_classe_insegnante_ore(Classe_4D, sdils, 2); //18 KHz


    set_classe_insegnante_ore(Classe_4E, dshb, 4);
    set_classe_insegnante_ore(Classe_4E, fsghs, 6);
    set_classe_insegnante_ore(Classe_4E, sdhusdc, 7);
    set_classe_insegnante_ore(Classe_4E, jhkssdf, 6);
    set_classe_insegnante_ore(Classe_4E, sdkjl, 2);
    set_classe_insegnante_ore(Classe_4E, jndslnk, 3);
    set_classe_insegnante_ore(Classe_4E, shdlnk, 1);
    set_classe_insegnante_ore(Classe_4E, hgsdj, 3);

    set_classe_insegnante_ore(Classe_5E, dshb, 3);
    set_classe_insegnante_ore(Classe_5E, sdhyk, 6);
    set_classe_insegnante_ore(Classe_5E, sdhusdc, 4);
    set_classe_insegnante_ore(Classe_5E, jhkssdf, 6);
    set_classe_insegnante_ore(Classe_5E, sdils, 2);
    set_classe_insegnante_ore(Classe_5E, sdhbls, 4);
    set_classe_insegnante_ore(Classe_5E, dshbl, 3);
    set_classe_insegnante_ore(Classe_5E, hgsdj, 3);
    set_classe_insegnante_ore(Classe_5E, shdlnk, 1);


    // VINCOLI SU ORA DI ENTRATA
    ora_di_entrata_prof[p_insegnante(sdhusdc)] = 1;
    ora_di_entrata_prof[p_insegnante(jhkssdf)] = 1;
    ora_di_entrata_prof[p_insegnante(jhbksdjl)] = 2;


    for (int i = 0; i < actual_classi_sezione; i++) {
        assert(ore_classi[i] <= actual_giorni_settimana * actual_ore_giornaliere);
        assert(ore_classi[i] >= PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
    }
}
