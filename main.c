#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_initialization/database_initializers.h"
#include "data_initialization/generatore.h"
#include "orario/orario.h"
#include "printer/printer.h"
#include "printer/risultati_json.h"
#include "ricerca/diagnosi.h"
#include "ricerca/fattibilita.h"
#include "ricerca/ricerca.h"
#include "ricerca/vincoli.h"

// Generatore di orari scolastici. Dati classi, docenti e quante ore fa ogni docente in ogni classe (da solo o in
// codocenza), cerca un orario settimanale che rispetti le regole (ore obbligatorie, ore di ingresso, al massimo 3 ore
// al giorno per docente in una classe e contigue, al massimo 3 classi al giorno per docente, regola del sabato...).
// La ricerca e' una visita in profondita' con marcia indietro (ricerca/ricerca.c) che riempie gli slot in ordine:
// giorno, ora, classe. Prima di cercare si verificano alcune condizioni necessarie sui dati (ricerca/fattibilita.c).

// Uso: ./untitled [seed] [-b | --backtrack-griglia] [--casuale SEME [--classi N] [--ritardi]] [-n MAX] [--tempo SEC]
//                  [--motivi] [--dati FILE] [--senza-verifica] [--orario FILE] [--json FILE]
//   seed: l'ordine delle classi viene mescolato (stesso seed = stesso ordine)
//   -b / --backtrack: scrive nel file dell'orario anche ogni marcia indietro, in formato compatto (una riga con gli slot
//                     cambiati, vedi printer.h). Senza, il file contiene solo gli orari completi.
//   --backtrack-griglia: come -b, ma con la griglia parziale intera a ogni marcia (circa 30 KB per marcia: lento e
//                        molto grande, solo per leggere il file a mano)
//   --orario FILE: dove scrivere gli orari (default orario.txt); se FILE finisce in .gz si scrive compresso con gzip
//   --max-mb MB: interrompe la ricerca quando il file degli orari supera MB megabyte sul disco (per -b); come con
//                --tempo, la diagnosi e il JSON vengono scritti comunque
//   --casuale SEME: invece dei dati del programma usa dati generati a caso (riproducibili con lo stesso SEME):
//                   classi da 29 a 36 ore, docenti da 5 a 24 ore, con lezioni da soli e in codocenza.
//   --classi N: numero di classi dei dati casuali (default 8)
//   --ritardi: nei dati casuali alcuni docenti entrano in ritardo (ora di ingresso 1, 2 o 3)
//   -n MAX: si ferma dopo MAX orari completi
//   --tempo SEC: interrompe la ricerca dopo SEC secondi (la diagnosi viene stampata comunque)
//   --motivi: nella diagnosi indica anche perche' i docenti vengono scartati negli slot con piu' marce indietro
//   --dati FILE: scrive in FILE la descrizione dei dati (classi, docenti, lezioni, codocenze)
//   --senza-verifica: salta la verifica preliminare dei dati (che segnala i casi impossibili prima di cercare)
//   --json FILE: alla fine scrive in FILE dati, esito, primo orario trovato e diagnosi per slot, in JSON (per il sito)
// Le opzioni della riga di comando (n_gen_max e stampa_backtrack invece sono globali della ricerca).
typedef struct {
    int mescola_classi;          // 1 se e' stato dato un seed per l'ordine delle classi
    unsigned int seed_classi;
    int dati_casuali;            // 1 se i dati si generano a caso invece di usare quelli del programma
    unsigned long long seme_dati;
    int n_classi_casuali;
    int con_ritardi;             // dati casuali: alcuni docenti entrano in ritardo
    const char *file_dati;       // dove scrivere la descrizione dei dati (NULL = da nessuna parte)
    double secondi_massimi;      // 0 = nessun limite di tempo
    double mb_massimi;           // 0 = nessun limite di spazio per il file degli orari
    int con_motivi;
    int senza_verifica;          // 1 = non fare la verifica preliminare
    const char *file_orario;     // dove scrivere gli orari (.gz = compresso)
    const char *file_json;       // dove scrivere i risultati in JSON (NULL = da nessuna parte)
} Opzioni;

