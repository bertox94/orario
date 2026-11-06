// Le funzioni e lo stato della ricerca che i file condivisi (la traccia, le potature, il controllo in avanti, la prova
// di ingresso_uscita/prova.c) chiamano per nome. Le definisce ogni solver nel suo main.c (main e i solver di
// solutori/, quasi tutte con controlli/comuni.h): ogni solver ha le sue, e i file condivisi usano quelle del solver con
// cui sono compilati. Qui ci sono solo le dichiarazioni, una volta per tutti.
#ifndef CONTROLLI_H
#define CONTROLLI_H

#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// ---- lo stato ----
extern teaching_slot orario[];   // una cella per (giorno, ora, classe), con i due docenti (-1: nessuno)
extern int orario_len;           // quante celle ha l'orario (giorni x ore x classi)
extern long long int n_gen;      // orari completi trovati
extern long long int n_gen_max;  // quanti cercarne al massimo (-n)

// ---- le celle: pos = (giorno * ore + ora) * classi + classe ----
int g_giorno(int pos);
int g_ora(int pos);
int g_classe(int pos);

// ---- i controlli della ricerca (vedi il main.c del solver) ----
int activities_for_this_class_and_hour(int pos, int gio, int ora, int cla);
int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2);
int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof);
int prof_è_disponibile(int base3d_goi, int p1);
int deve_ancora_svolgere_ore_in_questa_classe(int base2d_ci, int p1);
int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof);
int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof);
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, int prof);
int verifica_ora_ingresso(int prof, int ora);
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(int cla, int p1, int p2);
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(int base2d_ci, int prof);

// ---- mettere un docente (o una coppia) nella cella ----
void prep_chiamata_ricorsiva(int base2d_ci, int base3d_goi, QueueItem *it, int giorno, int cla, int prof);
void prep_chiamata_ricorsiva_insieme(int base2d_ci, int base3d_goi, int cla, QueueItem *it, QueueItem *jt, int giorno);

#endif
