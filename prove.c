#include "prove.h"
#include "ricerca.h"
#include "data_initialization/generatore.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Quello che un test manda al processo che lo ha lanciato.
typedef struct {
    int generato;        // 0: i dati non si sono potuti generare
    int trovato, fermato;
    long long int nodi;
    int pos_max, celle;
    double secondi;
} Esito;

// Nel processo figlio: genera i dati, cerca l'orario (al massimo PROVE_LIMITE_NODI nodi) e scrive i passi nel file
// file_passi (i primi PROVE_PASSI nodi).
static Esito un_test(const int seme, const int classi, const int ritardi, const char *file_passi) {
    Esito e = {0};
    if (!genera_dati_casuali(seme, classi, ritardi)) return e;
    e.generato = 1;
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    f = NULL;  // l'orario trovato non si stampa: conta solo il report
    limite_nodi = PROVE_LIMITE_NODI;
    passi = fopen(file_passi, "w");
    passi_max = PROVE_PASSI;
    const clock_t inizio = clock();
    print_orario(0);
    e.secondi = (clock() - inizio) / (double) CLOCKS_PER_SEC;
    if (passi) fclose(passi);
    e.trovato = trovato;
    e.fermato = fermato;
    e.nodi = nodi;
    e.pos_max = pos_max;
    e.celle = orario_len;
    return e;
}

// copia il file da in a (in fondo)
static void accoda(const char *da, FILE *a) {
    FILE *in = fopen(da, "r");
    if (!in) return;
    char buf[1 << 16];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, in)) > 0) fwrite(buf, 1, n, a);
    fclose(in);
}

int esegui_prove(const int n) {
    FILE *report = fopen("report.txt", "w"), *tutti_i_passi = fopen("passi.txt", "w");
    if (!report || !tutti_i_passi) {
        fprintf(stderr, "Non riesco a scrivere report.txt e passi.txt nella cartella corrente\n");
        return 1;
    }
    const int totale = (n + 1) * (n + 1) * 2;
    fprintf(report, "Prove: semi da 0 a %d, classi da %d a %d, senza e con ritardi (%d test), limite %lld nodi\n\n",
            n, PROVE_CLASSI_MIN, PROVE_CLASSI_MIN + n, totale, PROVE_LIMITE_NODI);
    fprintf(report, "%-6s %-6s %-7s %-14s %-19s %12s %12s %10s\n", "test", "seme", "classi", "ritardi", "esito", "nodi",
            "cella max", "secondi");
    fflush(report);
    int k = 0, trovati = 0, al_limite = 0;
    for (int seme = 0; seme <= n; seme++)
        for (int classi = PROVE_CLASSI_MIN; classi <= PROVE_CLASSI_MIN + n; classi++)
            for (int ritardi = 0; ritardi <= 1; ritardi++) {
                k++;
                printf("Test %d di %d: seme %d, %d classi, %s ritardi... ", k, totale, seme, classi,
                       ritardi ? "con" : "senza");
                fflush(stdout);
                char file_passi[64];
                snprintf(file_passi, sizeof file_passi, "passi-%d.tmp", k);
                int tubo[2];
                if (pipe(tubo)) return 1;
                fflush(NULL);
                const pid_t figlio = fork();
                if (figlio == 0) {  // il test, in un processo a parte
                    close(tubo[0]);
                    const Esito e = un_test(seme, classi, ritardi, file_passi);
                    write(tubo[1], &e, sizeof e);
                    _exit(0);
                }
                close(tubo[1]);
                Esito e = {0};
                const ssize_t letti = read(tubo[0], &e, sizeof e);
                close(tubo[0]);
                waitpid(figlio, NULL, 0);
                const char *esito = letti != sizeof e ? "test interrotto"
                                    : !e.generato ? "dati non generati"
                                    : e.trovato ? "trovato"
                                    : e.fermato ? "limite di nodi" : "nessuna soluzione";
                trovati += e.trovato;
                al_limite += e.fermato;
                char cella[32] = "";
                if (e.generato) snprintf(cella, sizeof cella, "%d/%d", e.pos_max, e.celle);
                fprintf(report, "%-6d %-6d %-7d %-14s %-19s %12lld %12s %10.2f\n", k, seme, classi,
                        ritardi ? "con ritardi" : "senza ritardi", esito, e.nodi, cella, e.secondi);
                fflush(report);
                if (e.fermato) {  // solo chi arriva al limite tiene i passi
                    fprintf(tutti_i_passi, "=== test %d: seme %d, %d classi, %s ritardi: i primi %lld nodi ===\n", k,
                            seme, classi, ritardi ? "con" : "senza", PROVE_PASSI);
                    accoda(file_passi, tutti_i_passi);
                    fprintf(tutti_i_passi, "\n");
                    fflush(tutti_i_passi);
                }
                remove(file_passi);
                printf("%s (%lld nodi, %.1f s)\n", esito, e.nodi, e.secondi);
            }
    fprintf(report, "\nTrovati %d su %d, al limite di nodi %d.\n", trovati, totale, al_limite);
    fclose(report);
    fclose(tutti_i_passi);
    printf("Fatto: %d test, %d orari trovati, %d al limite di nodi. Report in report.txt, passi in passi.txt.\n", totale,
           trovati, al_limite);
    return 0;
}
