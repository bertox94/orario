// Il programma dell'orario: i comandi. Ognuno mette insieme i pezzi delle altre cartelle (scuola, generatore,
// solver, verifica, passi, spiegazioni); lo stato in JSON e' in stato.c, il file per continuare in fine.c.
//
//   orario genera SEME CLASSI RITARDI
//       scrive su stdout un caso casuale (genera.h); RITARDI: 1 con i docenti che entrano tardi, 0 senza
//   orario risolvi CASO [--limite NODI] [--stato FILE] [--passi FILE] [--continua]
//       cerca un orario per il caso (ricerca.h) e ne scrive lo stato, in JSON: su stdout alla fine, oppure in FILE
//       ogni mezzo secondo e alla fine (scritto a parte e poi rinominato: chi legge trova sempre un JSON intero).
//       --limite: dopo tanti nodi si ferma; --passi: scrive ogni nodo della ricerca (passi.h)
//       --continua (con --passi): riprende la ricerca che ha scritto quei passi, fermata dal limite di nodi, dal
//       punto in cui era, e va avanti fino al limite nuovo (i nodi in tutto, non quelli in piu'). I passi nuovi si
//       aggiungono in fondo: alla fine tutto e' come dopo una ricerca sola con il limite nuovo.
//       --da FILE (non con --continua): la ricerca parte da una strada scelta a mano, scritta in FILE in numeri:
//       quante scelte, e per ognuna la cella e la voce (-1: ora buca). Vedi ricerca.h, cerca_da_una_strada: se non
//       trova, «impossibile» vuol dire solo che non c'e' un orario nella parte cercata. Nello stato c'e' "a_mano": 1.
//   orario scelte CASO
//       per il sito: le scelte che la ricerca ha in fondo a una strada. La strada arriva su stdin come per --da.
//       Scrive {"cella": C, "scelte": [voce | -1, ...]} nell'ordine in cui la ricerca le proverebbe (cella -1 e
//       nessuna scelta se da li' non si va avanti).
//   orario scuola CASO
//       scrive {"celle": N, "scuola": {...}}: il caso come lo vede il programma (stato.h), con i docenti e le voci
//       numerati come li numera lui. Serve a chi fa l'orario con un altro metodo (esatto/) e deve parlarne con gli
//       stessi numeri.
//   orario verifica CASO
//       ricontrolla un orario fatto da altri: su stdin una voce per cella (-1: ora buca), nell'ordine delle celle.
//       Scrive {"orario": [...], "verifica": {...}} (verifica.h). Esce con 2 se quello che arriva non e' un orario
//       di questo caso (numeri mancanti, o voci che non esistono).
//   orario nodo CASO
//       per il sito: quello che c'e' da dire di un nodo della ricerca. Il nodo arriva su stdin, tutto in numeri: il
//       motivo per cui da li' non si va avanti (tipo, docente, classe, ora, servono, possibili: i campi di un record
//       dei passi; tipo 0 se si va avanti), quante celle sono decise, e per ognuna, nell'ordine in cui sono state
//       decise, la cella e la voce (-1: ora buca). Scrive su stdout
//       {"giorno": G, "ora": O, "segni": [[cella, segno], ...], "conto": {...}, "docenti": [...], "buche": [...]}:
//       l'ora a cui i conti sono fatti; per ogni cella di cui il motivo parla 1 fatta, 2 puo', 3 no, 4 serve
//       (SEGNO_); il conto del motivo in dettaglio; e i conti di ogni docente (tutto in spiegazioni/spiega.h).
//
// Lo stato che «risolvi» scrive e' descritto in stato.h.
// Esce con 0, o con 2 se il comando non va o il caso non si legge.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../generatore/genera.h"
#include "../passi/passi.h"
#include "../solver/ricerca.h"
#include "../spiegazioni/spiega.h"
#include "fine.h"
#include "stato.h"

// ---- seguire la ricerca: lo stato su file ogni mezzo secondo ----

static int a_mano = 0;  // la ricerca e' partita da una strada scelta a mano (--da): si scrive nello stato

// Chi segue la ricerca: i passi su file (se chiesti) e lo stato su file ogni mezzo secondo (se chiesto).
typedef struct {
    Passi *passi;
    const char *file_stato;
    clock_t inizio, ultima_scrittura;
    double secondi_prima;  // di una ricerca che continua: i secondi che aveva gia' fatto
} Seguito;

static double secondi_da(const Seguito *x) {
    return x->secondi_prima + (clock() - x->inizio) / (double) CLOCKS_PER_SEC;
}

// Scrive lo stato nel suo file: prima in un file accanto, poi lo rinomina. Rinominare e' un colpo solo: chi legge
// (il sito) trova o il file vecchio o quello nuovo, mai uno scritto a meta'.
static void stato_su_file(const Seguito *x, const Ricerca *r) {
    char provvisorio[1024];
    snprintf(provvisorio, sizeof provvisorio, "%s.tmp", x->file_stato);
    FILE *f = fopen(provvisorio, "w");
    if (!f) return;
    scrivi_stato(f, r, secondi_da(x), a_mano);
    fclose(f);
    rename(provvisorio, x->file_stato);
}

