//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H

#define MAX_STR_SZ_IN_DISPLAY 18
#include <stdio.h>

#include "../data_initialization/database_initializers.h"

#include "../orario/orario.h"

void print(const teaching_slot orario[], const int pos, unsigned long long attempt, FILE *f);

// riepilogo dei dati su stdout
void stampa_dati(void);

// descrizione completa dei dati (classi, docenti, lezioni da soli e in codocenza) su un file
void stampa_dati_su_file(FILE *fp);
#endif //UNTITLED_PRINTER_H
