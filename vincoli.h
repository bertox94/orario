#ifndef UNTITLED_VINCOLI_H
#define UNTITLED_VINCOLI_H

// I vincoli della ricerca (definiti in ricerca.c), per chi li vuole valutare uno per uno: passo.c, che per il
// sito dice il valore di ogni controllo in una cella. La ricerca usa le stesse funzioni.

#include "ricerca.h"

// La cella che si sta decidendo, con gli indici di partenza negli array piatti che la riguardano.
typedef struct {
    int pos, giorno, ora, cla;
    int base2d_ci;     // classe, docente: ore_classi_docenti[base2d_ci + p] e simili
    int base3d_goi;    // giorno, ora, docente: prof_gia_impegnato_giorno_ora[base3d_goi + p]
    int base2d_gc;     // giorno, classe: n_ore_buche[base2d_gc]
    int base3d_gci;    // giorno, classe, docente: ore_fatte_da_un_prof_in_una_classe_in_giorno[base3d_gci + p]
    QueueItem **coda;  // la coda di questa cella: i docenti della classe
    int n_coda;        // quanti sono
} Cella;

Cella cella(const Ricerca *r, int pos);
// le ore che restano al docente p nella classe della cella
int ore_rimaste(const Ricerca *r, const Cella *c, int p);

// i controlli della classe
// le ore che la classe deve ancora fare, contando questa cella
int ore_rimaste_alla_classe(const Ricerca *r, const Cella *c);
// le celle obbligatorie che la classe deve ancora riempire, contando questa
int celle_obbligatorie_rimaste(const Ricerca *r, const Cella *c);
// la classe puo' avere lezione in questa cella
int activities_for_this_class_and_hour(const Ricerca *r, const Cella *c);
// i posti che restano alla classe nei giorni dopo oggi
int posti_nei_giorni_dopo(const Ricerca *r, const Cella *c);
// la classe puo' avere un'ora buca in questa cella
int la_classe_puo_avere_ora_buca(const Ricerca *r, const Cella *c);
// lo stato della classe in questa cella: che cosa prova la ricerca (vedi ricerca.c)
typedef enum { DEVE_FARE_ATTIVITA, PUO_FARE_ATTIVITA, NON_PUO_FARE_ATTIVITA } StatoClasse;
StatoClasse stato_della_classe(const Ricerca *r, const Cella *c);

// i controlli di un docente
int prof_è_disponibile(const Ricerca *r, const Cella *c, int p);
int deve_ancora_svolgere_ore_in_questa_classe(const Ricerca *r, const Cella *c, int p);
int prof_fa_al_max_n_ore(const Ricerca *r, const Cella *c, int p);
int prof_fa_ore_contigue(const Ricerca *r, const Cella *c, int p);
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(const Ricerca *r, const Cella *c, int p);
int verifica_ora_ingresso(const Ricerca *r, int p, int ora);
// in coppia, e da solo
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const Ricerca *r, const Cella *c, int p1, int p2);
int sabato_esaurisci_priorita_subito_cod(const Ricerca *r, const Cella *c, int prof1, int prof2);
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const Ricerca *r, const Cella *c, int p);
int sabato_esaurisci_priorita_subito(const Ricerca *r, const Cella *c, int prof);

// i tre gruppi, come li usa la ricerca
int docente_puo_insegnare(const Ricerca *r, const Cella *c, int p1);
int secondo_della_coppia_puo_insegnare(const Ricerca *r, const Cella *c, int p1, int p2);
int docente_puo_insegnare_da_solo(const Ricerca *r, const Cella *c, int p1);

// mettere (delta 1) o togliere (-1) un'ora in questa cella: di un docente da solo, o di una coppia
void segna_da_solo(Ricerca *r, const Cella *c, int p, int delta);
void segna_coppia(Ricerca *r, const Cella *c, int p1, int p2, int delta);

#endif //UNTITLED_VINCOLI_H