// Chiamata dalla ricerca a ogni nodo (ricerca.h, a_ogni_nodo): scrive il passo e, ogni tanto, lo stato.
static void segui(const Ricerca *r, const int profondita, const int cella, const int voce, const Motivo *m) {
    Seguito *x = r->di_chi;
    if (x->passi) passi_nodo(x->passi, r->nodi, profondita, cella, voce, m);
    if (!x->file_stato || r->nodi & 4095) return;  // l'orologio si guarda ogni 4096 nodi
    // e' passato mezzo secondo dall'ultima scrittura?
    const clock_t adesso = clock();
    if (adesso - x->ultima_scrittura < CLOCKS_PER_SEC / 2) return;
    x->ultima_scrittura = adesso;
    stato_su_file(x, r);
}

// ---- una strada scritta in numeri ----

// Legge da `f` una strada: quante scelte, e per ognuna la cella e la voce. Ritorna le coppie (da liberare) e in *n
// quante sono; NULL se non e' scritta bene o i numeri non sono celle e voci di questa ricerca. (Che ogni scelta sia
// davvero possibile lo controlla poi la ricerca, percorrendola.)
static int *leggi_la_strada(FILE *f, const Ricerca *r, int *n) {
    if (fscanf(f, "%d", n) != 1 || *n < 0 || *n > r->celle) return NULL;
    int *strada = malloc(sizeof(int) * 2 * (*n + 1));
    for (int i = 0; strada && i < *n; i++)
        if (fscanf(f, "%d %d", &strada[2 * i], &strada[2 * i + 1]) != 2 || strada[2 * i] < 0
            || strada[2 * i] >= r->celle || strada[2 * i + 1] < BUCA || strada[2 * i + 1] >= r->s->voci) {
            free(strada);
            return NULL;
        }
    return strada;
}

// «scelte»: le scelte in fondo alla strada che arriva su stdin. 0, o 2 se la strada non e' possibile.
static int scelte_di_una_strada(const Scuola *s) {
    Ricerca *r = nuova_ricerca(s);
    int n = 0, cella = -1;
    int *strada = r ? leggi_la_strada(stdin, r, &n) : NULL, *scelte = r ? malloc(sizeof(int) * r->max_scelte) : NULL;
    const int quante = strada && scelte ? scelte_dopo_la_strada(r, strada, n, &cella, scelte) : -1;
    if (quante < 0) return 2;
    printf("{\"cella\":%d,\"scelte\":[", cella);
    for (int i = 0; i < quante; i++) printf("%s%d", i ? "," : "", scelte[i]);
    puts("]}");
    free(strada), free(scelte);
    libera_ricerca(r);
    return 0;
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
    // (il tipo arriva come numero: lo stesso che c'e' nei passi)
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
        // solo le celle che hanno un segno
        if (segno[i]) printf("%s[%d,%d]", primo ? "" : ",", i, segno[i]), primo = 0;
    fputs("],\"conto\":", stdout);  // i pezzi da cui vengono i due numeri del conto
    scrivi_il_conto(stdout, r, ora_della_settimana, &m);
    fputs(",\"docenti\":", stdout);  // e i conti di ogni docente
    scrivi_i_docenti(stdout, r, ora_della_settimana);
    fputs(",\"buche\":", stdout);  // e le ore buche di ogni classe
    scrivi_le_buche(stdout, r, ora_della_settimana);
    puts("}");
    free(segno);
    libera_ricerca(r);
    return 0;
}

// ---- la scuola e la verifica di un orario fatto da altri ----

// «verifica»: legge un orario da stdin e lo ricontrolla. 0, o 2 se non e' un orario di questo caso.
static int verifica_un_orario(const Scuola *s) {
    const int celle = s->giorni * s->ore * s->classi;
    int *orario = malloc(sizeof(int) * celle);
    for (int i = 0; orario && i < celle; i++)
        if (scanf("%d", &orario[i]) != 1 || orario[i] >= s->voci) {
            free(orario);
            orario = NULL;
        }
    if (!orario) return 2;
    fputc('{', stdout);
    scrivi_orario_e_verifica(stdout, s, orario);
    puts("}");
    free(orario);
    return 0;
}

static int usa(const char *programma) {
    fprintf(stderr, "Uso:\n  %s genera SEME CLASSI RITARDI\n  %s risolvi CASO [--limite NODI] [--stato FILE] "
            "[--passi FILE] [--continua | --da STRADA]\n  %s nodo CASO < NODO\n  %s scelte CASO < STRADA\n"
            "  %s scuola CASO\n  %s verifica CASO < ORARIO\n", programma, programma, programma, programma, programma,
            programma);
    return 2;
}

