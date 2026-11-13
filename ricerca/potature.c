#include "potature.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Le potature: vedi potature.h. Una funzione per potatura, con lo stesso nome del controllo della verifica dei dati
// che rifa'.

// Una lezione che la classe deve fare: un docente da solo (q < 0) o una coppia (p < q). ip e iq: il posto dei due
// tra i docenti della classe; ore: quante ne ha in tutto.
typedef struct {
    int p, q, ip, iq, ore;
} Voce;

// Quello che un docente puo' ancora fare oggi in una classe (vedi potature.h): da che ora puo' cominciare e quante ore
// al massimo. Fino all'ora h compresa ne puo' fare fino_a(oggi, h): quelle tra `comincia` e h, non piu' di `massimo`.
typedef struct {
    int comincia, massimo;
} Oggi;

// Una risposta della potatura 7 gia' data: le due impronte dello stato della classe, e se un orario suo c'era.
// (0, 0): posto vuoto.
struct Ricordo {
    uint64_t impronta1, impronta2;
    int ce;
};
#define RICORDI (1 << 18)  // quante risposte si tengono: una nuova con lo stesso posto prende il posto della vecchia

// Per ogni classe, i suoi docenti e le sue voci; per ogni docente, le sue classi: non cambiano durante la ricerca.
// In piu' un appunto che si riscrive a ogni nodo: quello che ogni docente puo' fare oggi in ogni sua classe, calcolato
// la prima volta che una potatura lo chiede in quel nodo (lo chiedono in quattro).
struct Potature {
    int *docenti, n_docenti[CLASSE_SEZIONE_SZ];  // docenti[k * INSEGNANTE_SZ + i]: l'i-esimo docente della classe k
    int *classi, n_classi[INSEGNANTE_SZ];        // classi[p * CLASSE_SEZIONE_SZ + i]: l'i-esima classe del docente p
    int *posto;                                  // posto[p * CLASSE_SEZIONE_SZ + i]: il suo posto tra i docenti di lei
    Voce *voci;                                  // voci[inizio_voci[k] + i]: l'i-esima voce della classe k
    int inizio_voci[CLASSE_SEZIONE_SZ + 1];
    // l'appunto: per il docente al posto i della classe k, oggi[inizio_docenti[k] + i], valido se nodo[...] e'
    // adesso, il numero di questa chiamata di potatura
    Oggi *oggi;
    long long int *nodo, adesso;
    // il ricordo della potatura 7 (vedi la_classe_ha_un_orario_suo_nel_resto): le risposte gia' date
    struct Ricordo *ricordo;
    int inizio_docenti[CLASSE_SEZIONE_SZ + 1];
};

void prepara_potature(Ricerca *r) {
    const Scuola *s = r->scuola;
    const int C = s->actual_classi_sezione, T = s->actual_insegnanti;
    struct Potature *t = calloc(1, sizeof *t);
    assert(t);
    t->docenti = malloc(sizeof(int) * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ);
    t->classi = malloc(sizeof(int) * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ);
    t->posto = malloc(sizeof(int) * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ);
    assert(t->docenti && t->classi && t->posto);
    // i docenti di ogni classe e le classi di ogni docente; intanto si contano le voci
    int n_voci = 0;
    for (int k = 0; k < C; k++) {
        t->inizio_docenti[k + 1] = t->inizio_docenti[k];
        for (int p = 0; p < T; p++) {
            if (!get_ore_insegnante_in_una_classe(s, k, p)) continue;
            t->classi[p * CLASSE_SEZIONE_SZ + t->n_classi[p]] = k;
            t->posto[p * CLASSE_SEZIONE_SZ + t->n_classi[p]++] = t->n_docenti[k];
            t->docenti[k * INSEGNANTE_SZ + t->n_docenti[k]++] = p;
            t->inizio_docenti[k + 1]++;
            n_voci += get_ore_insegnante_da_solo_in_una_classe(s, k, p) > 0;
            for (int q = p + 1; q < T; q++) n_voci += get_ore_insegnanti_codocenza_in_una_classe(s, k, p, q) > 0;
        }
    }
    // le voci di ogni classe: i suoi docenti da soli e a coppie
    t->voci = malloc(sizeof(Voce) * (n_voci + 1));
    t->oggi = malloc(sizeof(Oggi) * (t->inizio_docenti[C] + 1));
    t->nodo = calloc(t->inizio_docenti[C] + 1, sizeof(long long int));
    t->ricordo = calloc(RICORDI, sizeof(struct Ricordo));
    assert(t->voci && t->oggi && t->nodo && t->ricordo);
    n_voci = 0;
    for (int k = 0; k < C; k++) {
        t->inizio_voci[k] = n_voci;
        for (int ip = 0; ip < t->n_docenti[k]; ip++) {
            const int p = t->docenti[k * INSEGNANTE_SZ + ip], solo = get_ore_insegnante_da_solo_in_una_classe(s, k, p);
            if (solo) t->voci[n_voci++] = (Voce){p, -1, ip, -1, solo};
            for (int iq = ip + 1; iq < t->n_docenti[k]; iq++) {
                const int q = t->docenti[k * INSEGNANTE_SZ + iq];
                const int insieme = get_ore_insegnanti_codocenza_in_una_classe(s, k, p, q);
                if (insieme) t->voci[n_voci++] = (Voce){p, q, ip, iq, insieme};
            }
        }
    }
    t->inizio_voci[C] = n_voci;
    r->potature = t;
}

