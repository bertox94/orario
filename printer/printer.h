//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H

#define MAX_STR_SZ_IN_DISPLAY 18
#include <stdio.h>

#include "../data_initialization/database_initializers.h"

// L'orario fino alla cella pos (esclusa) in f: una riga per classe con docente e codocente, giorni e ore in colonna, e
// per ogni classe le ore messe e quelle da fare di ogni docente. attempt: quanti orari completi trovati finora.
void print(const teaching_slot orario[], const int pos, unsigned long long attempt, FILE *f);
#endif //UNTITLED_PRINTER_H