int main(const int argc, char **argv) {
    // «genera» non legge nessun caso: si fa subito
    if (argc == 5 && !strcmp(argv[1], "genera"))
        return genera_caso(stdout, strtoull(argv[2], NULL, 10), atoi(argv[3]), atoi(argv[4])) ? 0 : 2;
    const int vuole_un_nodo = argc == 3 && !strcmp(argv[1], "nodo");
    const int vuole_le_scelte = argc == 3 && !strcmp(argv[1], "scelte");
    const int vuole_la_scuola = argc == 3 && !strcmp(argv[1], "scuola");
    const int vuole_la_verifica = argc == 3 && !strcmp(argv[1], "verifica");
    const int altro_comando = vuole_un_nodo || vuole_le_scelte || vuole_la_scuola || vuole_la_verifica;
    if (!altro_comando && (argc < 3 || strcmp(argv[1], "risolvi"))) return usa(argv[0]);
    // le opzioni di «risolvi»: ognuna con il suo valore dopo
    const char *file_stato = NULL, *file_passi = NULL, *file_strada = NULL;
    long long limite = 0;
    int continua = 0;
    for (int i = 3; i < argc; i += 2) {
        if (!strcmp(argv[i], "--continua")) {  // l'unica senza valore: la prossima opzione e' subito dopo
            continua = 1;
            i--;
            continue;
        }
        if (i + 1 == argc) return usa(argv[0]);
        if (!strcmp(argv[i], "--limite")) limite = atoll(argv[i + 1]);
        else if (!strcmp(argv[i], "--stato")) file_stato = argv[i + 1];
        else if (!strcmp(argv[i], "--passi")) file_passi = argv[i + 1];
        else if (!strcmp(argv[i], "--da")) file_strada = argv[i + 1];
        else return usa(argv[0]);
    }
    // tutti gli altri comandi cominciano leggendo il caso
    char errore[256];
    Scuola *s = leggi_scuola(argv[2], errore, sizeof errore);
    if (!s) {
        fprintf(stderr, "%s\n", errore);
        return 2;
    }
    if (altro_comando) {
        int uscita = 0;
        if (vuole_un_nodo) uscita = nodo(s);
        else if (vuole_le_scelte) uscita = scelte_di_una_strada(s);
        else if (vuole_la_verifica) uscita = verifica_un_orario(s);
        else {  // la scuola
            printf("{\"celle\":%d,", s->giorni * s->ore * s->classi);
            scrivi_la_scuola(stdout, s);
            puts("}");
        }
        free(s);
        return uscita;
    }
    // «risolvi»: la ricerca, con chi la segue (i passi e lo stato su file, se chiesti)
    Ricerca *r = nuova_ricerca(s);
    Seguito seguito = {NULL, file_stato, clock(), clock(), 0};
    // per continuare: la strada dell'ultimo nodo gia' visitato, letta dai passi
    int *strada = r && continua ? malloc(sizeof(int) * 2 * r->celle) : NULL, profondita = 0;
    long long gia_fatti = 0;
    if (r && continua && file_passi && strada) {
        seguito.passi = passi_riapri(file_passi, r->celle, &gia_fatti, strada, &profondita);
        if (seguito.passi && !leggi_la_fine(file_passi, r, gia_fatti, &seguito.secondi_prima)) {
            passi_chiudi(seguito.passi);
            seguito.passi = NULL;
        }
    } else if (r && file_passi && !continua)
        seguito.passi = passi_apri(file_passi, r->celle);
    if (!r || (file_passi && !seguito.passi) || (continua && !seguito.passi)) {
        fprintf(stderr, continua ? "non riesco a continuare: servono i passi di una ricerca fermata dal limite di nodi\n"
                                 : "non riesco a preparare la ricerca\n");
        return 2;
    }
    // la ricerca chiama `segui` a ogni nodo: e' li' che si scrivono i passi e, ogni tanto, lo stato
    r->limite = limite;
    r->a_ogni_nodo = segui;
    r->di_chi = &seguito;
    if (file_stato) stato_su_file(&seguito, r);  // subito uno stato «in corso»: il sito ha gia' la scuola da mostrare
    if (file_strada && !continua) {
        // da una strada scelta a mano
        FILE *f = fopen(file_strada, "r");
        int n = 0, *sua = f ? leggi_la_strada(f, r, &n) : NULL;
        if (f) fclose(f);
        a_mano = 1;
        if (!sua || !cerca_da_una_strada(r, sua, n)) {
            fprintf(stderr, "non riesco a partire da quella strada: non e' una strada possibile per questo caso\n");
            return 2;
        }
        free(sua);
    } else if (!continua) cerca_orario(r);
    else if (!riprendi_la_ricerca(r, strada, profondita)) {
        fprintf(stderr, "non riesco a continuare: i passi non sono di questo caso\n");
        return 2;
    }
    free(strada);
    passi_chiudi(seguito.passi);
    if (file_passi) scrivi_la_fine(file_passi, r, secondi_da(&seguito));
    // lo stato finale: nel file se c'e', se no su stdout
    if (file_stato) stato_su_file(&seguito, r);
    else scrivi_stato(stdout, r, secondi_da(&seguito), a_mano);
    libera_ricerca(r);
    free(s);
    return 0;
}