void libera_potature(Ricerca *r) {
    if (!r->potature) return;
    free(r->potature->docenti);
    free(r->potature->classi);
    free(r->potature->posto);
    free(r->potature->voci);
    free(r->potature->oggi);
    free(r->potature->nodo);
    free(r->potature->ricordo);
    free(r->potature);
    r->potature = NULL;
}

// ---- il punto della ricerca: la cella pos e quello che serve a tutte le potature ----

typedef struct {
    const Ricerca *r;
    const Scuola *s;
    int giorno, ora, cla;  // la cella pos, la prima non decisa
    int G, O, C, M;        // giorni, ore al giorno, classi, ore obbligatorie
    int dopo;              // i giorni dopo oggi
    // Che cosa guardare (vedi potatura): le classi per le potature 1, 4 e 5, i docenti per la 3 e per la 2.
    int classi[CLASSE_SEZIONE_SZ], n_classi;
    int docenti_classi[INSEGNANTE_SZ], n_docenti_classi;
    int docenti_ore[INSEGNANTE_SZ], n_docenti_ore;
    int classi_decise[CLASSE_SEZIONE_SZ], n_classi_decise;  // per la 7: le classi che hanno una cella decisa in piu'
    Scatto *perche;  // se c'e', la potatura che non passa ci scrive i suoi numeri
} Punto;

// Il conto non torna: scrive i numeri (se qualcuno li vuole) e ritorna 0, il valore della potatura che non passa.
static int non_passa(const Punto *x, const int docente, const int classe, const int ora, const int servono,
                     const int possibili) {
    if (x->perche) *x->perche = (Scatto){-1, docente, classe, ora, servono, possibili};
    return 0;
}

// la prima ora di oggi non ancora decisa nella classe k
static int prima_ora_libera(const Punto *x, const int k) {
    return x->ora + (k < x->cla);
}

static int ingresso(const Punto *x, const int p) {
    return x->s->ora_di_entrata_prof[p];
}

// le ore che restano al docente p nella classe k
static int ore_rimaste_in_classe(const Punto *x, const int k, const int p) {
    return x->s->ore_classi_docenti[k * INSEGNANTE_SZ + p] - x->r->ore_classi_docenti_svolte[k * INSEGNANTE_SZ + p];
}

// le ore che restano al docente p in tutte le classi: e' la sua priorita' nella coda
static int ore_rimaste_in_tutto(const Punto *x, const int p) {
    return x->r->queue_items[p].priority;
}

// il docente p oggi all'ora h insegna gia' (in una cella decisa)
static int impegnato(const Punto *x, const int h, const int p) {
    return x->r->prof_gia_impegnato_giorno_ora[(x->giorno * ORA_GIORNALIERA_SZ + h) * INSEGNANTE_SZ + p];
}

static int ha_ora_buca_oggi(const Punto *x, const int k) {
    return x->r->n_ore_buche[x->giorno * CLASSE_SEZIONE_SZ + k] > 0;
}

