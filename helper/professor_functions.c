#include "../data_initialization/database_initializers.h"
//
// Created by Alberto Tiraboschi on 09/06/2026.
//
int is_vuoto(const int p) { return p == -1; }

int is_docente(const int p) { return !is_vuoto(p) && !codocenti[p]; }

int is_codocente(const int p) { return !is_vuoto(p) && codocenti[p]; }