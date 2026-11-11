#ifndef UNTITLED_VERIFICA_ORARIO_H
#define UNTITLED_VERIFICA_ORARIO_H

#include "data_initialization/database_initializers.h"

// La verifica dell'orario trovato, indipendente dalla ricerca: non usa niente di ricerca.c (ne' i vincoli, ne' lo
// stato delle ore gia' messe). Legge solo i dati della scuola e l'orario finito, cella per cella, e rifa' i conti.
// Se la ricerca sbagliasse, qui si vedrebbe.
//
// L'orario: una cella per (giorno, ora, classe), la cella (giorno * ore + ora) * classi + classe; in ogni cella i due
// docenti (-1: nessuno; tutti e due -1: ora buca).
//
// I controlli, in quest'ordine:
//  0. le ore: in ogni classe ogni docente da solo e ogni coppia fanno esattamente le ore dei dati
//  1. un docente e' in una sola classe per volta
//  2. nessuna lezione prima dell'ora di ingresso del docente
//  3. un docente fa al massimo MAX_ORE_PROF_IN_UNA_CLASSE ore al giorno nella stessa classe
//  4. le ore di un docente in una classe, nello stesso giorno, sono di seguito
//  5. un docente insegna in al massimo MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi al giorno
//  6. le prime PRIME_N_ORE_OBBLIGATORIE ore di ogni giorno ogni classe ha lezione
//  7. dopo un'ora buca la classe quel giorno non ha piu' lezione
// (La regola del sabato della ricerca non e' una regola dell'orario: e' una conseguenza di queste.)

#define VERIFICA_ORARIO_CONTROLLI 8

typedef struct {
    const char *nome;     // che cosa controlla
    int ok;               // 1: l'orario lo rispetta
    char dettaglio[200];  // se no, la prima cosa che non va
} ControlloOrario;

// Riempie i VERIFICA_ORARIO_CONTROLLI controlli; 1 se l'orario li rispetta tutti.
int verifica_orario(const teaching_slot *orario, ControlloOrario controlli[VERIFICA_ORARIO_CONTROLLI]);

#endif //UNTITLED_VERIFICA_ORARIO_H