// Quello che puo' ancora fare oggi nella classe k il docente che sta al posto i tra i suoi (vedi potature.h): calcolato
// una volta per nodo, poi letto dall'appunto.
static Oggi calcola_oggi(const Punto *x, const int k, const int p) {
    const Ricerca *r = x->r;
    const int da = prima_ora_libera(x, k);
    if (da >= x->O || ha_ora_buca_oggi(x, k)) return (Oggi){da, 0};
    const int fatte = r->ore_fatte_da_un_prof_in_una_classe_in_giorno[
        (x->giorno * CLASSE_SEZIONE_SZ + k) * INSEGNANTE_SZ + p];
    if (fatte) {  // oggi qui ha gia' insegnato: solo continuando subito, dall'ora prima
        const teaching_slot prima = r->orario[(x->giorno * x->O + da - 1) * x->C + k];
        if (prima.insegnante1 != p && prima.insegnante2 != p) return (Oggi){da, 0};  // il suo blocco si e' chiuso
        if (impegnato(x, da, p)) return (Oggi){da, 0};                               // e adesso e' in un'altra classe
        return (Oggi){da, MAX_ORE_PROF_IN_UNA_CLASSE - fatte};
    }
    if (r->classi_fatte_da_un_prof_in_giorno[x->giorno * INSEGNANTE_SZ + p]
        >= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) return (Oggi){da, 0};  // oggi niente classi nuove
    int comincia = MAX(da, ingresso(x, p));
    if (comincia < x->O && impegnato(x, comincia, p)) comincia++;  // a quell'ora e' in un'altra classe
    return (Oggi){comincia, MAX_ORE_PROF_IN_UNA_CLASSE};
}

static Oggi puo_fare_oggi(const Punto *x, const int k, const int i) {
    struct Potature *t = x->r->potature;
    const int dove = t->inizio_docenti[k] + i;
    if (t->nodo[dove] != t->adesso) {
        t->oggi[dove] = calcola_oggi(x, k, t->docenti[k * INSEGNANTE_SZ + i]);
        t->nodo[dove] = t->adesso;
    }
    return t->oggi[dove];
}

static int fino_a(const Oggi oggi, const int h) {
    return MIN(oggi.massimo, MAX(0, h - oggi.comincia + 1));
}

// ---- le potature ----
// Ognuna guarda le classi o i docenti che il punto le indica (tutti, o solo quelli per cui qualcosa e' cambiato).

// le ore che restano a una voce della classe k
static int ore_rimaste_alla_voce(const Punto *x, const int k, const Voce v) {
    const Ricerca *r = x->r;
    if (v.q < 0) return v.ore - r->ore_classi_docenti_alone_svolte[k * INSEGNANTE_SZ + v.p];
    return v.ore - r->ore_classi_codocenza_svolte[k * INSEGNANTE_SZ * INSEGNANTE_SZ + v.p * INSEGNANTE_SZ + v.q];
}

// 1. Un docente in una classe: le ore che gli restano li' stanno in quelle che ci puo' ancora fare, oggi
// (puo_fare_oggi) e nei giorni dopo (al massimo 3 al giorno, e meno se entra tardi).
static int le_ore_del_docente_in_una_classe_stanno_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int j = 0; j < x->n_classi; j++) {
        const int k = x->classi[j];
        for (int i = 0; i < t->n_docenti[k]; i++) {
            const int p = t->docenti[k * INSEGNANTE_SZ + i], restano = ore_rimaste_in_classe(x, k, p);
            if (!restano) continue;
            const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, x->O - ingresso(x, p));
            const int possibili = x->dopo * al_giorno + fino_a(puo_fare_oggi(x, k, i), x->O - 1);
            if (restano > possibili) return non_passa(x, p, k, -1, restano, possibili);
        }
    }
    return 1;
}

// 2. Un docente in tutto: le ore che gli restano stanno nelle ore libere che ha ancora, oggi (dal suo ingresso, da
// quest'ora, dove non insegna gia') e nei giorni dopo (tutte le ore dal suo ingresso).
static int le_ore_del_docente_stanno_nel_resto(const Punto *x) {
    for (int j = 0; j < x->n_docenti_ore; j++) {
        const int p = x->docenti_ore[j], restano = ore_rimaste_in_tutto(x, p);
        if (!restano) continue;
        const int da = MAX(x->ora, ingresso(x, p));
        int libere = x->dopo * (x->O - ingresso(x, p)) + MAX(0, x->O - da);
        if (da == x->ora && impegnato(x, x->ora, p)) libere--;
        if (restano > libere) return non_passa(x, p, -1, -1, restano, libere);
    }
    return 1;
}

