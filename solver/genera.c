#include "genera.h"
#include "nomi.h"
#include "scuola.h"

#include <stdint.h>
#include <stdlib.h>

// Il generatore dei casi casuali (che casi fa e' scritto in genera.h). Deve dare sempre gli stessi casi: i test
// notturni e la prova automatica li rigenerano dal seme. Per questo i numeri casuali sono fatti in casa (uguali su
// ogni computer) e l'ordine in cui si chiedono non si puo' cambiare: una chiamata in piu' o in meno, o spostata,
// cambia tutti i casi.

#define GIORNI 6
#define ORE 6
#define OBBLIGATORIE 5     // le prime ore con lezione
#define ORE_CLASSE_MIN 30  // le ore di una classe in una settimana
#define ORE_CLASSE_MAX 36
#define ORE_DOCENTE_MIN 5  // le ore di un docente in una settimana
#define ORE_DOCENTE_MAX 24
#define INGRESSO_MAX 3     // l'ingresso piu' tardo: la quarta ora
#define MAX_LEZIONI 4000
#define TENTATIVI 20000    // quante volte si riprova prima di dire che il caso non si riesce a generare

// Una lezione generata: il docente fa `ore` ore nella classe, di cui `insieme` con il codocente (-1: nessuno).
typedef struct {
    int classe, docente, ore, codocente, insieme;
} Lezione;

typedef struct {
    uint64_t stato;  // dei numeri casuali
    Lezione lezione[MAX_LEZIONI];
    int lezioni, docenti, carico[MAX_DOCENTI];
} Caso;

// ---- i numeri casuali: xorshift64* ----

// il prossimo numero casuale: tre spostamenti di bit sullo stato, e una moltiplicazione
static uint64_t casuale(Caso *c) {
    uint64_t x = c->stato;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    c->stato = x;
    return x * 2685821657736338717ULL;
}

// un numero tra `da` e `a`, compresi
static int tra(Caso *c, const int da, const int a) {
    return da + (int) (casuale(c) % (uint64_t) (a - da + 1));
}

// Parte dal seme: lo stato non puo' essere zero (resterebbe zero per sempre), e i primi numeri si buttano perche'
// con semi vicini si somigliano.
static void semina(Caso *c, const unsigned long long seme) {
    c->stato = (uint64_t) seme * 0x9E3779B97F4A7C15ULL + 0x1234567ULL;
    if (!c->stato) c->stato = 1;
    for (int i = 0; i < 8; i++) casuale(c);
}

// ---- un tentativo ----

// il meno carico fra tre docenti presi a caso, tra quelli che con queste ore non superano il massimo (-1: nessuno)
static int il_meno_carico(Caso *c, const int ore, const int escluso) {
    int scelto = -1;
    for (int i = 0; i < 3; i++) {
        const int d = tra(c, 0, c->docenti - 1);
        if (d == escluso || c->carico[d] + ore > ORE_DOCENTE_MAX) continue;
        if (scelto < 0 || c->carico[d] < c->carico[scelto]) scelto = d;
    }
    return scelto;
}

// Prova a generare un caso: le lezioni di ogni classe, poi quanti docenti, poi un docente per ogni lezione. 0 se il
// tentativo non riesce (un docente con troppe o troppo poche ore, o niente lezioni in coppia o da soli).
static int tenta(Caso *c, const int classi) {
    const int percento_in_coppia = tra(c, 15, 60);  // quante lezioni su cento hanno anche un codocente
    int ore_in_tutto = 0;                           // le ore di tutti i docenti, codocenti compresi
    c->lezioni = 0;
    // le lezioni di ogni classe: si tagliano le sue ore in pezzi da 1 a 6, finche' non restano ore
    for (int k = 0; k < classi; k++)
        for (int resto = tra(c, ORE_CLASSE_MIN, ORE_CLASSE_MAX); resto > 0;) {
            if (c->lezioni == MAX_LEZIONI) return 0;
            // (`lunga` in una variabile sua: MIN e' una macro, e scriverci dentro tra(...) lo chiamerebbe due volte)
            const int lunga = tra(c, 1, 6), ore = MIN(lunga, resto);
            resto -= ore;
            Lezione *l = &c->lezione[c->lezioni++];
            *l = (Lezione){k, -1, ore, -1, 0};
            // in coppia: per alcune delle sue ore (da una a tutte) c'e' anche il codocente, che si sceglie dopo
            if (tra(c, 1, 100) <= percento_in_coppia) l->insieme = tra(c, 1, ore);
            ore_in_tutto += ore + l->insieme;
        }
    // i docenti: tanti da avere in media tra 10 e 20 ore, e comunque tra il minimo e il massimo
    const int pochi = (ore_in_tutto + ORE_DOCENTE_MAX - 1) / ORE_DOCENTE_MAX, tanti = ore_in_tutto / ORE_DOCENTE_MIN;
    int da = MAX((ore_in_tutto + 19) / 20, pochi), a = MIN(ore_in_tutto / 10, tanti);
    if (da > a) da = pochi, a = tanti;
    if (da > a || a >= MAX_DOCENTI) return 0;
    c->docenti = tra(c, da, a);
    for (int d = 0; d < c->docenti; d++) c->carico[d] = 0;
    for (int i = 0; i < c->lezioni; i++) {  // le lezioni in ordine casuale
        const int j = tra(c, i, c->lezioni - 1);
        const Lezione l = c->lezione[i];
        c->lezione[i] = c->lezione[j];
        c->lezione[j] = l;
    }
    // un docente per ogni lezione (e un codocente, diverso, per quelle in coppia): il meno carico fra tre a caso
    int in_coppia = 0, da_soli = 0;  // c'e' almeno una lezione in coppia, e almeno una da soli
    for (int i = 0; i < c->lezioni; i++) {
        Lezione *l = &c->lezione[i];
        if ((l->docente = il_meno_carico(c, l->ore, -1)) < 0) return 0;
        c->carico[l->docente] += l->ore;
        if (!l->insieme) {
            da_soli = 1;
            continue;
        }
        if ((l->codocente = il_meno_carico(c, l->insieme, l->docente)) < 0) return 0;
        c->carico[l->codocente] += l->insieme;
        in_coppia = 1;
    }
    // il tentativo vale se nessun docente ha troppo poche ore, e ci sono lezioni di tutti e due i tipi
    for (int d = 0; d < c->docenti; d++)
        if (c->carico[d] < ORE_DOCENTE_MIN) return 0;
    return in_coppia && da_soli;
}

