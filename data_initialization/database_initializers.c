//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"
#include "../constants/days.h"
#include "../constants/hours.h"
#include "../constants/professors.h"
#include "../constants/classes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils.h"

#include "../printer/printer.h"

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
int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

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
    perror(g);
    exit(-1);
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
    for (int i = 0; i < insegnanti_COUNT; i++)
        if (!strcmp(insegnanti[i], in))
            return i;
    perror(in);
    exit(-1);
    return -1;
}

/*
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
*/

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

void add_ore_classi_codocenza(int cla, int p1, int p2, int ore) {
    swap_min_first(&p1, &p2);
    ore_classi_codocenza[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2] += ore;
}

void increment_ore_classi_codocenza_svolte(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    ore_classi_codocenza_svolte[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2]++;
}

void decrement_ore_classi_codocenza_svolte(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    ore_classi_codocenza_svolte[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2]--;
}

int get_ore_insegnanti_codocenza_svolte_in_una_classe(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    return ore_classi_codocenza_svolte[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2];
}

int get_ore_insegnanti_codocenza_in_una_classe(const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    return ore_classi_codocenza[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2];
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


void set_classe_insegnante_ore(char cla[], char prof[], const int ore) {
    classi_docenti_teorici[p_classe_sezione(cla) * INSEGNANTE_SZ + p_insegnante(prof)] = 1;
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_insegnante_lonely(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);
    add_ore_insegnante_settimanale(p_insegnante(prof), ore);
    add_to_queue(p_insegnante(prof), ore);
}

void set_classe_insegnante_ore_cod(char cla[], char prof[], const int ore, char cod[], const int ore_cod) {
    classi_docenti_teorici[p_classe_sezione(cla) * INSEGNANTE_SZ + p_insegnante(prof)] = 1;
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_insegnante_lonely(p_classe_sezione(cla), p_insegnante(prof), ore - ore_cod);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);
    add_ore_insegnante_settimanale(p_insegnante(prof), ore);

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
    set_classe_insegnante_ore("Classe_3AG", "D. Malfoy", 2);
    set_classe_insegnante_ore("Classe_3AG", "Voldemort", 3);
    set_classe_insegnante_ore("Classe_3AG", "Itachi", 6);
    set_classe_insegnante_ore_cod("Classe_3AG", "Lupin", 5, "G. Vermilinguo", 3);
    set_classe_insegnante_ore("Classe_3AG", "Brook", 4);
    set_classe_insegnante_ore_cod("Classe_3AG", "Ash", 4, "Theoden", 2);
    set_classe_insegnante_ore_cod("Classe_3AG", "EJaeger", 4, "Denethor", 2);
    set_classe_insegnante_ore("Classe_3AG", "RWeasley", 1);
    set_classe_insegnante_ore_cod("Classe_3AG", "Mufasa", 3, "MUchiha", 3);

    set_classe_insegnante_ore_cod("Classe_3BG", "Mufasa", 5, "Radagast", 4);
    set_classe_insegnante_ore("Classe_3BG", "Simba", 6);
    set_classe_insegnante_ore_cod("Classe_3BG", "Nala", 4, "Theoden", 2);
    set_classe_insegnante_ore("Classe_3BG", "D. Malfoy", 2);
    set_classe_insegnante_ore_cod("Classe_3BG", "EJaeger", 3, "G. Vermilinguo", 3);
    set_classe_insegnante_ore("Classe_3BG", "RWeasley", 1);
    set_classe_insegnante_ore_cod("Classe_3BG", "Sarabi", 4, "MTartarughe", 1);
    set_classe_insegnante_ore_cod("Classe_3BG", "Ash", 4, "Denethor", 2);
    set_classe_insegnante_ore("Classe_3BG", "Pumba", 3);

    set_classe_insegnante_ore("Classe_3AI", "Goten", 3);
    set_classe_insegnante_ore("Classe_3AI", "Trunks", 6);
    set_classe_insegnante_ore_cod("Classe_3AI", "Timon", 5, "Jesus", 2);
    set_classe_insegnante_ore_cod("Classe_3AI", "Taipei", 4, "M. Tartarughe", 1);
    set_classe_insegnante_ore_cod("Classe_3AI", "Zazu", 3, "Feanor", 2);
    set_classe_insegnante_ore("Classe_3AI", "Doraemon", 1);
    set_classe_insegnante_ore("Classe_3AI", "Gohan", 2);
    set_classe_insegnante_ore_cod("Classe_3AI", "Rafiki", 4, "Gothmog", 2);
    set_classe_insegnante_ore_cod("Classe_3AI", "Jasmine", 4, "Morgoth", 2);

    set_classe_insegnante_ore_cod("Classe_3BI", "Mammolo", 5, "Morgoth", 2);
    set_classe_insegnante_ore_cod("Classe_3BI", "Sauron", 4, "Gothmog", 2);
    set_classe_insegnante_ore_cod("Classe_3BI", "Zazu", 3, "Feanor", 2);
    set_classe_insegnante_ore_cod("Classe_3BI", "Aladino", 4, "Sarabi", 1);
    set_classe_insegnante_ore("Classe_3BI", "Biancaneve", 3);
    set_classe_insegnante_ore("Classe_3BI", "Dotto", 6);
    set_classe_insegnante_ore("Classe_3BI", "Doraemon", 1);
    set_classe_insegnante_ore_cod("Classe_3BI", "Pisolo", 4, "Gothmog", 2);
    set_classe_insegnante_ore("Classe_3BI", "Arwen", 2);

    set_classe_insegnante_ore("Classe_3AM", "Jiraya", 4);
    set_classe_insegnante_ore("Classe_3AM", "Orochimaru", 6);
    set_classe_insegnante_ore_cod("Classe_3AM", "Naruto", 3, "Freezer", 1);
    set_classe_insegnante_ore_cod("Classe_3AM", "Cell", 6, "Sasuke", 3);
    set_classe_insegnante_ore("Classe_3AM", "Gohan", 2);
    set_classe_insegnante_ore_cod("Classe_3AM", "Sandokan", 3, "Jesus", 2);
    set_classe_insegnante_ore("Classe_3AM", "Doraemon", 1);
    set_classe_insegnante_ore_cod("Classe_3AM", "Vegeta", 4, "Freezer", 2);
    set_classe_insegnante_ore("Classe_3AM", "Tsunade", 3); //130 KHz

    set_classe_insegnante_ore("Classe_3BM", "M. Tartarughe", 4);
    set_classe_insegnante_ore("Classe_3BM", "Sakura", 6);
    set_classe_insegnante_ore_cod("Classe_3BM", "Goku", 3, "M. Bu", 1);
    set_classe_insegnante_ore_cod("Classe_3BM", "Chichi", 6, "Bulma", 3);
    set_classe_insegnante_ore("Classe_3BM", "Gohan", 2);
    set_classe_insegnante_ore_cod("Classe_3BM", "Sandokan", 3, "Jesus", 2);
    set_classe_insegnante_ore("Classe_3BM", "Doraemon", 1);
    set_classe_insegnante_ore_cod("Classe_3BM", "Vegeta", 4, "Freezer", 2);
    set_classe_insegnante_ore("Classe_3BM", "H. Potter", 3); //91 KHz

    set_classe_insegnante_ore("Classe_1BG", "B. Baggins", 2);
    set_classe_insegnante_ore("Classe_1BG", "Lupin", 5);
    set_classe_insegnante_ore_cod("Classe_1BG", "S. Gamgee", 4, "Morgoth", 2);
    set_classe_insegnante_ore("Classe_1BG", "H. Granger", 6);
    set_classe_insegnante_ore_cod("Classe_1BG", "Pipino", 3, "Sarabi", 2);
    set_classe_insegnante_ore_cod("Classe_1BG", "Merry", 4, "Denethor", 3);
    set_classe_insegnante_ore("Classe_1BG", "R. Weasley", 1);
    set_classe_insegnante_ore("Classe_1BG", "Elrond", 5);
    set_classe_insegnante_ore("Classe_1BG", "Arwen", 2); //65 KHz

    set_classe_insegnante_ore("Classe_1AI", "Saruman", 4);
    set_classe_insegnante_ore("Classe_1AI", "Gandalf", 4);
    set_classe_insegnante_ore("Classe_1AI", "Legolas", 6);
    set_classe_insegnante_ore("Classe_1AI", "Aragorn", 3);
    set_classe_insegnante_ore("Classe_1AI", "F. Baggins", 6);
    set_classe_insegnante_ore("Classe_1AI", "Arwen", 2);
    set_classe_insegnante_ore("Classe_1AI", "Gimli", 2);
    set_classe_insegnante_ore("Classe_1AI", "Glorfindel", 4);
    set_classe_insegnante_ore("Classe_1AI", "Galadriel", 1); //440 Hz

    set_classe_insegnante_ore_cod("Classe_1BI", "Taipei", 4, "M. Tartarughe", 1);
    set_classe_insegnante_ore("Classe_1BI", "Trunks", 6);
    set_classe_insegnante_ore_cod("Classe_1BI", "Crillin", 3, "M. Bu", 1);
    set_classe_insegnante_ore_cod("Classe_1BI", "Vegeta", 6, "Gogeta", 3);
    set_classe_insegnante_ore("Classe_1BI", "Gohan", 2);
    set_classe_insegnante_ore_cod("Classe_1BI", "Sandokan", 3, "Jesus", 2);
    set_classe_insegnante_ore("Classe_1BI", "Doraemon", 1);
    set_classe_insegnante_ore_cod("Classe_1BI", "Vegeta", 4, "Janemba", 2);
    set_classe_insegnante_ore("Classe_1BI", "Goten", 3); //38 KHz

    set_classe_insegnante_ore("Classe_1BM", "Haldir", 6);
    set_classe_insegnante_ore_cod("Classe_1BM", "Mammolo", 7, "Gothmog", 4);
    set_classe_insegnante_ore_cod("Classe_1BM", "Zazu", 4, "Gothmog", 2);
    set_classe_insegnante_ore_cod("Classe_1BM", "Eomer", 6, "Fingolfin", 2);
    set_classe_insegnante_ore("Classe_1BM", "Eowyn", 5);
    set_classe_insegnante_ore("Classe_1BM", "Celeborn", 4);
    set_classe_insegnante_ore("Classe_1BM", "Lurtz", 2);
    set_classe_insegnante_ore("Classe_1BM", "Galadriel", 1); //28 KHz

    set_classe_insegnante_ore("Classe_4AG", "Simba", 6);
    set_classe_insegnante_ore_cod("Classe_4AG", "T. Bombadil", 3, "Theoden", 2);
    set_classe_insegnante_ore_cod("Classe_4AG", "T. Bombadil", 4, "Denethor", 2);
    set_classe_insegnante_ore("Classe_4AG", "Voldemort", 3);
    set_classe_insegnante_ore_cod("Classe_4AG", "E. Jaeger", 5, "M. Uchiha", 5);
    set_classe_insegnante_ore("Classe_4AG", "Saruman", 4);
    set_classe_insegnante_ore("Classe_4AG", "Merry", 4);
    set_classe_insegnante_ore("Classe_4AG", "R. Weasley", 1);
    set_classe_insegnante_ore("Classe_4AG", "D. Malfoy", 2); //20 KHz

    set_classe_insegnante_ore("Classe_4BG", "Boromir", 4);
    set_classe_insegnante_ore_cod("Classe_4BG", "T. Bombadil", 3, "Theoden", 2);
    set_classe_insegnante_ore("Classe_4BG", "Lupin", 4);
    set_classe_insegnante_ore("Classe_4BG", "Orochimaru", 6);
    set_classe_insegnante_ore_cod("Classe_4BG", "Nala", 5, "Radagast", 5);
    set_classe_insegnante_ore_cod("Classe_4BG", "Ash", 4, "Denethor", 2);
    set_classe_insegnante_ore("Classe_4BG", "Pumba", 3);
    set_classe_insegnante_ore("Classe_4BG", "R. Weasley", 1);
    set_classe_insegnante_ore("Classe_4BG", "D. Malfoy", 2); //23 Khz

    set_classe_insegnante_ore("Classe_4AI", "Pipino", 4);
    set_classe_insegnante_ore("Classe_4AI", "Faramir", 6);
    set_classe_insegnante_ore("Classe_4AI", "Biancaneve", 3);
    set_classe_insegnante_ore("Classe_4AI", "Rafiki", 5);
    set_classe_insegnante_ore("Classe_4AI", "Jasmine", 5);
    set_classe_insegnante_ore("Classe_4AI", "Barbalbero", 6);
    set_classe_insegnante_ore("Classe_4AI", "Galadriel", 1);
    set_classe_insegnante_ore("Classe_4AI", "Gohan", 2); //20 KHz

    set_classe_insegnante_ore("Classe_4BI", "Rafiki", 6);
    set_classe_insegnante_ore("Classe_4BI", "Boromir", 3);
    set_classe_insegnante_ore("Classe_4BI", "Aladino", 4);
    set_classe_insegnante_ore("Classe_4BI", "S. Gamgee", 5);
    set_classe_insegnante_ore("Classe_4BI", "Isildur", 6);
    set_classe_insegnante_ore("Classe_4BI", "Zazu", 5);
    set_classe_insegnante_ore("Classe_4BI", "R. Weasley", 1);
    set_classe_insegnante_ore("Classe_4BI", "Gohan", 2); //18 KHz

    set_classe_insegnante_ore("Classe_4AM", "Jiraya", 4);
    set_classe_insegnante_ore("Classe_4AM", "H. Granger", 6);
    set_classe_insegnante_ore("Classe_4AM", "Naruto", 7);
    set_classe_insegnante_ore("Classe_4AM", "Cell", 6);
    set_classe_insegnante_ore("Classe_4AM", "M. C. Granitt", 2);
    set_classe_insegnante_ore("Classe_4AM", "Sandokan", 3);
    set_classe_insegnante_ore("Classe_4AM", "Doraemon", 1);
    set_classe_insegnante_ore("Classe_4AM", "Tsunade", 3);

    set_classe_insegnante_ore("Classe_4BM", "Jiraya", 3);
    set_classe_insegnante_ore("Classe_4BM", "Orochimaru", 6);
    set_classe_insegnante_ore("Classe_4BM", "Naruto", 4);
    set_classe_insegnante_ore("Classe_4BM", "Cell", 6);
    set_classe_insegnante_ore("Classe_4BM", "Gohan", 2);
    set_classe_insegnante_ore("Classe_4BM", "A. Silente", 4);
    set_classe_insegnante_ore("Classe_4BM", "Hagrid", 3);
    set_classe_insegnante_ore("Classe_4BM", "Tsunade", 3);
    set_classe_insegnante_ore("Classe_4BM", "Doraemon", 1);


    // VINCOLI SU ORA DI ENTRATA
    ora_di_entrata_prof[p_insegnante("Naruto")] = 1;
    ora_di_entrata_prof[p_insegnante("Cell")] = 1;
    ora_di_entrata_prof[p_insegnante("Vegeta")] = 2;


    for (int i = 0; i < actual_classi_sezione; i++) {
        assert(ore_classi[i] <= actual_giorni_settimana * actual_ore_giornaliere);
        assert(ore_classi[i] >= PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
    }
}
