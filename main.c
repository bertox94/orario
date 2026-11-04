#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_initialization/database_initializers.h"
#include "data_initialization/dati_da_file.h"
#include "data_initialization/generatore.h"
#include "orario/orario.h"
#include "printer/printer.h"
#include "printer/risultati_json.h"
#include "priority_queue/prio_queue.h"
#include "ricerca/diagnosi.h"
#include "ricerca/fattibilita.h"
#include "ricerca/ricerca.h"
#include "ricerca/vincoli.h"

// Generatore di orari scolastici. Dati classi, docenti e quante ore fa ogni docente in ogni classe (da solo o in
// codocenza), cerca un orario settimanale che rispetti le regole (ore obbligatorie, ore di ingresso, al massimo 3 ore
// al giorno per docente in una classe e contigue, al massimo 3 classi al giorno per docente, regola del sabato...).
// La ricerca e' una visita in profondita' con marcia indietro (ricerca/ricerca.c) che riempie gli slot in ordine:
// giorno, ora, classe. Prima di cercare si verificano alcune condizioni necessarie sui dati (ricerca/fattibilita.c).

// Uso: ./untitled [seed] [-b [--passi N]] [--casuale SEME [--classi N] [--ritardi]] [-n MAX] [--tempo SEC] [--motivi]
//                  [--dati FILE] [--orario FILE] [--json FILE] [--input FILE] [--solo-verifica] [--progresso]
//                  [--ore-obbligatorie N] [--ore-prof-classe N] [--classi-giorno N] [--analizza-slot FILE]
//   seed: l'ordine delle classi viene mescolato (stesso seed = stesso ordine)
//   -b / --backtrack: scrive nel file dell'orario anche ogni marcia indietro, in formato compatto (una riga con gli slot
//                     cambiati, vedi printer.h). Senza, il file contiene solo gli orari completi.
//   --passi N: con -b, scrive anche una riga "P" con l'orario parziale a ogni nodo della ricerca (dal primo slot deciso
//              in poi), la riga "S" per i nodi scartati dalla potatura e la riga "R" a ogni ritorno a un nodo dopo una
//              marcia indietro: per rivedere la ricerca passo passo dal sito. La registrazione (righe P, R, S, B e M)
//              si ferma dopo N nodi (righe P), cosi' il file resta piccolo anche se la ricerca va avanti a lungo.
//   --orario FILE: dove scrivere gli orari (default orario.txt); se FILE finisce in .gz si scrive compresso con gzip
//   --analizza-slot FILE: non cerca; rifa' lo stato fino a uno slot con l'orario parziale in FILE (prima riga la posizione,
//                poi gli slot come nel formato di -b) e scrive su una riga "ANALISI {json}" la coda dei docenti di quello
//                slot e perche' ognuno e' o non e' utilizzabile (per il sito). Va con gli stessi argomenti dei dati.
//   --casuale SEME: invece dei dati del programma usa dati generati a caso (riproducibili con lo stesso SEME):
//                   classi da 29 a 36 ore, docenti da 5 a 24 ore, con lezioni da soli e in codocenza.
//   --classi N: numero di classi dei dati casuali (default 8)
//   --ritardi: nei dati casuali alcuni docenti entrano in ritardo (ora di ingresso 1, 2 o 3)
//   -n MAX: si ferma dopo MAX orari completi
//   --tempo SEC: interrompe la ricerca dopo SEC secondi (la diagnosi viene stampata comunque)
//   --motivi: nella diagnosi indica anche perche' i docenti vengono scartati negli slot con piu' marce indietro
//   --dati FILE: scrive in FILE la descrizione dei dati (classi, docenti, lezioni, codocenze)
//   --json FILE: alla fine scrive in FILE dati, esito, primo orario trovato e diagnosi per slot, in JSON (per il sito)
//   --input FILE: legge i dati da FILE (JSON, vedi data_initialization/dati_da_file.h) invece di usare quelli del
//                 programma. Se i dati non vanno bene scrive "Errore nei dati: ..." ed esce con codice 2.
//   --solo-verifica: fa solo la verifica preliminare (e scrive il JSON con i dati), senza cercare
//   --progresso: durante la ricerca, circa ogni secondo, scrive su stdout una riga "PROGRESSO {json}" con nodi, marce
//                indietro, posizione corrente e piu' avanzata e marce per slot (per seguire la ricerca dal sito)
//   --ore-obbligatorie N, --ore-prof-classe N, --classi-giorno N: i parametri delle regole (default 5, 3, 3): ore
//                obbligatorie ogni giorno, ore al giorno di un docente nella stessa classe, classi diverse al giorno
// Se una scrittura nel file degli orari fallisce (disco pieno) la ricerca si ferma in modo ordinato, con diagnosi e JSON.
// Con il segnale SIGTERM (il pulsante "Ferma" del sito) la ricerca si ferma in modo ordinato, con diagnosi e JSON.
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
    int con_motivi;
    const char *file_orario;     // dove scrivere gli orari (.gz = compresso)
    const char *file_json;       // dove scrivere i risultati in JSON (NULL = da nessuna parte)
    const char *file_input;      // dati da leggere (NULL = dati del programma o casuali)
    int solo_verifica;
    const char *file_analisi;    // --analizza-slot FILE: analisi di uno slot, poi esce
    int con_progresso;
} Opzioni;

