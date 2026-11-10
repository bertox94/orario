#ifndef UNTITLED_VINCOLI_H
#define UNTITLED_VINCOLI_H

// I vincoli della ricerca (definiti in ricerca.c), per chi li vuole valutare uno per uno: passo.c, che per il
// sito dice il valore di ogni controllo in una cella. La ricerca usa le stesse funzioni.

#include <stddef.h>

#include "priority_queue/prio_queue.h"

// La cella che si sta decidendo, con gli indici di partenza negli array piatti che la riguardano.
typedef struct {
    int pos, giorno, ora, cla;
    int base2d_ci;     // classe, docente: ore_classi_docenti[base2d_ci + p] e simili
    int base3d_goi;    // giorno, ora, docente: prof_gia_impegnato_giorno_ora[base3d_goi + p]
    int base3d_gci;    // giorno, classe, docente: ore_fatte_da_un_prof_in_una_classe_in_giorno[base3d_gci + p]
    int base4d_queue;  // la coda dei docenti di questa cella: queue_mul[base4d_queue ...]
} Cella;

Cella cella(int pos);
int g_giorno(int p);
int g_ora(int p);
int g_classe(int p);
// le ore che restano al docente p nella classe della cella
int ore_rimaste(const Cella *c, int p);

// la classe puo' avere lezione in questa cella
int activities_for_this_class_and_hour(const Cella *c);

// i controlli di un docente
int prof_è_disponibile(const Cella *c, int p);
int deve_ancora_svolgere_ore_in_questa_classe(const Cella *c, int p);
int prof_fa_al_max_n_ore(const Cella *c, int p);
int prof_fa_ore_contigue(const Cella *c, int p);
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(const Cella *c, int p);
int verifica_ora_ingresso(int p, int ora);
// in coppia, e da solo
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const Cella *c, int p1, int p2);
int sabato_esaurisci_priorita_subito_cod(const Cella *c, int prof1, int prof2);
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const Cella *c, int p);
int sabato_esaurisci_priorita_subito(const Cella *c, int prof);

// i tre gruppi, come li usa la ricerca
int docente_puo_insegnare(const Cella *c, int p1);
int secondo_della_coppia_puo_insegnare(const Cella *c, int p1, int p2);
int docente_puo_insegnare_da_solo(const Cella *c, int p1);

// la coda della cella in ordine, e mettere (delta 1) o togliere (-1) un'ora
void sort_queue_mul(size_t start, size_t n);
void segna_da_solo(const Cella *c, QueueItem *it, int delta);
void segna_coppia(const Cella *c, QueueItem *it, QueueItem *jt, int delta);

#endif //UNTITLED_VINCOLI_H
