#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_initialization/database_initializers.h"
#include "data_initialization/generatore.h"
#include "orario/orario.h"
#include "printer/printer.h"
#include "ricerca/diagnosi.h"
#include "ricerca/ricerca.h"

// Uso: ./untitled [seed] [-b] [--casuale SEME [--classi N]] [-n MAX] [--tempo SEC] [--motivi] [--dati FILE]
//   seed: l'ordine delle classi viene mescolato (stesso seed = stesso ordine)
//   -b / --backtrack: scrive in orario.txt anche ogni marcia indietro (riga "Backtrack" + griglia parziale).
//                     Senza, orario.txt contiene solo gli orari completi e si va molto piu' veloci.
//   --casuale SEME: invece dei dati del programma usa dati generati a caso (riproducibili con lo stesso SEME):
//                   classi da 29 a 36 ore, docenti da 5 a 24 ore, con lezioni da soli e in codocenza.
//   --classi N: numero di classi dei dati casuali (default 8)
//   -n MAX: si ferma dopo MAX orari completi
//   --tempo SEC: interrompe la ricerca dopo SEC secondi (la diagnosi viene stampata comunque)
//   --motivi: nella diagnosi indica anche perche' i docenti vengono scartati negli slot con piu' marce indietro
//   --dati FILE: scrive in FILE la descrizione dei dati (classi, docenti, lezioni, codocenze)
typedef struct {
    int mescola_classi;          // 1 se e' stato dato un seed per l'ordine delle classi
    unsigned int seed_classi;
    int dati_casuali;            // 1 se i dati si generano a caso invece di usare quelli del programma
    unsigned long long seme_dati;
    int n_classi_casuali;
    const char *file_dati;       // dove scrivere la descrizione dei dati (NULL = da nessuna parte)
    double secondi_massimi;      // 0 = nessun limite di tempo
    int con_motivi;
} Opzioni;

static Opzioni leggi_opzioni(const int argc, char **argv) {
    Opzioni o = {0};
    o.n_classi_casuali = 8;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--backtrack")) {
            stampa_backtrack = 1;
        } else if (!strcmp(argv[i], "--casuale") && i + 1 < argc) {
            o.dati_casuali = 1;
            o.seme_dati = strtoull(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--classi") && i + 1 < argc) {
            o.n_classi_casuali = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--dati") && i + 1 < argc) {
            o.file_dati = argv[++i];
        } else if (!strcmp(argv[i], "--motivi")) {
            o.con_motivi = 1;
        } else if (!strcmp(argv[i], "--tempo") && i + 1 < argc) {
            o.secondi_massimi = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            n_gen_max = atoll(argv[++i]);
        } else {
            o.seed_classi = (unsigned int) strtoul(argv[i], NULL, 10);
            o.mescola_classi = 1;
        }
    }
    return o;
}

// Dati del programma oppure dati casuali; poi li stampa.
static void carica_dati(const Opzioni *o) {
    if (o->dati_casuali) {
        inizializza_calendario();
        genera_dati_casuali(o->seme_dati, o->n_classi_casuali);
    } else {
        inizializza_dati();
    }
    stampa_dati();
    fflush(stdout);
}

static void scrivi_file_dati(const Opzioni *o) {
    if (!o->file_dati) return;
    FILE *fd = fopen(o->file_dati, "w");
    if (!fd) {
        fprintf(stderr, "Impossibile scrivere %s\n", o->file_dati);
        return;
    }
    if (o->dati_casuali)
        fprintf(fd, "Dati casuali: --casuale %llu --classi %d\n\n", o->seme_dati, o->n_classi_casuali);
    stampa_dati_su_file(fd);
    fclose(fd);
}

static void prepara_orario(const Opzioni *o) {
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    inizializza_orario();
    if (o->mescola_classi) randomizza_ordine_classi(o->seed_classi);
}

static void stampa_risultati(const double secondi) {
    printf("Marce indietro: %lld\n", n_backtrack);
    stampa_diagnosi(n_backtrack);
    printf("Totale generati: %llu in %d m, %d s at %.0f Hz", n_gen, (int) secondi / 60, (int) secondi % 60,
           (double) n_gen / secondi);
    fflush(stdout);
}

int main(const int argc, char **argv) {
    time_t currentTime;
    time(&currentTime);
    const clock_t start = clock();
    const Opzioni o = leggi_opzioni(argc, argv);
    diagnosi_imposta(o.secondi_massimi, o.con_motivi);

    printf("Program started at: %s\n", ctime(&currentTime));
    carica_dati(&o);
    scrivi_file_dati(&o);
    prepara_orario(&o);

    diagnosi_avvia();
    file_orario = fopen("orario.txt", "w");
    cerca_orario(0);
    fclose(file_orario);

    stampa_risultati((clock() - start) / (double) CLOCKS_PER_SEC);
    return 0;
}
