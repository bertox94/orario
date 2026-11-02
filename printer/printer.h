//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H

#define MAX_STR_SZ_IN_DISPLAY 18
#include <stdio.h>

#include "../data_initialization/database_initializers.h"

// indice in orario[] dello slot di (giorno, ora, classe); definita in main.c
int slot_di(int giorno, int ora, int classe);

void print(const teaching_slot orario[], const int pos, unsigned long long attempt, FILE *f);
#endif //UNTITLED_PRINTER_H
