// Il programma dell'orario.
//
//   orario genera SEME CLASSI RITARDI
//       scrive su stdout un caso casuale (genera.h); RITARDI: 1 con i docenti che entrano tardi, 0 senza
//   orario risolvi CASO [--limite NODI] [--stato FILE] [--passi FILE]
//       cerca un orario per il caso (ricerca.h) e ne scrive lo stato, in JSON: su stdout alla fine, oppure in FILE
//       ogni mezzo secondo e alla fine (scritto a parte e poi rinominato: chi legge trova sempre un JSON intero).
//       --limite: dopo tanti nodi si ferma; --passi: scrive ogni nodo della ricerca (passi.h)
//
// Lo stato:
//   {"in_corso": 0 | 1, "esito": "in corso" | "trovato" | "impossibile" | "limite di nodi",
//    "nodi": N, "secondi": S, "profondita_massima": P, "celle": C,
//    "scuola": {"giorni": [...], "ore": [...], "classi": [...], "obbligatorie": M,
//               "docenti": [[nome, ingresso], ...], "voci": [[classe, docente, codocente | -1, ore], ...]},
//    "motivo": {...}              con «impossibile» gia' in partenza: il conto che non torna (ricerca.h, Motivo)
//    "orario": [voce | -1, ...]   con «trovato»: una voce per cella (-1: ora buca); la cella di (giorno, ora, classe)
//                                 e' (giorno * ore + ora) * classi + classe
//    "verifica": {"ok": 0 | 1, "controlli": [[regola, 0 | 1, dove], ...]}   con «trovato» (verifica.h)}
// Esce con 0, o con 2 se il comando non va o il caso non si legge.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "genera.h"
#include "passi.h"
#include "ricerca.h"
#include "verifica.h"

static const char *const ESITI[] = {"in corso", "trovato", "impossibile", "limite di nodi"};
static const char *const MOTIVI[] = {"", "ore_docente", "ore_docente_in_classe", "ore_classe", "prime_ore",
                                     "docente_senza_posto", "classe_senza_scelte", "orario_della_classe"};

static void json_testo(FILE *f, const char *testo) {
    fputc('"', f);
    for (; *testo; testo++) {
        if (*testo == '"' || *testo == '\\') fputc('\\', f);
        fputc(*testo, f);
    }
    fputc('"', f);
}

static void json_nomi(FILE *f, const char *chiave, const char nomi[][MAX_NOME], const int n) {
    fprintf(f, "\"%s\":[", chiave);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', f);
        json_testo(f, nomi[i]);
    }
    fputs("],", f);
}

static void scrivi_stato(FILE *f, const Ricerca *r, const double secondi) {
    const Scuola *s = r->s;
    fprintf(f, "{\"in_corso\":%d,\"esito\":\"%s\",\"nodi\":%lld,\"secondi\":%.2f,\"profondita_massima\":%d,"
            "\"celle\":%d,\"scuola\":{", r->esito == IN_CORSO, ESITI[r->esito], r->nodi, secondi,
            r->profondita_massima, r->celle);
    json_nomi(f, "giorni", s->nome_giorno, s->giorni);
    json_nomi(f, "ore", s->nome_ora, s->ore);
    json_nomi(f, "classi", s->nome_classe, s->classi);
    fprintf(f, "\"obbligatorie\":%d,\"docenti\":[", s->obbligatorie);
    for (int t = 0; t < s->docenti; t++) {
        fputs(t ? ",[" : "[", f);
        json_testo(f, s->nome_docente[t]);
        fprintf(f, ",%d]", s->ingresso[t]);
    }
    fputs("],\"voci\":[", f);
    for (int v = 0; v < s->voci; v++)
        fprintf(f, "%s[%d,%d,%d,%d]", v ? "," : "", s->voce[v].classe, s->voce[v].docente[0], s->voce[v].docente[1],
                s->voce[v].ore);
    fputs("]}", f);
    if (r->esito == IMPOSSIBILE && r->motivo_radice.tipo != NESSUNO) {
        const Motivo *m = &r->motivo_radice;
        fprintf(f, ",\"motivo\":{\"tipo\":\"%s\",\"docente\":%d,\"classe\":%d,\"ora\":%d,\"servono\":%d,"
                "\"possibili\":%d}", MOTIVI[m->tipo], m->docente, m->classe, m->ora, m->servono, m->possibili);
    }
    if (r->esito == TROVATO) {
        Controllo controllo[CONTROLLI];
        const int ok = verifica_orario(s, r->orario, controllo);
        fputs(",\"orario\":[", f);
        for (int i = 0; i < r->celle; i++) fprintf(f, "%s%d", i ? "," : "", r->orario[i]);
        fprintf(f, "],\"verifica\":{\"ok\":%d,\"controlli\":[", ok);
        for (int i = 0; i < CONTROLLI; i++) {
            fputs(i ? ",[" : "[", f);
            json_testo(f, controllo[i].regola);
            fprintf(f, ",%d,", controllo[i].rispettata);
            json_testo(f, controllo[i].dove);
            fputc(']', f);
        }
        fputs("]}", f);
    }
    fputs("}\n", f);
}

