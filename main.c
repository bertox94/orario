// Il programma: lancia la ricerca dell'orario (ricerca.c) su casi casuali, per le prove e per il sito.
//
// ./untitled test N [ORDINI LIMITE]   le prove con i dati casuali (prove.h)
// ./untitled caso SEME CLASSI RITARDI ORDINE LIMITE FILE PASSI   un caso casuale per il sito (caso.h)
// ./untitled dati SEME CLASSI RITARDI ORDINE                       i dati di quel caso in JSON (generatore.h)
// ./untitled passo SEME CLASSI RITARDI ORDINE PASSI K             il nodo K di quel caso: orario e controlli (passo.h)
// ORDINE e' l'ordine delle classi: 0 quello generato (generatore.h).
// Senza comando, o con un comando che non esiste, stampa questo elenco ed esce con 2.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comandi/prove.h"
#include "comandi/caso.h"
#include "comandi/passo.h"
#include "dati/generatore.h"

int main(const int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "test")) {  // le prove con i dati casuali
        if (argc != 3 && argc != 5) {
            fprintf(stderr, "Uso: %s test N [ORDINI LIMITE]\n", argv[0]);
            return 2;
        }
        if (argc == 3) esegui_prove(atoi(argv[2]), 1, PROVE_LIMITE_NODI);
        else esegui_prove(atoi(argv[2]), atoi(argv[3]), atoll(argv[4]));
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "caso")) {  // un caso casuale seguito dal sito
        if (argc < 9) {
            fprintf(stderr, "Uso: %s caso SEME CLASSI RITARDI ORDINE LIMITE FILE PASSI\n", argv[0]);
            return 2;
        }
        return esegui_caso(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoll(argv[6]),
                           argv[7], argv[8]);
    }
    if (argc > 1 && !strcmp(argv[1], "passo")) {  // un nodo della ricerca: orario e controlli, per il sito
        if (argc < 8) {
            fprintf(stderr, "Uso: %s passo SEME CLASSI RITARDI ORDINE PASSI K\n", argv[0]);
            return 2;
        }
        return stampa_passo(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), argv[6],
                            atoll(argv[7]));
    }
    if (argc > 1 && !strcmp(argv[1], "dati")) {  // i dati di un caso, per il sito
        if (argc < 6) {
            fprintf(stderr, "Uso: %s dati SEME CLASSI RITARDI ORDINE\n", argv[0]);
            return 2;
        }
        return !stampa_dati_casuali(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    }
    // nessun comando, o un comando che non esiste: l'elenco dei comandi
    if (argc > 1) fprintf(stderr, "Comando sconosciuto: %s\n", argv[1]);
    fprintf(stderr, "Uso:\n"
            "  %s test N [ORDINI LIMITE]                               le prove con i dati casuali\n"
            "  %s caso SEME CLASSI RITARDI ORDINE LIMITE FILE PASSI    un caso casuale, per il sito\n"
            "  %s dati SEME CLASSI RITARDI ORDINE                      i dati di quel caso\n"
            "  %s passo SEME CLASSI RITARDI ORDINE PASSI K             il nodo K di quel caso\n"
            "ORDINE e' l'ordine delle classi: 0 quello generato.\n",
            argv[0], argv[0], argv[0], argv[0]);
    return 2;
}
