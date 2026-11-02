//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H

// Stampa dell'orario (griglia classi x giorni/ore) e dei dati.

#define MAX_STR_SZ_IN_DISPLAY 18 // larghezza di una cella della griglia (nomi piu' lunghi vengono tagliati)
#include <stdio.h>

#include "../data_initialization/database_initializers.h"

#include "../orario/orario.h"

// Scrive in f l'orario con gli slot da 0 a pos - 1 (gli altri compaiono vuoti, "--"), poi per ogni classe le ore
// svolte di ogni docente. attempt e' il numero d'ordine dell'orario completo. Si usa per gli orari completi e,
// con -b, per l'orario parziale a ogni marcia indietro.
void print(const teaching_slot orario[], const int pos, unsigned long long attempt, FILE *f);

// riepilogo dei dati su stdout
void stampa_dati(void);

// descrizione completa dei dati (classi, docenti, lezioni da soli e in codocenza) su un file
void stampa_dati_su_file(FILE *fp);
#endif //UNTITLED_PRINTER_H