static Opzioni leggi_opzioni(const int argc, char **argv) {
    Opzioni o = {0};
    o.n_classi_casuali = 8;
    o.file_orario = "orario.txt";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--backtrack")) {
            stampa_backtrack = BACKTRACK_COMPATTO;
        } else if (!strcmp(argv[i], "--orario") && i + 1 < argc) {
            o.file_orario = argv[++i];
        } else if (!strcmp(argv[i], "--input") && i + 1 < argc) {
            o.file_input = argv[++i];
        } else if (!strcmp(argv[i], "--solo-verifica")) {
            o.solo_verifica = 1;
        } else if (!strcmp(argv[i], "--analizza-slot") && i + 1 < argc) {
            o.file_analisi = argv[++i];
        } else if (!strcmp(argv[i], "--progresso")) {
            o.con_progresso = 1;
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
        } else if (!strcmp(argv[i], "--passi") && i + 1 < argc) {
            passi_max = atoll(argv[++i]);
        } else if (!strcmp(argv[i], "--motivi")) {
            o.con_motivi = 1;
        } else if (!strcmp(argv[i], "--ore-obbligatorie") && i + 1 < argc) {
            param_ore_obbligatorie = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--ore-prof-classe") && i + 1 < argc) {
            param_ore_prof_in_una_classe = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--classi-giorno") && i + 1 < argc) {
            param_classi_diverse_al_giorno = atoi(argv[++i]);
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
    if (param_ore_obbligatorie < 1 || param_ore_obbligatorie > ORA_GIORNALIERA_SZ || param_ore_prof_in_una_classe < 1 ||
        param_ore_prof_in_una_classe > ORA_GIORNALIERA_SZ || param_classi_diverse_al_giorno < 1 ||
        param_classi_diverse_al_giorno > LIMITE_CLASSI_DIVERSE) {
        fprintf(stderr, "Parametri fuori dai limiti: ore obbligatorie 1..%d, ore di un docente in una classe 1..%d, "
                        "classi diverse al giorno 1..%d\n", ORA_GIORNALIERA_SZ, ORA_GIORNALIERA_SZ, LIMITE_CLASSI_DIVERSE);
        exit(2);
    }
    return o;
}

// Dati del programma oppure dati casuali; poi prepara l'elenco delle lezioni di ogni classe (serve alla verifica
// preliminare e alla potatura) e stampa i dati.
// Ritorna 0 se i dati letti da file non vanno bene (il motivo e' gia' stampato).
static int carica_dati(const Opzioni *o) {
    if (o->file_input) {
        char errore[512];
        prepara_coda();
        if (!carica_dati_da_file(o->file_input, errore, sizeof errore)) {
            printf("Errore nei dati: %s\n", errore);
            fflush(stdout);
            return 0;
        }
    } else if (o->dati_casuali) {
        inizializza_calendario();
        genera_dati_casuali(o->seme_dati, o->n_classi_casuali, o->con_ritardi);
    } else {
        inizializza_dati();
    }
    prepara_lezioni_delle_classi();
    ricorda_ordine_iniziale_coda();
    stampa_dati();
    fflush(stdout);
    return 1;
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
    if (verifica_fattibilita(motivo, dimensione)) return 1;
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
    // Un file oltre il limite del sistema (SIGXFSZ) o gzip che termina (SIGPIPE scrivendo nella pipe)
    // terminerebbero il solver senza diagnosi ne' JSON. Ignorati, la scrittura fallisce con un errore che la ricerca
    // nota (diagnosi_controllo_periodico) e si ferma in modo ordinato. Lo stesso vale per il disco pieno.
    signal(SIGXFSZ, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    // SIGTERM: fermarsi in modo ordinato (la ricerca lo nota nel controllo periodico)
    signal(SIGTERM, diagnosi_richiesta_di_arresto);
    diagnosi_imposta(o.secondi_massimi, o.con_motivi);
    diagnosi_imposta_progresso(o.con_progresso);

    printf("Program started at: %s\n", ctime(&currentTime));
    if (!carica_dati(&o)) return 2;
    scrivi_file_dati(&o);
    prepara_orario(&o);
    if (o.file_analisi) {
        printf("ANALISI ");
        if (analizza_slot(o.file_analisi, stdout)) return 0;
        fprintf(stderr, "File dell'orario parziale non valido: %s\n", o.file_analisi);
        return 3;
    }
    char motivo[256];
    if (!caso_possibile(&o, motivo, sizeof motivo)) {
        scrivi_json(o.file_json, argc, argv, motivo, 0);
        return 0;
    }
    if (o.solo_verifica) {
        printf("Verifica preliminare superata\n");
        scrivi_json(o.file_json, argc, argv, NULL, 0);
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
    // anche la chiusura scrive (l'ultimo buffer; per il .gz la fine del file compresso) e puo' fallire
    if (ferror(file_orario)) errore_di_scrittura = 1;
    if (compresso ? pclose(file_orario) != 0 : fclose(file_orario) != 0) errore_di_scrittura = 1;
    file_orario = NULL;

    const double secondi = (clock() - start) / (double) CLOCKS_PER_SEC;
    stampa_risultati(secondi);
    scrivi_json(o.file_json, argc, argv, NULL, secondi);
    return 0;
}
