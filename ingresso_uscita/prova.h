#ifndef UNTITLED_PROVA_H
#define UNTITLED_PROVA_H

// Mettere una coppia (o un docente da solo) nella cella per prova, per la traccia dei solver che hanno un controllo che
// guarda lo stato dopo la scelta (in-avanti). Si mette come fa la ricerca (prep_chiamata_ricorsiva
// [_insieme]) e poi si rimette tutto esattamente com'era, valore per valore. Non si usa post_chiamata_ricorsiva per
// togliere la prova: la prova si fa anche per docenti che non passano i controlli (per esempio gia' occupati in
// quest'ora) e post_... li renderebbe liberi. Il risultato vuol dire: "se la coppia si mettesse qui comunque".

#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"

typedef struct {
    double priorita[2];
    int impegnato[2], svolte[2], oggi[2], solo, insieme;
    teaching_slot cella;
} Prova;

// jt = NULL: il docente it da solo
Prova metti_per_prova(int pos, int giorno, int cla, int base2d_ci, int base3d_goi, QueueItem *it, QueueItem *jt);
void togli_prova(const Prova *s, int pos, int giorno, int cla, int base2d_ci, int base3d_goi, QueueItem *it,
                 QueueItem *jt);

#endif
