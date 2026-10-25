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

    set_classe_insegnante_ore(Classe_3A, BanderaS, 2);
    set_classe_insegnante_ore(Classe_3A, TosadoriF, 3);
    set_classe_insegnante_ore(Classe_3A, BosioA, 6);
    set_classe_insegnante_ore_cod(Classe_3A, CapuanoM, 5, MaioneA, 3);
    set_classe_insegnante_ore(Classe_3A, MariniA, 4);
    set_classe_insegnante_ore_cod(Classe_3A, RovettaR, 4, CalziD, 2);
    set_classe_insegnante_ore_cod(Classe_3A, GuidiA, 4, FierravantiD, 2);
    set_classe_insegnante_ore(Classe_3A, Marchione, 1);
    set_classe_insegnante_ore_cod(Classe_3A, BeltramiA, 3, SantonicolaA, 3);

    set_classe_insegnante_ore_cod(Classe_3B, BeltramiA, 5, LorenzoniL, 4);
    set_classe_insegnante_ore(Classe_3B, SperandeoF, 6);
    set_classe_insegnante_ore_cod(Classe_3B, RossiP, 4, CalziD, 2);
    set_classe_insegnante_ore(Classe_3B, BanderaS, 2);
    set_classe_insegnante_ore_cod(Classe_3B, GuidiA, 3, MaioneA, 3);
    set_classe_insegnante_ore(Classe_3B, Marchione, 1);
    set_classe_insegnante_ore_cod(Classe_3B, SpillerC, 4, Sannino, 1);
    set_classe_insegnante_ore_cod(Classe_3B, RovettaR, 4, FierravantiD, 2);
    set_classe_insegnante_ore(Classe_3B, BurattiI, 3);


    set_classe_insegnante_ore(Classe_3C, LancellottiO, 3);
    set_classe_insegnante_ore(Classe_3C, Guerra, 6);
    set_classe_insegnante_ore_cod(Classe_3C, MariniE, 5, Avolio, 2);
    set_classe_insegnante_ore_cod(Classe_3C, Tosi, 4, Sannino, 1);
    set_classe_insegnante_ore_cod(Classe_3C, FuscoG, 3, StranoS, 2);
    set_classe_insegnante_ore(Classe_3C, Quaini, 1);
    set_classe_insegnante_ore(Classe_3C, Nicoletti, 2);
    set_classe_insegnante_ore_cod(Classe_3C, GelminiE, 4, DelCornoE, 2);
    set_classe_insegnante_ore_cod(Classe_3C, PerghemE, 4, PasqualiniF, 2);

    set_classe_insegnante_ore_cod(Classe_3D, IncorvaiaL, 5, PasqualiniF, 2);
    set_classe_insegnante_ore_cod(Classe_3D, PerliniM, 4, DelCornoE, 2);
    set_classe_insegnante_ore_cod(Classe_3D, FuscoG, 3, StranoS, 2);
    set_classe_insegnante_ore_cod(Classe_3D, AvinoG, 4, SpillerC, 1);
    set_classe_insegnante_ore(Classe_3D, BorghettiF, 3);
    set_classe_insegnante_ore(Classe_3D, LoMeleC, 6);
    set_classe_insegnante_ore(Classe_3D, Quaini, 1);
    set_classe_insegnante_ore_cod(Classe_3D, YuC, 4, DelCornoE, 2);
    set_classe_insegnante_ore(Classe_3D, PisatiM, 2);

    set_classe_insegnante_ore(Classe_3E, Berno, 4);
    set_classe_insegnante_ore(Classe_3E, DalZovo, 6);
    set_classe_insegnante_ore_cod(Classe_3E, Sciambarella, 3, Barosi, 1);
    set_classe_insegnante_ore_cod(Classe_3E, Masetti, 6, Moretti, 3);
    set_classe_insegnante_ore(Classe_3E, Nicoletti, 2);
    set_classe_insegnante_ore_cod(Classe_3E, Trivini, 3, Avolio, 2);
    set_classe_insegnante_ore(Classe_3E, Quaini, 1);
    set_classe_insegnante_ore_cod(Classe_3E, Tiraboschi, 4, Barosi, 2);
    set_classe_insegnante_ore(Classe_3E, Alves, 3); //130 KHz

    set_classe_insegnante_ore(Classe_3F, Sannino, 4);
    set_classe_insegnante_ore(Classe_3F, Bortolotti, 6);
    set_classe_insegnante_ore_cod(Classe_3F, Ricciardi, 3, Rizzi, 1);
    set_classe_insegnante_ore_cod(Classe_3F, Kushi, 6, Paghera, 3);
    set_classe_insegnante_ore(Classe_3F, Nicoletti, 2);
    set_classe_insegnante_ore_cod(Classe_3F, Trivini, 3, Avolio, 2);
    set_classe_insegnante_ore(Classe_3F, Quaini, 1);
    set_classe_insegnante_ore_cod(Classe_3F, Tiraboschi, 4, Barosi, 2);
    set_classe_insegnante_ore(Classe_3F, Comelli, 3); //91 KHz

    set_classe_insegnante_ore(Classe_3G, MariniAlice, 2);
    set_classe_insegnante_ore(Classe_3G, CapuanoM, 5);
    set_classe_insegnante_ore_cod(Classe_3G, LavelliA, 4, PasqualiniF, 2);
    set_classe_insegnante_ore(Classe_3G, Pizzatti, 6);
    set_classe_insegnante_ore_cod(Classe_3G, RossiJ, 3, SpillerC, 2);
    set_classe_insegnante_ore_cod(Classe_3G, OrioS, 4, FierravantiD, 3);
    set_classe_insegnante_ore(Classe_3G, Marchione, 1);
    set_classe_insegnante_ore(Classe_3G, NegriL, 5);
    set_classe_insegnante_ore(Classe_3G, PisatiM, 2); //65 KHz

    set_classe_insegnante_ore(Classe_3K, DorinP, 4);
    set_classe_insegnante_ore(Classe_3K, ViglioneL, 4);
    set_classe_insegnante_ore(Classe_3K, BottarelliF, 6);
    set_classe_insegnante_ore(Classe_3K, FalconeS, 3);
    set_classe_insegnante_ore(Classe_3K, CarinoA, 6);
    set_classe_insegnante_ore(Classe_3K, PisatiM, 2);
    set_classe_insegnante_ore(Classe_3K, ManessiF, 2);
    set_classe_insegnante_ore(Classe_3K, GrossoC, 4);
    set_classe_insegnante_ore(Classe_3K, PeressuttiPL, 1); //440 Hz

    set_classe_insegnante_ore_cod(Classe_3H, Tosi, 4, Sannino, 1);
    set_classe_insegnante_ore(Classe_3H, Guerra, 6);
    set_classe_insegnante_ore_cod(Classe_3H, Manganelli, 3, Rizzi, 1);
    set_classe_insegnante_ore_cod(Classe_3H, Tiraboschi, 6, Benedetto, 3);
    set_classe_insegnante_ore(Classe_3H, Nicoletti, 2);
    set_classe_insegnante_ore_cod(Classe_3H, Trivini, 3, Avolio, 2);
    set_classe_insegnante_ore(Classe_3H, Quaini, 1);
    set_classe_insegnante_ore_cod(Classe_3H, Tiraboschi, 4, Gagliano, 2);
    set_classe_insegnante_ore(Classe_3H, LancellottiO, 3); //38 KHz

    set_classe_insegnante_ore(Classe_3Q, CresciniG, 6);
    set_classe_insegnante_ore_cod(Classe_3Q, IncorvaiaL, 7, DelCornoE, 4);
    set_classe_insegnante_ore_cod(Classe_3Q, FuscoG, 4, DelCornoE, 2);
    set_classe_insegnante_ore_cod(Classe_3Q, TonoliE, 6, SpacagnoPa, 2);
    set_classe_insegnante_ore(Classe_3Q, ZaniE, 5);
    set_classe_insegnante_ore(Classe_3Q, CultraroMR, 4);
    set_classe_insegnante_ore(Classe_3Q, BoschettiS, 2);
    set_classe_insegnante_ore(Classe_3Q, PeressuttiPL, 1); //28 KHz

    set_classe_insegnante_ore(Classe_4A, SperandeoF, 6);
    set_classe_insegnante_ore_cod(Classe_4A, MenapaceP, 3, CalziD, 2);
    set_classe_insegnante_ore_cod(Classe_4A, MenapaceP, 4, FierravantiD, 2);
    set_classe_insegnante_ore(Classe_4A, TosadoriF, 3);
    set_classe_insegnante_ore_cod(Classe_4A, GuidiA, 5, SantonicolaA, 5);
    set_classe_insegnante_ore(Classe_4A, DorinP, 4);
    set_classe_insegnante_ore(Classe_4A, OrioS, 4);
    set_classe_insegnante_ore(Classe_4A, Marchione, 1);
    set_classe_insegnante_ore(Classe_4A, BanderaS, 2); //20 KHz

    set_classe_insegnante_ore(Classe_4B, CavallariM, 4);
    set_classe_insegnante_ore_cod(Classe_4B, MenapaceP, 3, CalziD, 2);
    set_classe_insegnante_ore(Classe_4B, CapuanoM, 4);
    set_classe_insegnante_ore(Classe_4B, DalZovo, 6);
    set_classe_insegnante_ore_cod(Classe_4B, RossiP, 5, LorenzoniL, 5);
    set_classe_insegnante_ore_cod(Classe_4B, RovettaR, 4, FierravantiD, 2);
    set_classe_insegnante_ore(Classe_4B, BurattiI, 3);
    set_classe_insegnante_ore(Classe_4B, Marchione, 1);
    set_classe_insegnante_ore(Classe_4B, BanderaS, 2); //23 Khz

    set_classe_insegnante_ore(Classe_4C, RossiJ, 4);
    set_classe_insegnante_ore(Classe_4C, VivaG, 6);
    set_classe_insegnante_ore(Classe_4C, BorghettiF, 3);
    set_classe_insegnante_ore(Classe_4C, GelminiE, 5);
    set_classe_insegnante_ore(Classe_4C, PerghemE, 5);
    set_classe_insegnante_ore(Classe_4C, StefaniniR, 6);
    set_classe_insegnante_ore(Classe_4C, PeressuttiPL, 1);
    set_classe_insegnante_ore(Classe_4C, Nicoletti, 2); //20 KHz

    set_classe_insegnante_ore(Classe_4D, GelminiE, 6);
    set_classe_insegnante_ore(Classe_4D, CavallariM, 3);
    set_classe_insegnante_ore(Classe_4D, AvinoG, 4);
    set_classe_insegnante_ore(Classe_4D, LavelliA, 5);
    set_classe_insegnante_ore(Classe_4D, MarchiD, 6);
    set_classe_insegnante_ore(Classe_4D, FuscoG, 5);
    set_classe_insegnante_ore(Classe_4D, Marchione, 1);
    set_classe_insegnante_ore(Classe_4D, Nicoletti, 2); //18 KHz


    set_classe_insegnante_ore(Classe_4E, Berno, 4);
    set_classe_insegnante_ore(Classe_4E, Pizzatti, 6);
    set_classe_insegnante_ore(Classe_4E, Sciambarella, 7);
    set_classe_insegnante_ore(Classe_4E, Masetti, 6);
    set_classe_insegnante_ore(Classe_4E, Torbol, 2);
    set_classe_insegnante_ore(Classe_4E, Trivini, 3);
    set_classe_insegnante_ore(Classe_4E, Quaini, 1);
    set_classe_insegnante_ore(Classe_4E, Alves, 3);

    set_classe_insegnante_ore(Classe_5E, Berno, 3);
    set_classe_insegnante_ore(Classe_5E, DalZovo, 6);
    set_classe_insegnante_ore(Classe_5E, Sciambarella, 4);
    set_classe_insegnante_ore(Classe_5E, Masetti, 6);
    set_classe_insegnante_ore(Classe_5E, Nicoletti, 2);
    set_classe_insegnante_ore(Classe_5E, Caniato, 4);
    set_classe_insegnante_ore(Classe_5E, Cerini, 3);
    set_classe_insegnante_ore(Classe_5E, Alves, 3);
    set_classe_insegnante_ore(Classe_5E, Quaini, 1);


    // VINCOLI SU ORA DI ENTRATA
    ora_di_entrata_prof[p_insegnante(Sciambarella)] = 1;
    ora_di_entrata_prof[p_insegnante(Masetti)] = 1;
    ora_di_entrata_prof[p_insegnante(Tiraboschi)] = 1;


    for (int i = 0; i < actual_classi_sezione; i++) {
        assert(ore_classi[i] <= actual_giorni_settimana * actual_ore_giornaliere);
        assert(ore_classi[i] >= PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana);
    }
}
