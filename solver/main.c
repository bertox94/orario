// Il programma dell'orario.
//
//   orario genera SEME CLASSI RITARDI
//       scrive su stdout un caso casuale (genera.h); RITARDI: 1 con i docenti che entrano tardi, 0 senza
//   orario risolvi CASO [--limite NODI] [--stato FILE] [--passi FILE]
//       cerca un orario per il caso (ricerca.h) e ne scrive lo stato, in JSON: su stdout alla fine, oppure in FILE
//       ogni mezzo secondo e alla fine (scritto a parte e poi rinominato: chi legge trova sempre un JSON intero).
//       --limite: dopo tanti nodi si ferma; --passi: scrive ogni nodo della ricerca (passi.h)
//   orario nodo CASO
//       per il sito: quello che c'e' da dire di un nodo della ricerca. Il nodo arriva su stdin, tutto in numeri: il
//       motivo per cui da li' non si va avanti (tipo, docente, classe, ora, servono, possibili: i campi di un record
//       dei passi; tipo 0 se si va avanti), quante celle sono decise, e per ognuna, nell'ordine in cui sono state
//       decise, la cella e la voce (-1: ora buca). Scrive su stdout
//       {"giorno": G, "ora": O, "segni": [[cella, segno], ...], "conto": {...}, "docenti": [...]}:
//       l'ora a cui i conti sono fatti; per ogni cella di cui il motivo parla 1 fatta, 2 puo', 3 no, 4 serve
//       (ricerca.h, SEGNO_); il conto del motivo in dettaglio; e i conti di ogni docente (limiti.c, in fondo).
//
// Lo stato:
//   {"in_corso": 0 | 1, "esito": "in corso" | "trovato" | "impossibile" | "limite di nodi",
//    "nodi": N, "secondi": S, "profondita_massima": P, "celle": C,
//    "ritorni": R, "ritorni_per_cella": [...]   i ritorni indietro, in tutto e per cella (ricerca.h)
//    "scuola": {"giorni": [...], "ore": [...], "classi": [...], "obbligatorie": M, "ore_di_seguito": N, "classi_al_giorno": N,
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

// ---- scrivere lo stato, in JSON ----

// un testo tra virgolette, con la barra davanti alle virgolette e alle barre che contiene
static void json_testo(FILE *f, const char *testo) {
    fputc('"', f);
    for (; *testo; testo++) {
        if (*testo == '"' || *testo == '\\') fputc('\\', f);
        fputc(*testo, f);
    }
    fputc('"', f);
}

// "chiave": ["nome", "nome", ...], con la virgola in fondo
static void json_nomi(FILE *f, const char *chiave, const char nomi[][MAX_NOME], const int n) {
    fprintf(f, "\"%s\":[", chiave);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', f);
        json_testo(f, nomi[i]);
    }
    fputs("],", f);
}