// 3. Un docente e le classi al giorno: tolto quello che puo' ancora fare oggi in ogni classe, quello che resta lo
// porta in quella classe in almeno (ore / 3, arrotondato in su) dei giorni dopo; in ognuno di quei giorni insegna in
// al massimo 3 classi.
static int le_classi_del_docente_stanno_in_tre_al_giorno_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int j = 0; j < x->n_docenti_classi; j++) {
        const int p = x->docenti_classi[j];
        int servono = 0;
        for (int i = 0; i < t->n_classi[p]; i++) {
            const int k = t->classi[p * CLASSE_SEZIONE_SZ + i], restano = ore_rimaste_in_classe(x, k, p);
            if (!restano) continue;
            const int qui = t->posto[p * CLASSE_SEZIONE_SZ + i];  // il suo posto tra i docenti di k
            const int dopo_oggi = restano - fino_a(puo_fare_oggi(x, k, qui), x->O - 1);
            if (dopo_oggi > 0) servono += (dopo_oggi + MAX_ORE_PROF_IN_UNA_CLASSE - 1) / MAX_ORE_PROF_IN_UNA_CLASSE;
        }
        const int possibili = MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO * x->dopo;
        if (servono > possibili) return non_passa(x, p, -1, -1, servono, possibili);
    }
    return 1;
}

// 4. Una classe in tutto: le ore che le restano stanno nelle lezioni che puo' ancora avere, oggi e nei giorni dopo.
// Oggi: non piu' delle ore che le restano oggi (dalla sua prima ora libera, se non ha gia' un'ora buca) e non piu' di
// quelle che le sue voci possono ancora fare oggi (puo_fare_oggi: al massimo 3 ore per docente, solo chi e' libero).
// Nei giorni dopo: non piu' delle ore di quei giorni, e ogni voce al massimo 3 al giorno dall'ingresso e non piu' delle
// ore che le restano. Esempio: l'ultimo giorno alla prima ora, una classe con una coppia che ha ancora 4 ore e due
// docenti con un'ora ha 6 ore da fare in 6 ore, ma la coppia oggi ne puo' fare 3: 5 lezioni, la sesta ora resta vuota.
static int le_ore_della_classe_stanno_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int j = 0; j < x->n_classi; j++) {
        const int k = x->classi[j], restano = x->s->ore_classi[k] - x->r->ore_classe_svolte[k];
        if (!restano) continue;
        int lezioni_oggi = 0, lezioni_dopo = 0, lezioni_un_giorno = 0;
        for (int i = t->inizio_voci[k]; i < t->inizio_voci[k + 1]; i++) {
            const Voce v = t->voci[i];
            const int ore = ore_rimaste_alla_voce(x, k, v);
            if (!ore) continue;
            int oggi = fino_a(puo_fare_oggi(x, k, v.ip), x->O - 1), entra = ingresso(x, v.p);
            if (v.q >= 0) {
                oggi = MIN(oggi, fino_a(puo_fare_oggi(x, k, v.iq), x->O - 1));
                entra = MAX(entra, ingresso(x, v.q));
            }
            const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, x->O - entra);
            lezioni_oggi += MIN(ore, oggi);
            lezioni_dopo += MIN(ore, x->dopo * al_giorno);
            lezioni_un_giorno += MIN(ore, al_giorno);
        }
        const int posti_oggi = ha_ora_buca_oggi(x, k) ? 0 : x->O - prima_ora_libera(x, k);
        const int possibili = MIN(posti_oggi, lezioni_oggi) + MIN(lezioni_dopo, x->dopo * MIN(x->O, lezioni_un_giorno));
        if (restano > possibili) return non_passa(x, -1, k, -1, restano, possibili);
    }
    return 1;
}

