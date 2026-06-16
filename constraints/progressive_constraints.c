//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#include <assert.h>
#include <stdio.h>

#include "../data_initialization/database_initializers.h"

int activities_for_this_class_and_hour(teaching_slot orario[], int pos, const int ora, const int cla,
                                       const int classe_finisce_tardi_num[]) {
    //se ti trovi ad avere già n classe con orari pieni
    const int giorni_pieni = (ore_classe_settimanali[cla] - PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana) %
                             actual_ore_giornaliere;

    //l'ultima ora non è permessa se è di un giorno che supera i giorni pieni, le altre prima possono,
    //quelle fino a obbligatorie devono
    if (ora == actual_ore_giornaliere - 1 && classe_finisce_tardi_num[cla] >= giorni_pieni)
        return 0;


    //sostituire con parametro senza usare orario
    if (pos - actual_classi_sezione >= 0) {
        if (pos / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana == (
                pos - actual_classi_sezione) / actual_classi_sezione / actual_ore_giornaliere % actual_giorni_settimana)
            return orario[pos - actual_classi_sezione].insegnante1 != -1
                   || orario[pos - actual_classi_sezione].insegnante2 != -1;
    }

    return 1;
}
