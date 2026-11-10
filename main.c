// Il programma: lancia la ricerca dell'orario (ricerca.c).
//
// ./untitled           i dati di prova (inizializza_dati): su console solo il tempo impiegato
// ./untitled test N    le prove con i dati casuali (prove.h)
// ./untitled caso SEME CLASSI RITARDI LIMITE FILE [PASSI]   un caso casuale per il sito (caso.h)
// ./untitled passo SEME CLASSI RITARDI PASSI K               il nodo K di quel caso: orario e controlli (passo.h)
// ./untitled dati SEME CLASSI RITARDI                         i dati di quel caso in JSON (generatore.h)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_initialization/database_initializers.h"
#include "ricerca.h"
#include "prove.h"
#include "caso.h"
#include "passo.h"
#include "verifica_dati.h"
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
        if (argc < 7) {
            fprintf(stderr, "Uso: %s caso SEME CLASSI RITARDI LIMITE FILE [PASSI]\n", argv[0]);
            return 2;
        }
        return esegui_caso(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]), atoll(argv[5]), argv[6],
                           argc > 7 ? argv[7] : NULL);
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
        if (!genera_dati_casuali(strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4]))) return 1;
        stampa_dati_casuali();
        return 0;
    }
    const clock_t inizio = clock();
    inizializza_dati();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;

    char errore[512];
    if (!verifica_dati(errore, sizeof errore)) {  // l'orario non puo' esistere: la ricerca non parte
        printf("Errore nei dati: %s\n", errore);
        return 1;
    }
    print_orario(0);

    const double secondi = (clock() - inizio) / (double) CLOCKS_PER_SEC;
    printf(trovato ? "Orario trovato in %.2f s\n" : "Nessun orario possibile (%.2f s)\n", secondi);
    return 0;
}