static Opzioni leggi_opzioni(const int argc, char **argv) {
    Opzioni o = {0};
    o.n_classi_casuali = 8;
    o.file_orario = "orario.txt";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--backtrack")) {
            stampa_backtrack = BACKTRACK_COMPATTO;
        } else if (!strcmp(argv[i], "--backtrack-griglia")) {
            stampa_backtrack = BACKTRACK_GRIGLIA;
        } else if (!strcmp(argv[i], "--orario") && i + 1 < argc) {
            o.file_orario = argv[++i];
        } else if (!strcmp(argv[i], "--max-mb") && i + 1 < argc) {
            o.mb_massimi = atof(argv[++i]);
        } else if (!strcmp(argv[i], "--json") && i + 1 < argc) {
            o.file_json = argv[++i];
        } else if (!strcmp(argv[i], "--casuale") && i + 1 < argc) {
            o.dati_casuali = 1;
            o.seme_dati = strtoull(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--classi") && i + 1 < argc) {
            o.n_classi_casuali = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--ritardi")) {
            o.con_ritardi = 1;
        } else if (!strcmp(argv[i], "--dati") && i + 1 < argc) {
            o.file_dati = argv[++i];
        } else if (!strcmp(argv[i], "--senza-verifica")) {
            o.senza_verifica = 1;
        } else if (!strcmp(argv[i], "--motivi")) {
            o.con_motivi = 1;
        } else if (!strcmp(argv[i], "--tempo") && i + 1 < argc) {
            o.secondi_massimi = atof(argv[++i]);
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            n_gen_max = atoll(argv[++i]);
        } else {
            // qualunque altro argomento e' il seed per l'ordine delle classi
            o.seed_classi = (unsigned int) strtoul(argv[i], NULL, 10);
            o.mescola_classi = 1;
        }
    }
    return o;
}

// Dati del programma oppure dati casuali; poi prepara l'elenco delle lezioni di ogni classe (serve alla verifica
// preliminare e alla potatura) e stampa i dati.
static void carica_dati(const Opzioni *o) {
    if (o->dati_casuali) {
        inizializza_calendario();
        genera_dati_casuali(o->seme_dati, o->n_classi_casuali, o->con_ritardi);
    } else {
        inizializza_dati();
    }
    prepara_lezioni_delle_classi();
    stampa_dati();
    fflush(stdout);
}

// Con --dati FILE: scrive in FILE la descrizione completa dei dati (e, per i dati casuali, come rigenerarli).
static void scrivi_file_dati(const Opzioni *o) {
    if (!o->file_dati) return;
    FILE *fd = fopen(o->file_dati, "w");
    if (!fd) {
        fprintf(stderr, "Impossibile scrivere %s\n", o->file_dati);
        return;
    }
    if (o->dati_casuali)
        fprintf(fd, "Dati casuali: --casuale %llu --classi %d%s\n\n", o->seme_dati, o->n_classi_casuali,
                o->con_ritardi ? " --ritardi" : "");
    stampa_dati_su_file(fd);
    fclose(fd);
}

// Verifica preliminare dei dati: se violano una condizione necessaria il caso e' impossibile e non si cerca.
// Il motivo si scrive su stdout e resta in `motivo` (per il JSON).
static int caso_possibile(const Opzioni *o, char *motivo, const size_t dimensione) {
    motivo[0] = '\0';
    if (o->senza_verifica || verifica_fattibilita(motivo, dimensione)) return 1;
    printf("Impossibile (verifica preliminare): %s\n", motivo);
    fflush(stdout);
    return 0;
}

// Apre il file degli orari. Se il nome finisce in .gz si scrive compresso: i dati passano per gzip (un processo a
// parte, avviato da popen), cosi' sul disco arriva solo il file compresso. *compresso dice come va chiuso.
static FILE *apri_file_orario(const char *nome, int *compresso) {
    const size_t n = strlen(nome);
    *compresso = n > 3 && !strcmp(nome + n - 3, ".gz");
    if (!*compresso) return fopen(nome, "w");
    if (strchr(nome, '\'')) return NULL; // il nome va tra apici nel comando: non deve contenerne
    char comando[4096];
    snprintf(comando, sizeof comando, "gzip -1 > '%s'", nome);
    return popen(comando, "w");
}

// Uno slot per ogni (giorno, ora, classe), nell'ordine in cui la ricerca li riempie; con un seed le classi si mescolano.
static void prepara_orario(const Opzioni *o) {
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    inizializza_orario();
    if (o->mescola_classi) randomizza_ordine_classi(o->seed_classi);
}

// Alla fine: marce indietro, diagnosi, orari trovati e velocita' (orari completi al secondo).
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
    diagnosi_imposta_spazio(o.file_orario, o.mb_massimi);

    printf("Program started at: %s\n", ctime(&currentTime));
    carica_dati(&o);
    scrivi_file_dati(&o);
    prepara_orario(&o);
    char motivo[256];
    if (!caso_possibile(&o, motivo, sizeof motivo)) {
        scrivi_json(o.file_json, argc, argv, motivo, 0);
        return 0;
    }

    // la ricerca scrive nel file dell'orario gli orari completi (e con -b anche ogni marcia indietro)
    int compresso;
    file_orario = apri_file_orario(o.file_orario, &compresso);
    if (!file_orario) {
        fprintf(stderr, "Impossibile scrivere %s\n", o.file_orario);
        return 1;
    }
    if (stampa_backtrack == BACKTRACK_COMPATTO) stampa_intestazione_marce_compatte(file_orario);
    diagnosi_avvia();
    cerca_orario(0);
    if (compresso) pclose(file_orario);
    else fclose(file_orario);

    const double secondi = (clock() - start) / (double) CLOCKS_PER_SEC;
    stampa_risultati(secondi);
    scrivi_json(o.file_json, argc, argv, NULL, secondi);
    return 0;
}
