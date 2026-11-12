#include "prove.h"
#include "ricerca.h"
#include "data_initialization/generatore.h"
#include "verifica_dati.h"

#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Quello che un test manda al processo che lo ha lanciato.
typedef struct {
    int al_limite;       // la ricerca si e' fermata a PROVE_LIMITE_NODI nodi
    int pos_max, celle;  // la cella piu' avanti raggiunta, su quante
    double secondi;
} Esito;

// Nel processo figlio: genera i dati e cerca l'orario, al massimo `limite` nodi. Un test con i dati che non passano la
// verifica (verifica_dati.h) si salta: la ricerca non parte e non si stampa niente. (Come i dati che non si generano:
// un test che non arriva al limite.)
static Esito un_test(const int seme, const int classi, const int ritardi, const int ordine,
                     const long long int limite) {
    Esito e = {0};
    char errore[VERIFICA_DATI_ERRORE];
    Scuola *s = nuova_scuola();
    if (!genera_dati_casuali(s, seme, classi, ritardi, ordine) || !verifica_dati(s, errore, sizeof errore)) return e;
    Ricerca *r = nuova_ricerca(s);
    r->limite_nodi = limite;
    const clock_t inizio = clock();
    cerca_orario(r, 0);
    e.secondi = (clock() - inizio) / (double) CLOCKS_PER_SEC;
    e.al_limite = r->fermato;
    e.pos_max = r->pos_max;
    e.celle = r->orario_len;
    return e;  // il processo finisce qui: la memoria torna al sistema
}

// Ogni test in un processo a parte (fork): lo stato della ricerca e i dati sono globali, cosi' ognuno parte da zero.
static Esito in_un_processo(const int seme, const int classi, const int ritardi, const int ordine,
                            const long long int limite) {
    Esito e = {0};
    int tubo[2];
    if (pipe(tubo)) return e;
    fflush(stdout);
    const pid_t figlio = fork();
    if (figlio == 0) {
        close(tubo[0]);
        const Esito x = un_test(seme, classi, ritardi, ordine, limite);
        if (write(tubo[1], &x, sizeof x) != sizeof x) _exit(1);
        _exit(0);
    }
    close(tubo[1]);
    if (read(tubo[0], &e, sizeof e) != sizeof e) e = (Esito){0};
    close(tubo[0]);
    waitpid(figlio, NULL, 0);
    return e;
}

void esegui_prove(const int n, const int ordini, const long long int limite) {
    for (int seme = 0; seme <= n; seme++)
        for (int classi = PROVE_CLASSI_MIN; classi <= PROVE_CLASSI_MIN + n; classi++)
            for (int ritardi = 0; ritardi <= 1; ritardi++)
                for (int ordine = 0; ordine < ordini; ordine++) {
                    const Esito e = in_un_processo(seme, classi, ritardi, ordine, limite);
                    if (!e.al_limite) continue;
                    printf("seme %d, %d classi, %s ritardi", seme, classi, ritardi ? "con" : "senza");
                    if (ordine) printf(", ordine %d", ordine);
                    printf(": %lld nodi, cella piu' avanti %d su %d, %.2f s\n", limite, e.pos_max, e.celle, e.secondi);
                    fflush(stdout);  // chi legge le righe man mano (il sito) le vede subito
                }
}