// 5. Una classe e le ore obbligatorie: per ogni ora obbligatoria h, le celle che la classe deve ancora riempire tra la
// prima ora e h (oggi dalla sua prima ora libera, e tutti i giorni dopo) non sono piu' delle lezioni che le sue voci
// possono ancora fare in quelle ore: ogni voce, oggi quello che possono fare i suoi docenti (puo_fare_oggi), nei
// giorni dopo al massimo 3 al giorno dall'ingresso (per una coppia: di chi entra piu' tardi), e in tutto non piu' delle
// ore che le restano.
static int le_lezioni_bastano_per_le_ore_obbligatorie_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int j = 0; j < x->n_classi; j++) {
        const int k = x->classi[j], da = prima_ora_libera(x, k);
        // di ogni voce che ha ancora ore, una volta sola: le ore, quello che i suoi docenti possono fare oggi e
        // l'ingresso
        const int n_voci = t->inizio_voci[k + 1] - t->inizio_voci[k];
        int restano[n_voci + 1], entra[n_voci + 1], n = 0;
        Oggi oggi_p[n_voci + 1], oggi_q[n_voci + 1];
        for (int i = t->inizio_voci[k]; i < t->inizio_voci[k + 1]; i++) {
            const Voce v = t->voci[i];
            restano[n] = ore_rimaste_alla_voce(x, k, v);
            if (!restano[n]) continue;
            oggi_p[n] = oggi_q[n] = puo_fare_oggi(x, k, v.ip);
            entra[n] = ingresso(x, v.p);
            if (v.q >= 0) {
                oggi_q[n] = puo_fare_oggi(x, k, v.iq);
                entra[n] = MAX(entra[n], ingresso(x, v.q));
            }
            n++;
        }
        for (int h = 0; h < x->M; h++) {
            const int celle = x->dopo * (h + 1) + MAX(0, h - da + 1);
            int servono = celle;
            for (int i = 0; i < n && servono > 0; i++) {
                const int oggi = MIN(fino_a(oggi_p[i], h), fino_a(oggi_q[i], h));
                const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, h + 1 - entra[i]));
                servono -= MIN(restano[i], oggi + x->dopo * al_giorno);
            }
            if (servono > 0) return non_passa(x, -1, k, h, celle, celle - servono);
        }
    }
    return 1;
}

// 6. Ora per ora: a un'ora obbligatoria ogni classe ha lezione e ogni lezione ha almeno un docente tutto suo. I
// docenti che hanno ancora ore e a quell'ora sono gia' entrati devono essere almeno quante le classi: a quest'ora di
// oggi le classi ancora da decidere e i docenti non gia' impegnati; nelle ore dopo di oggi, e nei giorni dopo, tutte
// le classi. (Guarda sempre tutti i docenti: e' un conto solo.)
static int i_docenti_liberi_bastano_per_le_classi_nel_resto(const Punto *x) {
    int entrati_entro[ORA_GIORNALIERA_SZ + 1] = {0}, liberi_adesso = 0;  // con ore da fare, per ora di ingresso
    for (int p = 0; p < x->s->actual_insegnanti; p++) {
        if (!ore_rimaste_in_tutto(x, p)) continue;
        entrati_entro[ingresso(x, p)]++;
        liberi_adesso += ingresso(x, p) <= x->ora && !impegnato(x, x->ora, p);
    }
    if (x->ora < x->M && liberi_adesso < x->C - x->cla)
        return non_passa(x, -1, -1, x->ora, x->C - x->cla, liberi_adesso);
    for (int h = 0, entrati = 0; h < x->M; h++) {
        entrati += entrati_entro[h];
        if (entrati < x->C && (x->dopo > 0 || h > x->ora)) return non_passa(x, -1, -1, h, x->C, entrati);
    }
    return 1;
}

// 7. L'orario della classe, da sola. Si guarda una classe come se le altre non ci fossero (i suoi docenti sempre
// liberi) e si cerca di riempire le sue celle che restano, da adesso alla fine della settimana, con le ore che restano
// alle sue voci, con le regole che riguardano solo lei: ogni docente dal suo ingresso, al massimo 3 ore al giorno nella
// classe e di seguito; lezione nelle ore obbligatorie; l'ora buca solo dopo, quando le ore che restano stanno nei
// giorni dopo, e poi per quel giorno basta; alla fine nessuna ora avanzata. Se un orario cosi' non esiste, non esiste
// nemmeno quello di tutta la scuola (che per questa classe sarebbe uno di quelli). E' una ricerca piccola dentro la
// ricerca: se non finisce in ORARIO_CLASSE_TENTATIVI passi non si sa, e la potatura passa.
// La risposta dipende solo dallo stato della classe: il giorno e l'ora da cui riparte, le ore che restano a ogni voce
// e, per oggi, le ore gia' fatte da ogni docente e se era li' l'ora prima. Lo stesso stato torna moltissime volte
// (mentre la ricerca cambia le altre classi, o mette le stesse lezioni in un altro ordine nei giorni passati), quindi
// le risposte si ricordano: dello stato si tengono due impronte di 64 bit calcolate in due modi diversi, e la risposta
// vale se coincidono tutte e due (che due stati diversi le abbiano uguali tutte e due non succede in pratica).
#define ORARIO_CLASSE_TENTATIVI 2000