// ---- seguire la ricerca: lo stato su file ogni mezzo secondo ----

// Chi segue la ricerca: i passi su file (se chiesti) e lo stato su file ogni mezzo secondo (se chiesto).
typedef struct {
    Passi *passi;
    const char *file_stato;
    clock_t inizio, ultima_scrittura;
} Seguito;

static double secondi_da(const clock_t inizio) {
    return (clock() - inizio) / (double) CLOCKS_PER_SEC;
}

static void stato_su_file(const Seguito *x, const Ricerca *r) {
    char provvisorio[1024];
    snprintf(provvisorio, sizeof provvisorio, "%s.tmp", x->file_stato);
    FILE *f = fopen(provvisorio, "w");
    if (!f) return;
    scrivi_stato(f, r, secondi_da(x->inizio));
    fclose(f);
    rename(provvisorio, x->file_stato);
}

static void segui(const Ricerca *r, const int profondita, const int cella, const int voce, const Motivo *m) {
    Seguito *x = r->di_chi;
    if (x->passi) passi_nodo(x->passi, r->nodi, profondita, cella, voce, m);
    if (!x->file_stato || r->nodi & 4095) return;  // l'orologio si guarda ogni 4096 nodi
    const clock_t adesso = clock();
    if (adesso - x->ultima_scrittura < CLOCKS_PER_SEC / 2) return;
    x->ultima_scrittura = adesso;
    stato_su_file(x, r);
}

static int usa(const char *programma) {
    fprintf(stderr, "Uso:\n  %s genera SEME CLASSI RITARDI\n  %s risolvi CASO [--limite NODI] [--stato FILE] "
            "[--passi FILE]\n", programma, programma);
    return 2;
}

int main(const int argc, char **argv) {
    if (argc == 5 && !strcmp(argv[1], "genera"))
        return genera_caso(stdout, strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4])) ? 0 : 2;
    if (argc < 3 || strcmp(argv[1], "risolvi")) return usa(argv[0]);
    const char *file_stato = NULL, *file_passi = NULL;
    long long limite = 0;
    for (int i = 3; i < argc; i += 2) {
        if (i + 1 == argc) return usa(argv[0]);
        if (!strcmp(argv[i], "--limite")) limite = atoll(argv[i + 1]);
        else if (!strcmp(argv[i], "--stato")) file_stato = argv[i + 1];
        else if (!strcmp(argv[i], "--passi")) file_passi = argv[i + 1];
        else return usa(argv[0]);
    }
    char errore[256];
    Scuola *s = leggi_scuola(argv[2], errore, sizeof errore);
    if (!s) {
        fprintf(stderr, "%s\n", errore);
        return 2;
    }
    Ricerca *r = nuova_ricerca(s);
    Seguito seguito = {r && file_passi ? passi_apri(file_passi, r->celle) : NULL, file_stato, clock(), clock()};
    if (!r || (file_passi && !seguito.passi)) {
        fprintf(stderr, "non riesco a preparare la ricerca\n");
        return 2;
    }
    r->limite = limite;
    r->a_ogni_nodo = segui;
    r->di_chi = &seguito;
    if (file_stato) stato_su_file(&seguito, r);
    cerca_orario(r);
    passi_chiudi(seguito.passi);
    if (file_stato) stato_su_file(&seguito, r);
    else scrivi_stato(stdout, r, secondi_da(seguito.inizio));
    libera_ricerca(r);
    free(s);
    return 0;
}
