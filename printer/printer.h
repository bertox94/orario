#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H

#define MAX_STR_SZ_IN_DISPLAY 18
#include <stdio.h>

#include "../data_initialization/database_initializers.h"

// la griglia dell'orario fino alla cella pos (e le statistiche per classe) in f, come in orario.txt
void print(const teaching_slot orario[], int pos, unsigned long long attempt, FILE *f);
// i dati letti, su stdout
void stampa_dati(void);
#endif //UNTITLED_PRINTER_H