typedef struct {
    const Punto *x;
    int k, n_docenti, n_voci;
    const Voce *voci;
    int *restano;          // le ore che restano a ogni voce
    int *fatte, *ultima;   // nel giorno che si sta riempiendo, per ogni docente della classe: le ore fatte e l'ultima
                           // ora in cui ha insegnato (-9: mai)
    int ore_classe;        // le ore che restano alla classe
    int tentativi;         // quanti passi restano
} OrarioClasse;

// il docente al posto i tra quelli della classe puo' insegnare all'ora h del giorno che si sta riempiendo
static int puo_qui(const OrarioClasse *u, const int i, const int h) {
    const int p = u->x->r->potature->docenti[u->k * INSEGNANTE_SZ + i];
    return ingresso(u->x, p) <= h && u->fatte[i] < MAX_ORE_PROF_IN_UNA_CLASSE
           && (u->fatte[i] == 0 || u->ultima[i] == h - 1);
}

static int riempi(OrarioClasse *u, int g, int h);

// si passa al giorno dopo (i docenti ripartono da zero); se di li' non si riesce, il giorno torna com'era
static int giorno_dopo(OrarioClasse *u, const int g) {
    int fatte[u->n_docenti + 1], ultima[u->n_docenti + 1];
    memcpy(fatte, u->fatte, sizeof(int) * u->n_docenti);
    memcpy(ultima, u->ultima, sizeof(int) * u->n_docenti);
    for (int i = 0; i < u->n_docenti; i++) u->fatte[i] = 0, u->ultima[i] = -9;
    if (riempi(u, g + 1, 0)) return 1;
    memcpy(u->fatte, fatte, sizeof(int) * u->n_docenti);
    memcpy(u->ultima, ultima, sizeof(int) * u->n_docenti);
    return 0;
}

// Riempie le celle della classe dal giorno g, ora h in poi: 1 se ci riesce (o se i tentativi sono finiti).
static int riempi(OrarioClasse *u, const int g, const int h) {
    const Punto *x = u->x;
    if (g == x->G) return u->ore_classe == 0;
    if (h == x->O) return giorno_dopo(u, g);
    if (--u->tentativi < 0) return 1;
    // le stesse due domande della ricerca (ricerca.c): la classe puo' avere lezione? puo' avere un'ora buca?
    const int obbligatorie = x->M * x->G - (g * x->M + MIN(h, x->M - 1));
    if (u->ore_classe >= obbligatorie) {
        for (int i = 0; i < u->n_voci; i++) {
            const Voce v = u->voci[i];
            if (!u->restano[i] || !puo_qui(u, v.ip, h) || (v.iq >= 0 && !puo_qui(u, v.iq, h))) continue;
            const int prima_p = u->ultima[v.ip], prima_q = v.iq >= 0 ? u->ultima[v.iq] : 0;
            u->restano[i]--, u->ore_classe--, u->fatte[v.ip]++, u->ultima[v.ip] = h;
            if (v.iq >= 0) u->fatte[v.iq]++, u->ultima[v.iq] = h;
            if (riempi(u, g, h + 1)) return 1;
            u->restano[i]++, u->ore_classe++, u->fatte[v.ip]--, u->ultima[v.ip] = prima_p;
            if (v.iq >= 0) u->fatte[v.iq]--, u->ultima[v.iq] = prima_q;
        }
    }
    if (h >= x->M && u->ore_classe <= (x->G - 1 - g) * x->O) return giorno_dopo(u, g);
    return 0;
}

