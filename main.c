// Il programma: lancia la ricerca dell'orario (ricerca.c).
//
// ./untitled           i dati di prova (inizializza_dati): l'orario trovato in orario.txt, su console il tempo
// ./untitled test N    le prove con i dati casuali (prove.h)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_initialization/database_initializers.h"
#include "printer/printer.h"
#include "ricerca.h"
#include "prove.h"

int main(const int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "test")) {  // le prove con i dati casuali
        if (argc < 3) {
            fprintf(stderr, "Uso: %s test N\n", argv[0]);
            return 2;
        }
        esegui_prove(atoi(argv[2]));
        return 0;
    }
    const clock_t inizio = clock();
    inizializza_dati();
    stampa_dati();
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;

    f = fopen("orario.txt", "w");
    print_orario(0);

    const double secondi = (clock() - inizio) / (double) CLOCKS_PER_SEC;
    printf(trovato ? "Orario trovato in %.2f s (in orario.txt)\n" : "Nessun orario possibile (%.2f s)\n", secondi);
    fclose(f);
    return 0;
}
