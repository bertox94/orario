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

// Marce indietro in formato compatto (opzione -b). Una riga per marcia indietro con solo gli slot cambiati rispetto
// alla marcia precedente: e' centinaia di volte piu' piccolo della griglia completa e permette comunque di ricostruire
// l'orario parziale di ogni marcia. Formato (descritto anche nell'intestazione del file):
//   B pos da s_da ... s_(pos-1)
// marcia indietro nello slot pos (nessun docente utilizzabile); gli slot da `da` a pos - 1 valgono come scritto, quelli
// prima di `da` come nella riga B precedente. Ogni slot e' "i" (docente di indice i), "i+j" (coppia) o "-" (vuoto).
// Le posizioni sono quelle della ricerca: pos = (giorno * ore + ora) * classi + k, con la classe k-esima dell'ordine
// scritto nell'intestazione.
// Con --passi N, una riga "P pos da s_da ... s_(pos-1)" (stesso formato delle righe B, stessa catena di differenze) a
// ogni nodo della ricerca: l'orario parziale con i primi pos slot decisi. Si scrivono al massimo N righe P, poi la
// ricerca continua senza registrarle. Se il nodo di un passo e' un ramo scartato dalla potatura, subito dopo la riga P
// c'e' una riga "S m" (m = motivo dello scarto, come l'indice in diagnosi_ramo_scartato: 0 lezioni della classe, 1 ore dei
// docenti); se invece non ha nessun docente utilizzabile, dopo la P c'e' la riga B della marcia indietro.
// Con --motivi, subito dopo ogni riga B c'e' una riga "M d:m d:m ...": per ogni docente d che nella classe aveva ancora
// ore da fare, il motivo m (come in "nomi_motivi" del JSON) per cui non si e' potuto mettere nello slot.
void stampa_intestazione_marce_compatte(FILE *f);

void stampa_marcia_compatta(FILE *f, const teaching_slot orario[], int pos);
void stampa_passo_compatto(FILE *f, const teaching_slot orario[], int pos);  // riga P di --passi, vedi sopra

// riepilogo dei dati su stdout
void stampa_dati(void);

// descrizione completa dei dati (classi, docenti, lezioni da soli e in codocenza) su un file
void stampa_dati_su_file(FILE *fp);
#endif //UNTITLED_PRINTER_H