static int la_classe_ha_un_orario_suo_nel_resto(const Punto *x) {
    const struct Potature *t = x->r->potature;
    for (int j = 0; j < x->n_classi_decise; j++) {
        const int k = x->classi_decise[j], da = prima_ora_libera(x, k);
        const int n_docenti = t->n_docenti[k], n_voci = t->inizio_voci[k + 1] - t->inizio_voci[k];
        int restano[n_voci + 1], fatte[n_docenti + 1], ultima[n_docenti + 1];
        OrarioClasse u = {x, k, n_docenti, n_voci, &t->voci[t->inizio_voci[k]], restano, fatte, ultima,
                          x->s->ore_classi[k] - x->r->ore_classe_svolte[k], ORARIO_CLASSE_TENTATIVI};
        for (int i = 0; i < n_voci; i++) restano[i] = ore_rimaste_alla_voce(x, k, u.voci[i]);
        // oggi com'e' davvero: le ore gia' fatte da ogni docente nella classe, e se era li' nell'ultima ora decisa
        const teaching_slot prima = da > 0 ? x->r->orario[(x->giorno * x->O + da - 1) * x->C + k]
                                           : (teaching_slot){-1, -1};
        for (int i = 0; i < n_docenti; i++) {
            const int p = t->docenti[k * INSEGNANTE_SZ + i];
            fatte[i] = x->r->ore_fatte_da_un_prof_in_una_classe_in_giorno[
                (x->giorno * CLASSE_SEZIONE_SZ + k) * INSEGNANTE_SZ + p];
            ultima[i] = prima.insegnante1 == p || prima.insegnante2 == p ? da - 1 : -9;
        }
        // da dove riparte: oggi dalla sua prima ora libera, o (giornata finita: ora buca, o ultima ora decisa) dal
        // giorno dopo, con i docenti che ripartono da zero
        int g = x->giorno, h = da;
        if (ha_ora_buca_oggi(x, k) || da >= x->O) {
            g++, h = 0;
            for (int i = 0; i < n_docenti; i++) fatte[i] = 0, ultima[i] = -9;
        }
        // lo stato, un byte per numero, e le sue due impronte
        unsigned char stato[4 + n_voci + n_docenti];
        int n = 0;
        stato[n++] = k & 255, stato[n++] = k >> 8, stato[n++] = g, stato[n++] = h;
        for (int i = 0; i < n_voci; i++) stato[n++] = restano[i];
        for (int i = 0; i < n_docenti; i++) stato[n++] = fatte[i] << 1 | (ultima[i] == h - 1);
        uint64_t impronta1 = 1469598103934665603ULL, impronta2 = 0x9E3779B97F4A7C15ULL;
        for (int i = 0; i < n; i++) {
            impronta1 = (impronta1 ^ stato[i]) * 1099511628211ULL;
            impronta2 = (impronta2 + stato[i] + 1) * 0xD6E8FEB86659FD93ULL;
            impronta2 ^= impronta2 >> 32;
        }
        struct Ricordo *ricordo = &t->ricordo[impronta1 & (RICORDI - 1)];
        int ce;
        if (ricordo->impronta1 == impronta1 && ricordo->impronta2 == impronta2) ce = ricordo->ce;
        else {
            ce = riempi(&u, g, h);
            *ricordo = (struct Ricordo){impronta1, impronta2, ce};
        }
        if (!ce) return non_passa(x, -1, k, -1, x->s->ore_classi[k] - x->r->ore_classe_svolte[k], 0);
    }
    return 1;
}

// ---- che cosa guardare ----
// Ogni nodo della strada fin qui ha passato le potature al suo ingresso. Quindi a questo nodo basta rifare i conti che
// la scelta appena messa (nella cella pos - 1) ha cambiato; gli altri valgono ancora.
// Che cosa cambia mettendo una scelta nella cella della classe k all'ora di adesso:
// - la classe k: ha un'ora decisa in piu' (la sua prima ora libera avanza), e con un'ora buca ha finito la giornata;
// - i docenti messi: un'ora in meno da fare, a quest'ora sono impegnati, oggi hanno insegnato in k. Cambia quello che
//   possono fare oggi nelle altre loro classi, e quindi i conti di quelle classi;
// - gli altri docenti di k: in k la prima ora libera e' avanzata (chi era li' l'ora prima ha chiuso il suo blocco).
// Quando comincia un'ora nuova a tutti i docenti resta un'ora libera in meno (la potatura 2 li guarda tutti); quando
// comincia un giorno nuovo cambia tutto, e si guarda tutto.

static void aggiungi_classe(Punto *x, char *vista, const int k) {
    if (vista[k]) return;
    vista[k] = 1;
    x->classi[x->n_classi++] = k;
}