// Lo stato della ricerca (com'e' fatto e' scritto in cima al file): sempre i numeri e la scuola; a ricerca finita
// anche il motivo, se un orario non c'era gia' in partenza, oppure l'orario trovato con la sua verifica.
static void scrivi_stato(FILE *f, const Ricerca *r, const double secondi) {
    const Scuola *s = r->s;
    fprintf(f, "{\"in_corso\":%d,\"esito\":\"%s\",\"nodi\":%lld,\"secondi\":%.2f,\"profondita_massima\":%d,"
            "\"celle\":%d,\"ritorni\":%lld,\"ritorni_per_cella\":[", r->esito == IN_CORSO, ESITI[r->esito], r->nodi,
            secondi, r->profondita_massima, r->celle, r->ritorni);
    for (int i = 0; i < r->celle; i++) fprintf(f, "%s%lld", i ? "," : "", r->ritorni_in[i]);
    fputs("],\"scuola\":{", f);
    json_nomi(f, "giorni", s->nome_giorno, s->giorni);
    json_nomi(f, "ore", s->nome_ora, s->ore);
    json_nomi(f, "classi", s->nome_classe, s->classi);
    fprintf(f, "\"obbligatorie\":%d,\"ore_di_seguito\":%d,\"classi_al_giorno\":%d,\"docenti\":[", s->obbligatorie,
            MAX_ORE_DI_SEGUITO, MAX_CLASSI_AL_GIORNO);
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

// Scrive lo stato nel suo file: prima in un file accanto, poi lo rinomina. Rinominare e' un colpo solo: chi legge
// (il sito) trova o il file vecchio o quello nuovo, mai uno scritto a meta'.
static void stato_su_file(const Seguito *x, const Ricerca *r) {
    char provvisorio[1024];
    snprintf(provvisorio, sizeof provvisorio, "%s.tmp", x->file_stato);
    FILE *f = fopen(provvisorio, "w");
    if (!f) return;
    scrivi_stato(f, r, secondi_da(x->inizio));
    fclose(f);
    rename(provvisorio, x->file_stato);
}

// Chiamata dalla ricerca a ogni nodo (ricerca.h, a_ogni_nodo): scrive il passo e, ogni tanto, lo stato.
static void segui(const Ricerca *r, const int profondita, const int cella, const int voce, const Motivo *m) {
    Seguito *x = r->di_chi;
    if (x->passi) passi_nodo(x->passi, r->nodi, profondita, cella, voce, m);
    if (!x->file_stato || r->nodi & 4095) return;  // l'orologio si guarda ogni 4096 nodi
    const clock_t adesso = clock();
    if (adesso - x->ultima_scrittura < CLOCKS_PER_SEC / 2) return;
    x->ultima_scrittura = adesso;
    stato_su_file(x, r);
}

// ---- un nodo, per il sito ----

// Rimette l'orario com'era al nodo che arriva su stdin (una scelta dopo l'altra, con la stessa segna della ricerca)
// e scrive le celle di cui parla il suo motivo, il conto in dettaglio e i conti di ogni docente. 0, o 2 se quello
// che arriva non e' un nodo di questo caso.
static int nodo(const Scuola *s) {
    Ricerca *r = nuova_ricerca(s);
    signed char *segno = r ? malloc(r->celle) : NULL;
    int tipo, profondita, cella = -1;
    Motivo m;
    if (!segno || scanf("%d %d %d %d %d %d %d", &tipo, &m.docente, &m.classe, &m.ora, &m.servono, &m.possibili,
                        &profondita) != 7)
        return 2;
    m.tipo = (TipoMotivo) tipo;
    // chi e' nominato nel motivo deve esistere: i segni si scrivono nelle sue celle
    if (tipo < NESSUNO || tipo > ORARIO_DELLA_CLASSE || m.docente >= s->docenti || m.classe >= s->classi
        || m.ora >= s->ore || profondita < 0 || profondita > r->celle)
        return 2;
    const int del_docente = tipo == ORE_DOCENTE || tipo == ORE_DOCENTE_IN_CLASSE || tipo == DOCENTE_SENZA_POSTO;
    const int della_classe = tipo != NESSUNO && tipo != ORE_DOCENTE && tipo != DOCENTE_SENZA_POSTO;
    if ((del_docente && m.docente < 0) || (della_classe && m.classe < 0) || (tipo == PRIME_ORE && m.ora < 0)) return 2;
    for (int i = 0; i < profondita; i++) {
        int voce;
        // ogni scelta: una cella ancora vuota, con l'ora buca o una voce della classe di quella cella
        if (scanf("%d %d", &cella, &voce) != 2 || cella < 0 || cella >= r->celle || r->orario[cella] != VUOTA
            || voce < BUCA || voce >= s->voci || (voce >= 0 && s->voce[voce].classe != cella % s->classi))
            return 2;
        segna(r, cella, voce, 1);
    }
    const int ora_della_settimana = segni_del_nodo(r, profondita, cella, &m, segno);
    printf("{\"giorno\":%d,\"ora\":%d,\"segni\":[", ora_della_settimana / s->ore, ora_della_settimana % s->ore);
    for (int i = 0, primo = 1; i < r->celle; i++)
        if (segno[i]) printf("%s[%d,%d]", primo ? "" : ",", i, segno[i]), primo = 0;
    fputs("],\"conto\":", stdout);  // i pezzi da cui vengono i due numeri del conto
    scrivi_il_conto(stdout, r, ora_della_settimana, &m);
    fputs(",\"docenti\":", stdout);  // e i conti di ogni docente
    scrivi_i_docenti(stdout, r, ora_della_settimana);
    puts("}");
    free(segno);
    libera_ricerca(r);
    return 0;
}

static int usa(const char *programma) {
    fprintf(stderr, "Uso:\n  %s genera SEME CLASSI RITARDI\n  %s risolvi CASO [--limite NODI] [--stato FILE] "
            "[--passi FILE]\n  %s nodo CASO < NODO\n", programma, programma, programma);
    return 2;
}

int main(const int argc, char **argv) {
    if (argc == 5 && !strcmp(argv[1], "genera"))
        return genera_caso(stdout, strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4])) ? 0 : 2;
    const int vuole_un_nodo = argc == 3 && !strcmp(argv[1], "nodo");
    if (!vuole_un_nodo && (argc < 3 || strcmp(argv[1], "risolvi"))) return usa(argv[0]);
    // le opzioni di «risolvi»: ognuna con il suo valore dopo
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
    if (vuole_un_nodo) {
        const int uscita = nodo(s);
        free(s);
        return uscita;
    }
    // «risolvi»: la ricerca, con chi la segue (i passi e lo stato su file, se chiesti)
    Ricerca *r = nuova_ricerca(s);
    Seguito seguito = {r && file_passi ? passi_apri(file_passi, r->celle) : NULL, file_stato, clock(), clock()};
    if (!r || (file_passi && !seguito.passi)) {
        fprintf(stderr, "non riesco a preparare la ricerca\n");
        return 2;
    }
    r->limite = limite;
    r->a_ogni_nodo = segui;
    r->di_chi = &seguito;
    if (file_stato) stato_su_file(&seguito, r);  // subito uno stato «in corso»: il sito ha gia' la scuola da mostrare
    cerca_orario(r);
    passi_chiudi(seguito.passi);
    // lo stato finale: nel file se c'e', se no su stdout
    if (file_stato) stato_su_file(&seguito, r);
    else scrivi_stato(stdout, r, secondi_da(seguito.inizio));
    libera_ricerca(r);
    free(s);
    return 0;
}