// Le ore di ingresso: quanti docenti entrano tardi (da uno a tutti) e quali; ognuno alla seconda, terza o quarta
// ora, anticipata finche' le sue ore stanno nelle ore che gli restano nella settimana.
static void ritardi(Caso *c, int ingresso[]) {
    int ordine[MAX_DOCENTI], tardi[MAX_DOCENTI] = {0};
    for (int d = 0; d < c->docenti; d++) ordine[d] = d;
    const int quanti = tra(c, 1, c->docenti);
    // i primi `quanti` di un ordine casuale: a ogni posto si scambia con uno a caso tra quelli che restano
    for (int i = 0; i < quanti; i++) {
        const int j = tra(c, i, c->docenti - 1), d = ordine[i];
        ordine[i] = ordine[j];
        ordine[j] = d;
        tardi[ordine[i]] = 1;
    }
    for (int d = 0; d < c->docenti; d++) {
        if (!tardi[d]) continue;
        ingresso[d] = tra(c, 1, INGRESSO_MAX);
        while (ingresso[d] > 0 && c->carico[d] > (ORE - ingresso[d]) * GIORNI) ingresso[d]--;
    }
}

// il nome del docente d: dall'elenco di nomi.h, o D e il suo numero se l'elenco e' finito
static void nome_docente(FILE *f, const int d) {
    if (d < N_NOMI_DOCENTI) fputs(NOMI_DOCENTI[d], f);
    else fprintf(f, "D%03d", d + 1);
}

// Genera il caso e lo scrive: si tenta finche' un tentativo riesce (ogni tentativo continua con i numeri casuali
// dove il precedente si e' fermato), poi si decidono gli ingressi e si scrive tutto nel formato di leggi_scuola.
int genera_caso(FILE *f, const unsigned long long seme, const int classi, const int con_ritardi) {
    Caso *c = malloc(sizeof(Caso));
    if (!c || classi < 1 || classi > MAX_CLASSI) return 0;
    semina(c, seme);
    int riuscito = 0;
    for (int i = 0; i < TENTATIVI && !riuscito; i++) riuscito = tenta(c, classi);
    if (riuscito) {
        int ingresso[MAX_DOCENTI] = {0};
        if (con_ritardi) ritardi(c, ingresso);
        fprintf(f, "# seme %llu, %d classi, %s ritardi\n", seme, classi, con_ritardi ? "con" : "senza");
        fputs("giorni\tLun\tMar\tMer\tGio\tVen\tSab\nore\t08\t09\t10\t11\t12\t13\n", f);
        fprintf(f, "obbligatorie\t%d\n", OBBLIGATORIE);
        for (int d = 0; d < c->docenti; d++) {
            fputs("docente\t", f);
            nome_docente(f, d);
            fprintf(f, "\t%d\n", ingresso[d]);
        }
        // classe per classe; di ogni lezione le ore da solo e quelle in coppia
        for (int k = 0; k < classi; k++)
            for (int i = 0; i < c->lezioni; i++) {
                const Lezione *l = &c->lezione[i];
                if (l->classe != k) continue;
                if (l->ore > l->insieme) {
                    fprintf(f, "lezione\tC%02d\t%d\t", k + 1, l->ore - l->insieme);
                    nome_docente(f, l->docente);
                    fputc('\n', f);
                }
                if (l->insieme) {
                    fprintf(f, "lezione\tC%02d\t%d\t", k + 1, l->insieme);
                    nome_docente(f, l->docente);
                    fputc('\t', f);
                    nome_docente(f, l->codocente);
                    fputc('\n', f);
                }
            }
    }
    free(c);
    return riuscito;
}