static void guarda_tutto(Punto *x) {
    for (int k = 0; k < x->C; k++) x->classi[k] = x->classi_decise[k] = k;
    x->n_classi = x->n_classi_decise = x->C;
    for (int p = 0; p < x->s->actual_insegnanti; p++) x->docenti_classi[p] = x->docenti_ore[p] = p;
    x->n_docenti_classi = x->n_docenti_ore = x->s->actual_insegnanti;
}

static void guarda_quello_che_e_cambiato(Punto *x, const int pos) {
    const struct Potature *t = x->r->potature;
    const int k = (pos - 1) % x->C;                         // la classe della cella appena decisa
    const teaching_slot messi = x->r->orario[pos - 1];
    const int docenti[2] = {messi.insegnante1, messi.insegnante2};
    char vista[CLASSE_SEZIONE_SZ];
    memset(vista, 0, x->C);
    aggiungi_classe(x, vista, k);
    x->classi_decise[x->n_classi_decise++] = k;
    for (int d = 0; d < 2; d++) {
        const int p = docenti[d];
        if (p < 0) continue;
        for (int i = 0; i < t->n_classi[p]; i++) aggiungi_classe(x, vista, t->classi[p * CLASSE_SEZIONE_SZ + i]);
        x->docenti_ore[x->n_docenti_ore++] = p;
    }
    // la potatura 3 (le classi al giorno): tutti i docenti di k, che comprendono quelli messi
    for (int i = 0; i < t->n_docenti[k]; i++)
        x->docenti_classi[x->n_docenti_classi++] = t->docenti[k * INSEGNANTE_SZ + i];
    if (x->cla == 0) {  // comincia un'ora nuova: la potatura 2 per tutti
        for (int p = 0; p < x->s->actual_insegnanti; p++) x->docenti_ore[p] = p;
        x->n_docenti_ore = x->s->actual_insegnanti;
    }
}

// Le potature accese con le celle fino a pos - 1 decise. tutto: si rifanno tutti i conti, non solo quelli cambiati.
static int guarda_le_potature(const Ricerca *r, const int pos, const int tutto, Scatto *perche) {
    const Scuola *s = r->scuola;
    const int G = s->actual_giorni_settimana, O = s->actual_ore_giornaliere, C = s->actual_classi_sezione;
    Punto x;  // (non azzerato: gli elenchi sono lunghi, e si leggono solo fin dove sono scritti)
    x.r = r, x.s = s, x.giorno = pos / C / O, x.ora = pos / C % O, x.cla = pos % C, x.G = G, x.O = O, x.C = C;
    x.M = MIN(PRIME_N_ORE_OBBLIGATORIE, O), x.dopo = G - 1 - x.giorno;
    x.n_classi = x.n_docenti_classi = x.n_docenti_ore = x.n_classi_decise = 0;
    x.perche = perche;
    r->potature->adesso++;  // un nodo nuovo: l'appunto di prima non vale piu'
    // la prima cella di un giorno (e della ricerca): tutto. (La 7 per una classe cambia solo quando si decide una
    // sua cella: all'inizio di un giorno basterebbe la classe appena decisa, ma all'inizio della ricerca servono
    // tutte.)
    if (tutto || pos % (C * O) == 0) guarda_tutto(&x);
    else guarda_quello_che_e_cambiato(&x, pos);
    int (*const controlli[N_POTATURE])(const Punto *) = {
        le_ore_del_docente_in_una_classe_stanno_nel_resto, le_ore_del_docente_stanno_nel_resto,
        le_classi_del_docente_stanno_in_tre_al_giorno_nel_resto, le_ore_della_classe_stanno_nel_resto,
        le_lezioni_bastano_per_le_ore_obbligatorie_nel_resto, i_docenti_liberi_bastano_per_le_classi_nel_resto,
        la_classe_ha_un_orario_suo_nel_resto,
    };
    for (int i = 0; i < N_POTATURE; i++) {
        if (!(r->potature_attive >> i & 1) || controlli[i](&x)) continue;
        if (perche) perche->quale = i;
        return i;
    }
    return -1;
}

int potatura(const Ricerca *r, const int pos) {
    return guarda_le_potature(r, pos, 0, NULL);
}

int potatura_spiegata(const Ricerca *r, const int pos, Scatto *perche) {
    return guarda_le_potature(r, pos, 1, perche);
}
