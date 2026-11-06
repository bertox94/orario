#include "prova.h"

#include "../controlli/controlli.h"  // mettere un docente nella cella: come fa il solver

static int idx_oggi(const int giorno, const int cla, const int p) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + p;
}

Prova metti_per_prova(const int pos, const int giorno, const int cla, const int base2d_ci, const int base3d_goi,
                      QueueItem *it, QueueItem *jt) {
    Prova s;
    QueueItem *q[2] = {it, jt};
    for (int z = 0; z < 2; z++) {
        if (!q[z]) continue;
        const int p = q[z]->teacher;
        s.priorita[z] = q[z]->priority;
        s.impegnato[z] = prof_gia_impegnato_giorno_ora[base3d_goi + p];
        s.svolte[z] = ore_classi_docenti_svolte[base2d_ci + p];
        s.oggi[z] = ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, p)];
    }
    s.solo = ore_classi_docenti_alone_svolte[base2d_ci + it->teacher];
    s.insieme = jt ? get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, it->teacher, jt->teacher) : 0;
    s.cella = orario[pos];
    if (jt) prep_chiamata_ricorsiva_insieme(base2d_ci, base3d_goi, cla, it, jt, giorno);
    else prep_chiamata_ricorsiva(base2d_ci, base3d_goi, it, giorno, cla, it->teacher);
    orario[pos] = (teaching_slot){it->teacher, jt ? jt->teacher : -1};
    return s;
}

void togli_prova(const Prova *s, const int pos, const int giorno, const int cla, const int base2d_ci,
                 const int base3d_goi, QueueItem *it, QueueItem *jt) {
    QueueItem *q[2] = {it, jt};
    for (int z = 0; z < 2; z++) {
        if (!q[z]) continue;
        const int p = q[z]->teacher;
        q[z]->priority = s->priorita[z];
        prof_gia_impegnato_giorno_ora[base3d_goi + p] = s->impegnato[z];
        ore_classi_docenti_svolte[base2d_ci + p] = s->svolte[z];
        ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_oggi(giorno, cla, p)] = s->oggi[z];
    }
    ore_classi_docenti_alone_svolte[base2d_ci + it->teacher] = s->solo;
    if (jt) {
        while (get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, it->teacher, jt->teacher) > s->insieme)
            decrement_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
        while (get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, it->teacher, jt->teacher) < s->insieme)
            increment_ore_classi_codocenza_svolte(cla, it->teacher, jt->teacher);
    }
    orario[pos] = s->cella;
}
