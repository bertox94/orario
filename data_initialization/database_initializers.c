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

char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS];
char ore_giornaliere[ORA_GIORNALIERA_SZ][MAX_LEN_WORDS];
char classe_sezione[CLASSE_SEZIONE_SZ][MAX_LEN_WORDS];
char insegnanti[INSEGNANTE_SZ][MAX_LEN_WORDS];

// quanti giorni, ore, classi e docenti ci sono davvero (gli array sono grandi per il massimo)
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
int ore_classi_codocenza[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
int ore_classi_codocenza_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];

int ora_di_entrata_prof[INSEGNANTE_SZ];
int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];
int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];
// ---- dal nome all'indice (-1 se non c'e') ----
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

// ---- registrare un nome: lo aggiunge in fondo se non c'e' ancora (l'ordine di registrazione da' gli indici) ----

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

// ---- le ore ----
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

// ---- le lezioni ----
// Una lezione: il docente prof fa `ore` ore nella classe cla, tutte da solo. Registra classe e docente e lo mette in
// coda (la sua priorita' cresce delle ore).

void set_classe_insegnante_ore(char cla[], char prof[], const int ore) {
    set_classe_sezione(cla);
    set_insegnante(prof);
    add_ore_classi_insegnante(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_insegnante_lonely(p_classe_sezione(cla), p_insegnante(prof), ore);
    add_ore_classi_settimanale(p_classe_sezione(cla), ore);
    add_ore_insegnante_settimanale(p_insegnante(prof), ore);
    add_to_queue(p_insegnante(prof), ore);
}

// Una lezione con codocente: prof fa `ore` ore nella classe, di cui ore_cod insieme a cod (e ore - ore_cod da solo).
void set_classe_insegnante_ore_cod(char cla[], char prof[], const int ore, char cod[], const int ore_cod) {
    set_classe_sezione(cla);
    set_insegnante(prof);
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


void inizializza_dati() {
    for (int g = 0; g < N_GIORNI; g++) set_giorno_settimana(giorni[g]);
    for (int h = 0; h < N_ORE_GIORNO; h++) set_ora_giornaliera(ore_giorno[h]);

    queue_mul = malloc(
        sizeof(QueueItem *) * GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ);

    set_classe_insegnante_ore(Classe_3A, professori[DMalfoy], 2);
    set_classe_insegnante_ore(Classe_3A, professori[Voldemort], 3);
    set_classe_insegnante_ore(Classe_3A, professori[Itachi], 6);
    set_classe_insegnante_ore_cod(Classe_3A, professori[Lupin], 5, professori[GVermilinguo], 3);
    set_classe_insegnante_ore(Classe_3A, professori[Brook], 4);
    set_classe_insegnante_ore_cod(Classe_3A, professori[Ash], 4, professori[Theoden], 2);
    set_classe_insegnante_ore_cod(Classe_3A, professori[EJaeger], 4, professori[Denethor], 2);
    set_classe_insegnante_ore(Classe_3A, professori[RWeasley], 1);
    set_classe_insegnante_ore_cod(Classe_3A, professori[Mufasa], 3, professori[MUchiha], 3);

    set_classe_insegnante_ore_cod(Classe_3B, professori[Mufasa], 5, professori[Radagast], 4);
    set_classe_insegnante_ore(Classe_3B, professori[Simba], 6);
    set_classe_insegnante_ore_cod(Classe_3B, professori[Nala], 4, professori[Theoden], 2);
    set_classe_insegnante_ore(Classe_3B, professori[DMalfoy], 2);
    set_classe_insegnante_ore_cod(Classe_3B, professori[EJaeger], 3, professori[GVermilinguo], 3);
    set_classe_insegnante_ore(Classe_3B, professori[RWeasley], 1);
    set_classe_insegnante_ore_cod(Classe_3B, professori[Sarabi], 4, professori[MTartarughe], 1);
    set_classe_insegnante_ore_cod(Classe_3B, professori[Ash], 4, professori[Denethor], 2);
    set_classe_insegnante_ore(Classe_3B, professori[Pumba], 3);


    set_classe_insegnante_ore(Classe_3C, professori[Goten], 3);
    set_classe_insegnante_ore(Classe_3C, professori[Trunks], 6);
    set_classe_insegnante_ore_cod(Classe_3C, professori[Timon], 5, professori[Jesus], 2);
    set_classe_insegnante_ore_cod(Classe_3C, professori[Taipei], 4, professori[MTartarughe], 1);
    set_classe_insegnante_ore_cod(Classe_3C, professori[Zazu], 3, professori[Feanor], 2);
    set_classe_insegnante_ore(Classe_3C, professori[Doraemon], 1);
    set_classe_insegnante_ore(Classe_3C, professori[Gohan], 2);
    set_classe_insegnante_ore_cod(Classe_3C, professori[Rafiki], 4, professori[Gothmog], 2);
    set_classe_insegnante_ore_cod(Classe_3C, professori[Jasmine], 4, professori[Morgoth], 2);

    set_classe_insegnante_ore_cod(Classe_3D, professori[Mammolo], 5, professori[Morgoth], 2);
    set_classe_insegnante_ore_cod(Classe_3D, professori[Sauron], 4, professori[Gothmog], 2);
    set_classe_insegnante_ore_cod(Classe_3D, professori[Zazu], 3, professori[Feanor], 2);
    set_classe_insegnante_ore_cod(Classe_3D, professori[Aladino], 4, professori[Sarabi], 1);
    set_classe_insegnante_ore(Classe_3D, professori[Biancaneve], 3);
    set_classe_insegnante_ore(Classe_3D, professori[Dotto], 6);
    set_classe_insegnante_ore(Classe_3D, professori[Doraemon], 1);
    set_classe_insegnante_ore_cod(Classe_3D, professori[Pisolo], 4, professori[Gothmog], 2);
    set_classe_insegnante_ore(Classe_3D, professori[Arwen], 2);

    set_classe_insegnante_ore(Classe_3E, professori[Jiraya], 4);
    set_classe_insegnante_ore(Classe_3E, professori[Orochimaru], 6);
    set_classe_insegnante_ore_cod(Classe_3E, professori[Naruto], 3, professori[Freezer], 1);
    set_classe_insegnante_ore_cod(Classe_3E, professori[Cell], 6, professori[Sasuke], 3);
    set_classe_insegnante_ore(Classe_3E, professori[Gohan], 2);
    set_classe_insegnante_ore_cod(Classe_3E, professori[Sandokan], 3, professori[Jesus], 2);
    set_classe_insegnante_ore(Classe_3E, professori[Doraemon], 1);
    set_classe_insegnante_ore_cod(Classe_3E, professori[Vegeta], 4, professori[Freezer], 2);
    set_classe_insegnante_ore(Classe_3E, professori[Tsunade], 3); //130 KHz

    set_classe_insegnante_ore(Classe_3F, professori[MTartarughe], 4);
    set_classe_insegnante_ore(Classe_3F, professori[Sakura], 6);
    set_classe_insegnante_ore_cod(Classe_3F, professori[Goku], 3, professori[MBu], 1);
    set_classe_insegnante_ore_cod(Classe_3F, professori[Chichi], 6, professori[Bulma], 3);
    set_classe_insegnante_ore(Classe_3F, professori[Gohan], 2);
    set_classe_insegnante_ore_cod(Classe_3F, professori[Sandokan], 3, professori[Jesus], 2);
    set_classe_insegnante_ore(Classe_3F, professori[Doraemon], 1);
    set_classe_insegnante_ore_cod(Classe_3F, professori[Vegeta], 4, professori[Freezer], 2);
    set_classe_insegnante_ore(Classe_3F, professori[HPotter], 3); //91 KHz

    set_classe_insegnante_ore(Classe_3G, professori[BBaggins], 2);
    set_classe_insegnante_ore(Classe_3G, professori[Lupin], 5);
    set_classe_insegnante_ore_cod(Classe_3G, professori[SGamgee], 4, professori[Morgoth], 2);
    set_classe_insegnante_ore(Classe_3G, professori[HGranger], 6);
    set_classe_insegnante_ore_cod(Classe_3G, professori[Pipino], 3, professori[Sarabi], 2);
    set_classe_insegnante_ore_cod(Classe_3G, professori[Merry], 4, professori[Denethor], 3);
    set_classe_insegnante_ore(Classe_3G, professori[RWeasley], 1);
    set_classe_insegnante_ore(Classe_3G, professori[Elrond], 5);
    set_classe_insegnante_ore(Classe_3G, professori[Arwen], 2); //65 KHz

    set_classe_insegnante_ore(Classe_3K, professori[Saruman], 4);
    set_classe_insegnante_ore(Classe_3K, professori[Gandalf], 4);
    set_classe_insegnante_ore(Classe_3K, professori[Legolas], 6);
    set_classe_insegnante_ore(Classe_3K, professori[Aragorn], 3);
    set_classe_insegnante_ore(Classe_3K, professori[FBaggins], 6);
    set_classe_insegnante_ore(Classe_3K, professori[Arwen], 2);
    set_classe_insegnante_ore(Classe_3K, professori[Gimli], 2);
    set_classe_insegnante_ore(Classe_3K, professori[Glorfindel], 4);
    set_classe_insegnante_ore(Classe_3K, professori[Galadriel], 1); //440 Hz

    set_classe_insegnante_ore_cod(Classe_3H, professori[Taipei], 4, professori[MTartarughe], 1);
    set_classe_insegnante_ore(Classe_3H, professori[Trunks], 6);
    set_classe_insegnante_ore_cod(Classe_3H, professori[Crillin], 3, professori[MBu], 1);
    set_classe_insegnante_ore_cod(Classe_3H, professori[Vegeta], 6, professori[Gogeta], 3);
    set_classe_insegnante_ore(Classe_3H, professori[Gohan], 2);
    set_classe_insegnante_ore_cod(Classe_3H, professori[Sandokan], 3, professori[Jesus], 2);
    set_classe_insegnante_ore(Classe_3H, professori[Doraemon], 1);
    set_classe_insegnante_ore_cod(Classe_3H, professori[Vegeta], 4, professori[Janemba], 2);
    set_classe_insegnante_ore(Classe_3H, professori[Goten], 3); //38 KHz

    set_classe_insegnante_ore(Classe_3Q, professori[Haldir], 6);
    set_classe_insegnante_ore_cod(Classe_3Q, professori[Mammolo], 7, professori[Gothmog], 4);
    set_classe_insegnante_ore_cod(Classe_3Q, professori[Zazu], 4, professori[Gothmog], 2);
    set_classe_insegnante_ore_cod(Classe_3Q, professori[Eomer], 6, professori[Fingolfin], 2);
    set_classe_insegnante_ore(Classe_3Q, professori[Eowyn], 5);
    set_classe_insegnante_ore(Classe_3Q, professori[Celeborn], 4);
    set_classe_insegnante_ore(Classe_3Q, professori[Lurtz], 2);
    set_classe_insegnante_ore(Classe_3Q, professori[Galadriel], 1); //28 KHz

    set_classe_insegnante_ore(Classe_4A, professori[Simba], 6);
    set_classe_insegnante_ore_cod(Classe_4A, professori[TBombadil], 3, professori[Theoden], 2);
    set_classe_insegnante_ore_cod(Classe_4A, professori[TBombadil], 4, professori[Denethor], 2);
    set_classe_insegnante_ore(Classe_4A, professori[Voldemort], 3);
    set_classe_insegnante_ore_cod(Classe_4A, professori[EJaeger], 5, professori[MUchiha], 5);
    set_classe_insegnante_ore(Classe_4A, professori[Saruman], 4);
    set_classe_insegnante_ore(Classe_4A, professori[Merry], 4);
    set_classe_insegnante_ore(Classe_4A, professori[RWeasley], 1);
    set_classe_insegnante_ore(Classe_4A, professori[DMalfoy], 2); //20 KHz

    set_classe_insegnante_ore(Classe_4B, professori[Boromir], 4);
    set_classe_insegnante_ore_cod(Classe_4B, professori[TBombadil], 3, professori[Theoden], 2);
    set_classe_insegnante_ore(Classe_4B, professori[Lupin], 4);
    set_classe_insegnante_ore(Classe_4B, professori[Orochimaru], 6);
    set_classe_insegnante_ore_cod(Classe_4B, professori[Nala], 5, professori[Radagast], 5);
    set_classe_insegnante_ore_cod(Classe_4B, professori[Ash], 4, professori[Denethor], 2);
    set_classe_insegnante_ore(Classe_4B, professori[Pumba], 3);
    set_classe_insegnante_ore(Classe_4B, professori[RWeasley], 1);
    set_classe_insegnante_ore(Classe_4B, professori[DMalfoy], 2); //23 Khz

    set_classe_insegnante_ore(Classe_4C, professori[Pipino], 4);
    set_classe_insegnante_ore(Classe_4C, professori[Faramir], 6);
    set_classe_insegnante_ore(Classe_4C, professori[Biancaneve], 3);
    set_classe_insegnante_ore(Classe_4C, professori[Rafiki], 5);
    set_classe_insegnante_ore(Classe_4C, professori[Jasmine], 5);
    set_classe_insegnante_ore(Classe_4C, professori[Barbalbero], 6);
    set_classe_insegnante_ore(Classe_4C, professori[Galadriel], 1);
    set_classe_insegnante_ore(Classe_4C, professori[Gohan], 2); //20 KHz

    set_classe_insegnante_ore(Classe_4D, professori[Rafiki], 6);
    set_classe_insegnante_ore(Classe_4D, professori[Boromir], 3);
    set_classe_insegnante_ore(Classe_4D, professori[Aladino], 4);
    set_classe_insegnante_ore(Classe_4D, professori[SGamgee], 5);
    set_classe_insegnante_ore(Classe_4D, professori[Isildur], 6);
    set_classe_insegnante_ore(Classe_4D, professori[Zazu], 5);
    set_classe_insegnante_ore(Classe_4D, professori[RWeasley], 1);
    set_classe_insegnante_ore(Classe_4D, professori[Gohan], 2); //18 KHz


    set_classe_insegnante_ore(Classe_4E, professori[Jiraya], 4);
    set_classe_insegnante_ore(Classe_4E, professori[HGranger], 6);
    set_classe_insegnante_ore(Classe_4E, professori[Naruto], 7);
    set_classe_insegnante_ore(Classe_4E, professori[Cell], 6);
    set_classe_insegnante_ore(Classe_4E, professori[MCGranitt], 2);
    set_classe_insegnante_ore(Classe_4E, professori[Sandokan], 3);
    set_classe_insegnante_ore(Classe_4E, professori[Doraemon], 1);
    set_classe_insegnante_ore(Classe_4E, professori[Tsunade], 3);

    set_classe_insegnante_ore(Classe_5E, professori[Jiraya], 3);
    set_classe_insegnante_ore(Classe_5E, professori[Orochimaru], 6);
    set_classe_insegnante_ore(Classe_5E, professori[Naruto], 4);
    set_classe_insegnante_ore(Classe_5E, professori[Cell], 6);
    set_classe_insegnante_ore(Classe_5E, professori[Gohan], 2);
    set_classe_insegnante_ore(Classe_5E, professori[ASilente], 4);
    set_classe_insegnante_ore(Classe_5E, professori[Hagrid], 3);
    set_classe_insegnante_ore(Classe_5E, professori[Tsunade], 3);
    set_classe_insegnante_ore(Classe_5E, professori[Doraemon], 1);


    // VINCOLI SU ORA DI ENTRATA
    ora_di_entrata_prof[p_insegnante(professori[Naruto])] = 1;
    ora_di_entrata_prof[p_insegnante(professori[Cell])] = 1;
    ora_di_entrata_prof[p_insegnante(professori[Vegeta])] = 2;


    for (int i = 0; i < actual_classi_sezione; i++) {
        assert(ore_classi[i] <= actual_giorni_settimana * actual_ore_giornaliere);
        assert(ore_classi[i] >= PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
    }
}
