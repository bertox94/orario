// Il programma: lancia la ricerca dell'orario (ricerca.c) su casi casuali, per le prove e per il sito.
//
// ./untitled test N    le prove con i dati casuali (prove.h)
// ./untitled caso SEME CLASSI RITARDI LIMITE FILE PASSI     un caso casuale per il sito (caso.h)
// ./untitled dati SEME CLASSI RITARDI                         i dati di quel caso in JSON (generatore.h)
// ./untitled passo SEME CLASSI RITARDI PASSI K               il nodo K di quel caso: orario e controlli (passo.h)
// Senza comando, o con un comando che non esiste, stampa questo elenco ed esce con 2.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prove.h"
#include "caso.h"
#include "passo.h"
#include "data_initialization/generatore.h"

int main(const int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "test")) {  // le prove con i dati casuali
        if (argc < 3) {
            fprintf(stderr, "Uso: %s test N\n", argv[0]);
            return 2;
        }
        esegui_prove(atoi(argv[2]));
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "caso")) {  // un caso casuale seguito dal sito
        if (argc < 8) {
            fprintf(stderr, "Uso: %s caso SEME CLASSI RITARDI LIMITE FILE PASSI\n", argv[0]);
            return 2;
        }
        return esegui_caso(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]), atoll(argv[5]), argv[6],
                           argv[7]);
    }
    if (argc > 1 && !strcmp(argv[1], "passo")) {  // un nodo della ricerca: orario e controlli, per il sito
        if (argc < 7) {
            fprintf(stderr, "Uso: %s passo SEME CLASSI RITARDI PASSI K\n", argv[0]);
            return 2;
        }
        return stampa_passo(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]), argv[5], atoll(argv[6]));
    }
    if (argc > 1 && !strcmp(argv[1], "dati")) {  // i dati di un caso, per il sito
        if (argc < 5) {
            fprintf(stderr, "Uso: %s dati SEME CLASSI RITARDI\n", argv[0]);
            return 2;
        }
        return !stampa_dati_casuali(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]));
    }
    // nessun comando, o un comando che non esiste: l'elenco dei comandi
    if (argc > 1) fprintf(stderr, "Comando sconosciuto: %s\n", argv[1]);
    fprintf(stderr, "Uso:\n"
            "  %s test N                                         le prove con i dati casuali\n"
            "  %s caso SEME CLASSI RITARDI LIMITE FILE PASSI     un caso casuale, per il sito\n"
            "  %s dati SEME CLASSI RITARDI                       i dati di quel caso\n"
            "  %s passo SEME CLASSI RITARDI PASSI K              il nodo K di quel caso\n",
            argv[0], argv[0], argv[0], argv[0]);
    return 2;
}
