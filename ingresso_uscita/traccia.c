#include "../potature/potature.h"
#include "traccia.h"

#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

// ---- quello che serve di main.c (la ricerca e i suoi controlli: si richiamano, non si copiano) ----
extern teaching_slot orario[];
extern int orario_len;
extern long long int n_gen;
extern long long int n_gen_max;

int g_ora(int p);
int g_classe(int p);
int activities_for_this_class_and_hour(int pos, int gio, int ora, int cla);
int sabato_esaurisci_priorita_subito_cod(int pos, int giorno, int ora, int base2d_ci, int prof1, int prof2);
int sabato_esaurisci_priorita_subito(int pos, int giorno, int ora, int base2d_ci, int prof);
int prof_fa_al_max_n_ore(int giorno, int ora, int cla, int prof);
int prof_fa_ore_contigue(int giorno, int ora, int cla, int prof);
int prof_è_disponibile(int base3d_goi, int p1);
int deve_ancora_svolgere_ore_in_questa_classe(int base2d_ci, int p1);
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(int cla, int p1, int p2);
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(int base2d_ci, int prof);
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(int pos, int giorno, int cla, int prof);
int verifica_ora_ingresso(int prof, int ora);

#define MIN(a, b) ((a) < (b) ? (a) : (b))

long long int traccia_nodi = 0;
long long int traccia_backtrack_n = 0;
int traccia_con_nodi = 0;

static FILE *t = NULL;
static long long int max_registrati = 1000;
static int con_progresso = 0;
static int orari_scritti = 0;
static clock_t inizio, ultimo_progresso;

#define MAX_ORARI_SCRITTI 10

// ---- i nodi (con --max-nodi): si scrivono a pezzi, al massimo NODI_PER_RIGA per riga ----

#define NODI_PER_RIGA 1000
static long long int max_nodi_registrati = 0, nodi_registrati = 0;
static long long int nodi_da = 0; // il primo nodo del pezzo in corso
static int nodi_pezzo[NODI_PER_RIGA][3];
static int nodi_n = 0;

static void scrivi_nodi(void) {
    if (!t || !nodi_n) return;
    fprintf(t, "{\"tipo\":\"nodi\",\"da\":%lld,\"passi\":[", nodi_da);
    for (int i = 0; i < nodi_n; i++)
        fprintf(t, "%s[%d,%d,%d]", i ? "," : "", nodi_pezzo[i][0], nodi_pezzo[i][1], nodi_pezzo[i][2]);
    fprintf(t, "]}\n");
    nodi_n = 0;
}

void traccia_nodo(const int pos) {
    if (nodi_registrati >= max_nodi_registrati || traccia_backtrack_n > max_registrati) {
        scrivi_nodi();
        traccia_con_nodi = 0; // basta: la ricerca non chiama piu'
        return;
    }
    if (!nodi_n) nodi_da = traccia_nodi;
    nodi_pezzo[nodi_n][0] = pos;
    nodi_pezzo[nodi_n][1] = pos > 0 ? orario[pos - 1].insegnante1 : -1;
    nodi_pezzo[nodi_n][2] = pos > 0 ? orario[pos - 1].insegnante2 : -1;
    nodi_registrati++;
    if (++nodi_n == NODI_PER_RIGA) scrivi_nodi();
}

int traccia_apri(const char *percorso, const long long int max_backtrack, const long long int max_nodi,
                 const int progresso) {
    inizio = ultimo_progresso = clock();
    con_progresso = progresso;
    max_registrati = max_backtrack;
    if (!percorso) return 1;
    t = fopen(percorso, "w");
    max_nodi_registrati = max_nodi;
    traccia_con_nodi = t != NULL && max_nodi > 0;
    return t != NULL;
}

void traccia_chiudi(void) {
    scrivi_nodi();
    if (t) fclose(t);
    t = NULL;
}

static void stringa(const char *s) {
    fputc('"', t);
    for (; *s; s++) {
        const unsigned char c = (unsigned char) *s;
        if (c == '"' || c == '\\') fprintf(t, "\\%c", c);
        else if (c < 0x20) fprintf(t, "\\u%04x", c);
        else fputc(c, t);
    }
    fputc('"', t);
}

// ---- intestazione: i dati letti ----

void traccia_dati(void) {
    if (!t) return;
    fprintf(t, "{\"tipo\":\"dati\",\"giorni\":[");
    for (int i = 0; i < actual_giorni_settimana; i++) {
        if (i) fputc(',', t);
        stringa(giorni_settimana[i]);
    }
    fprintf(t, "],\"ore\":[");
    for (int i = 0; i < actual_ore_giornaliere; i++) {
        if (i) fputc(',', t);
        stringa(ore_giornaliere[i]);
    }
    fprintf(t, "],\"classi\":[");
    for (int c = 0; c < actual_classi_sezione; c++) {
        fprintf(t, "%s{\"nome\":", c ? "," : "");
        stringa(classe_sezione[c]);
        fprintf(t, ",\"ore\":%d}", ore_classi[c]);
    }
    fprintf(t, "],\"docenti\":[");
    for (int p = 0; p < actual_insegnanti; p++) {
        fprintf(t, "%s{\"nome\":", p ? "," : "");
        stringa(insegnanti[p]);
        fprintf(t, ",\"ore\":%d,\"ingresso\":%d}", ore_docenti[p], ora_di_entrata_prof[p]);
    }
    // per ogni classe e docente: ore in tutto, ore da solo, e le codocenze (con chi e quante ore insieme)
    fprintf(t, "],\"cattedre\":[");
    int primo = 1;
    for (int c = 0; c < actual_classi_sezione; c++)
        for (int p = 0; p < actual_insegnanti; p++) {
            const int i = c * INSEGNANTE_SZ + p;
            if (!ore_classi_docenti[i]) continue;
            fprintf(t, "%s{\"classe\":%d,\"docente\":%d,\"ore\":%d,\"da_solo\":%d,\"codocenze\":[", primo ? "" : ",",
                    c, p, ore_classi_docenti[i], ore_classi_docenti_alone[i]);
            primo = 0;
            int pc = 1;
            for (int q = 0; q < actual_insegnanti; q++) {
                const int n = q == p ? 0 : get_ore_insegnanti_codocenza_in_una_classe(c, p, q);
                if (!n) continue;
                fprintf(t, "%s{\"con\":%d,\"ore\":%d}", pc ? "" : ",", q, n);
                pc = 0;
            }
            fprintf(t, "]}");
        }
    fprintf(t, "],\"parametri\":{\"ore_obbligatorie\":%d,\"max_ore_prof_in_una_classe\":%d,"
            "\"max_classi_diverse_al_giorno\":%d}}\n", PRIME_N_ORE_OBBLIGATORIE, MAX_ORE_PROF_IN_UNA_CLASSE,
            MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
    fflush(t);
}

// ---- pezzi di un record ----

static void orario_fino_a(const int pos) {
    fprintf(t, "\"orario\":[");
    for (int i = 0; i < pos; i++) fprintf(t, "%s[%d,%d]", i ? "," : "", orario[i].insegnante1, orario[i].insegnante2);
    fputc(']', t);
}

static void coppia_o_null(const int a, const int b) {
    fprintf(t, "[%d,%d]", a, b);
}

// L'inizio di un controllo nell'elenco: {"nome":...,"ingressi":{  (si chiude con fine_controllo)
static int n_controllo;

static void inizia_controllo(const char *nome) {
    fprintf(t, "%s{\"nome\":\"%s\",\"ingressi\":{", n_controllo++ ? "," : "", nome);
}

static void fine_controllo(const int uscita, int *fermato_a, const int indice) {
    fprintf(t, "},\"uscita\":%d}", uscita ? 1 : 0);
    if (!uscita && *fermato_a < 0) *fermato_a = indice;
}

static int idx_giorno_classe(const int giorno, const int cla, const int prof) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof;
}

// I controlli, ognuno con quello che legge: uscita = il valore della funzione di main.

static int c_disponibile(const int pos, const int giorno, const int ora, const int base3d_goi, const int p,
                         int *fermato_a, const int k) {
    inizia_controllo("prof_è_disponibile");
    const int inizio_ora = giorno * actual_ore_giornaliere * actual_classi_sezione + ora * actual_classi_sezione;
    int in_classe = -1;
    for (int i = inizio_ora; i < pos; i++)
        if (orario[i].insegnante1 == p || orario[i].insegnante2 == p) in_classe = g_classe(i);
    fprintf(t, "\"impegnato_in_quest_ora\":%d,\"in_classe\":%d", prof_gia_impegnato_giorno_ora[base3d_goi + p],
            in_classe);
    const int u = prof_è_disponibile(base3d_goi, p);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_deve_ancora(const int base2d_ci, const int p, int *fermato_a, const int k) {
    inizia_controllo("deve_ancora_svolgere_ore_in_questa_classe");
    fprintf(t, "\"ore_svolte\":%d,\"ore_da_fare\":%d", ore_classi_docenti_svolte[base2d_ci + p],
            ore_classi_docenti[base2d_ci + p]);
    const int u = deve_ancora_svolgere_ore_in_questa_classe(base2d_ci, p);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_max_ore(const int giorno, const int ora, const int cla, const int p, int *fermato_a, const int k) {
    inizia_controllo("prof_fa_al_max_n_ore");
    fprintf(t, "\"ore_oggi_in_classe\":%d,\"massimo\":%d", ore_fatte_da_un_prof_in_una_classe_in_giorno[
                idx_giorno_classe(giorno, cla, p)], MAX_ORE_PROF_IN_UNA_CLASSE);
    const int u = prof_fa_al_max_n_ore(giorno, ora, cla, p);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_contigue(const int giorno, const int ora, const int cla, const int p, int *fermato_a, const int k) {
    inizia_controllo("prof_fa_ore_contigue");
    fprintf(t, "\"ore_oggi_in_classe\":%d,\"ora_precedente\":",
            ore_fatte_da_un_prof_in_una_classe_in_giorno[idx_giorno_classe(giorno, cla, p)]);
    if (ora > 0) {
        const teaching_slot *s = &orario[giorno * actual_ore_giornaliere * actual_classi_sezione +
                                         (ora - 1) * actual_classi_sezione + cla];
        coppia_o_null(s->insegnante1, s->insegnante2);
    } else fprintf(t, "null");
    const int u = prof_fa_ore_contigue(giorno, ora, cla, p);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_max_classi(const int pos, const int giorno, const int cla, const int p, int *fermato_a, const int k) {
    inizia_controllo("ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno");
    // le classi in cui il docente ha gia' insegnato oggi (prima di questo slot), piu' questa
    int classi[CLASSE_SEZIONE_SZ], n = 0;
    classi[n++] = cla;
    for (int i = giorno * actual_ore_giornaliere * actual_classi_sezione; i < pos; i++) {
        if (orario[i].insegnante1 != p && orario[i].insegnante2 != p) continue;
        const int c = g_classe(i);
        int gia = 0;
        for (int j = 0; j < n; j++) gia |= classi[j] == c;
        if (!gia) classi[n++] = c;
    }
    fprintf(t, "\"classi_oggi_con_questa\":[");
    for (int j = 0; j < n; j++) fprintf(t, "%s%d", j ? "," : "", classi[j]);
    fprintf(t, "],\"massimo\":%d", MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
    const int u = ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, giorno, cla, p);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_ingresso(const int ora, const int p, int *fermato_a, const int k) {
    inizia_controllo("verifica_ora_ingresso");
    fprintf(t, "\"ora\":%d,\"ora_di_ingresso\":%d", ora, ora_di_entrata_prof[p]);
    const int u = verifica_ora_ingresso(p, ora);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_insieme(const int cla, const int p1, const int p2, int *fermato_a, const int k) {
    inizia_controllo("docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe");
    fprintf(t, "\"ore_insieme_svolte\":%d,\"ore_insieme_da_fare\":%d",
            get_ore_insegnanti_codocenza_svolte_in_una_classe(cla, p1, p2),
            get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2));
    const int u = docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(cla, p1, p2);
    fine_controllo(u, fermato_a, k);
    return u;
}

// ingressi comuni alle due regole del sabato: chi c'era nell'ora prima e quante ore gli restano nella classe
static void ingressi_sabato(const int pos, const int giorno, const int ora, const int base2d_ci) {
    fprintf(t, "\"ultimo_giorno\":%d,\"ora\":%d", giorno == actual_giorni_settimana - 1, ora);
    if (giorno == actual_giorni_settimana - 1 && ora > 0) {
        const teaching_slot *s = &orario[pos - actual_classi_sezione];
        fprintf(t, ",\"ora_precedente\":[%d,%d],\"ore_rimaste\":[", s->insegnante1, s->insegnante2);
        const int pp[2] = {s->insegnante1, s->insegnante2};
        for (int i = 0; i < 2; i++) {
            if (i) fputc(',', t);
            if (pp[i] < 0) fprintf(t, "null");
            else fprintf(t, "%d", ore_classi_docenti[base2d_ci + pp[i]] - ore_classi_docenti_svolte[base2d_ci + pp[i]]);
        }
        fputc(']', t);
    }
}

static int c_sabato_cod(const int pos, const int giorno, const int ora, const int base2d_ci, const int p1, const int p2,
                        int *fermato_a, const int k) {
    inizia_controllo("sabato_esaurisci_priorita_subito_cod");
    ingressi_sabato(pos, giorno, ora, base2d_ci);
    const int u = sabato_esaurisci_priorita_subito_cod(pos, giorno, ora, base2d_ci, p1, p2);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_sabato(const int pos, const int giorno, const int ora, const int base2d_ci, const int p, int *fermato_a,
                    const int k) {
    inizia_controllo("sabato_esaurisci_priorita_subito");
    ingressi_sabato(pos, giorno, ora, base2d_ci);
    const int u = sabato_esaurisci_priorita_subito(pos, giorno, ora, base2d_ci, p);
    fine_controllo(u, fermato_a, k);
    return u;
}

static int c_senza_cod(const int base2d_ci, const int p, int *fermato_a, const int k) {
    inizia_controllo("deve_svolgere_ancora_ora_in_questa_classe_senza_codocente");
    fprintf(t, "\"ore_da_solo_svolte\":%d,\"ore_da_solo\":%d", ore_classi_docenti_alone_svolte[base2d_ci + p],
            ore_classi_docenti_alone[base2d_ci + p]);
    const int u = deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(base2d_ci, p);
    fine_controllo(u, fermato_a, k);
    return u;
}

static void controllo_slot(const int pos, const int giorno, const int ora, const int cla) {
    const int ore_rimanenti = ore_classi[cla] - ore_classe_svolte[cla];
    const int occupati = giorno * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    const int mancanti = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana - occupati;
    fprintf(t, "\"slot\":{\"nome\":\"activities_for_this_class_and_hour\",\"ingressi\":{\"ore_rimanenti\":%d,"
            "\"slot_obbligatori_mancanti\":%d,\"ore_buche_oggi\":%d},\"uscita\":%d}", ore_rimanenti, mancanti,
            n_ore_buche[giorno * CLASSE_SEZIONE_SZ + cla], activities_for_this_class_and_hour(pos, giorno, ora, cla));
}

static int registra(void) {
    traccia_backtrack_n++;
    return t && traccia_backtrack_n <= max_registrati;
}

static void testa_backtrack(const int pos, const int giorno, const int ora, const int cla, const int buca_dopo) {
    scrivi_nodi();
    fprintf(t, "{\"tipo\":\"backtrack\",\"n\":%lld,\"nodo\":%lld,\"pos\":%d,\"giorno\":%d,\"ora\":%d,\"classe\":%d,"
            "\"buca_dopo\":%d,", traccia_backtrack_n, traccia_nodi, pos, giorno, ora, cla, buca_dopo);
}

// ---- i record ----

void traccia_backtrack(const int pos, const int giorno, const int ora, const int cla, const int base2d_ci,
                       const int base3d_goi, const int base4d_queue, const int buca_dopo) {
    if (!registra()) return;
    testa_backtrack(pos, giorno, ora, cla, buca_dopo);
    controllo_slot(pos, giorno, ora, cla);
    fprintf(t, ",\"docenti\":[");
    int altri = 0, primo = 1, incoerente = 0;
    const int fine_coda = base4d_queue + queue_sz;
    // gli stessi docenti, nello stesso ordine, che la ricerca ha appena provato in questo slot
    for (int ii = base4d_queue; ii < fine_coda; ii++) {
        const QueueItem *it = queue_mul[ii];
        const int p1 = it->teacher;
        if (!ore_classi_docenti[base2d_ci + p1]) {
            altri++; // nessuna ora in questa classe: si ferma al primo o al secondo controllo
            continue;
        }
        fprintf(t, "%s{\"docente\":%d,\"priorita\":%.0f,\"controlli\":[", primo ? "" : ",", p1, it->priority);
        primo = 0;
        // i controlli del primo docente, nell'ordine di main (che si ferma al primo falso)
        int fermato = -1;
        n_controllo = 0;
        c_disponibile(pos, giorno, ora, base3d_goi, p1, &fermato, 0);
        c_deve_ancora(base2d_ci, p1, &fermato, 1);
        c_max_ore(giorno, ora, cla, p1, &fermato, 2);
        c_contigue(giorno, ora, cla, p1, &fermato, 3);
        c_max_classi(pos, giorno, cla, p1, &fermato, 4);
        c_ingresso(ora, p1, &fermato, 5);
        fprintf(t, "],\"fermato_a\":%d", fermato);
        {
            // le coppie con i docenti dopo di lui nella coda, poi da solo. La ricerca le prova solo se il docente passa
            // i controlli sopra (fermato < 0); la traccia le calcola per tutti, cosi' la pagina puo' mostrarle anche
            // per gli altri (sono controlli che leggono soltanto, non cambiano niente)
            fprintf(t, ",\"coppie\":[");
            int pc = 1;
            for (int jj = ii + 1; jj < fine_coda; jj++) {
                const int p2 = queue_mul[jj]->teacher;
                if (!get_ore_insegnanti_codocenza_in_una_classe(cla, p1, p2)) continue; // nessuna codocenza prevista
                int f2 = -1;
                n_controllo = 0;
                fprintf(t, "%s{\"codocente\":%d,\"controlli\":[", pc ? "" : ",", p2);
                pc = 0;
                c_disponibile(pos, giorno, ora, base3d_goi, p2, &f2, 0);
                c_insieme(cla, p1, p2, &f2, 1);
                c_max_ore(giorno, ora, cla, p2, &f2, 2);
                c_contigue(giorno, ora, cla, p2, &f2, 3);
                c_sabato_cod(pos, giorno, ora, base2d_ci, p1, p2, &f2, 4);
                c_max_classi(pos, giorno, cla, p2, &f2, 5);
                c_ingresso(ora, p2, &f2, 6);
                fprintf(t, "],\"fermato_a\":%d}", f2);
                incoerente |= fermato < 0 && f2 < 0;
            }
            int f3 = -1;
            n_controllo = 0;
            fprintf(t, "],\"da_solo\":{\"controlli\":[");
            c_senza_cod(base2d_ci, p1, &f3, 0);
            c_sabato(pos, giorno, ora, base2d_ci, p1, &f3, 1);
            fprintf(t, "],\"fermato_a\":%d}", f3);
            incoerente |= fermato < 0 && f3 < 0;
        }
        fputc('}', t);
    }
    // incoerente: una combinazione risulterebbe valida, ma la ricerca non ne ha trovata nessuna. Non deve succedere: se
    // succede, la traccia non rispecchia la ricerca.
    fprintf(t, "],\"altri_docenti\":%d,\"incoerente\":%d,", altri, incoerente);
    orario_fino_a(pos);
    fprintf(t, "}\n");
    fflush(t);
}

void traccia_slot_chiuso(const int pos, const int giorno, const int ora, const int cla) {
    if (!registra()) return;
    testa_backtrack(pos, giorno, ora, cla, 0);
    controllo_slot(pos, giorno, ora, cla);
    fprintf(t, ",\"docenti\":[],\"altri_docenti\":0,\"incoerente\":0,");
    orario_fino_a(pos);
    fprintf(t, "}\n");
    fflush(t);
}

// Una potatura (vedi potature/potature.h) all'ingresso della cella pos: la scelta nella cella prima lascia un resto
// impossibile. Nella traccia e' un backtrack in questa cella, con il controllo e i suoi numeri.
void traccia_potatura(const int pos, const int giorno, const int ora, const int cla) {
    if (!registra()) return;
    testa_backtrack(pos, giorno, ora, cla, 0);
    fprintf(t, "\"slot\":{\"nome\":\"%s\",\"ingressi\":{\"docente\":%d,\"classe\":%d,\"restano\":%d,"
            "\"possibili\":%d,\"fino_ora\":%d,\"giorno\":%d,\"ora\":%d},\"uscita\":0}",
            potatura.controllo == POTATURA_ORE_DOCENTI ? "ore_dei_docenti_stanno_nel_tempo" : "lezioni_della_classe_bastano",
            potatura.docente, potatura.classe, potatura.restano, potatura.possibili, potatura.fino_ora,
            potatura.giorno, potatura.ora);
    fprintf(t, ",\"docenti\":[],\"altri_docenti\":0,\"incoerente\":0,");
    orario_fino_a(pos);
    fprintf(t, "}\n");
    fflush(t);
}

void traccia_orario_trovato(void) {
    if (!t || orari_scritti >= MAX_ORARI_SCRITTI) return;
    orari_scritti++;
    scrivi_nodi();
    fprintf(t, "{\"tipo\":\"orario\",\"n\":%lld,\"nodo\":%lld,", n_gen, traccia_nodi);
    orario_fino_a(orario_len);
    fprintf(t, "}\n");
    fflush(t);
}

void traccia_progresso(const int pos) {
    if (!con_progresso) return;
    const clock_t ora = clock();
    if ((double) (ora - ultimo_progresso) / CLOCKS_PER_SEC < 0.5) return;
    ultimo_progresso = ora;
    fprintf(stderr, "{\"tipo\":\"progresso\",\"nodi\":%lld,\"backtrack\":%lld,\"orari\":%lld,\"pos\":%d,"
            "\"secondi\":%.1f}\n", traccia_nodi, traccia_backtrack_n, n_gen, pos,
            (double) (ora - inizio) / CLOCKS_PER_SEC);
    fflush(stderr);
}

void traccia_fine(const double secondi) {
    if (!t) return;
    scrivi_nodi();
    fprintf(t, "{\"tipo\":\"fine\",\"esito\":\"%s\",\"orari\":%lld,\"orari_massimi\":%lld,\"nodi\":%lld,"
            "\"backtrack\":%lld,\"backtrack_registrati\":%lld,\"secondi\":%.3f}\n",
            n_gen > 0 ? "trovato" : "nessuna soluzione", n_gen, n_gen_max, traccia_nodi, traccia_backtrack_n,
            MIN(traccia_backtrack_n, max_registrati), secondi);
    fflush(t);
}

void traccia_errore(const char *messaggio) {
    if (t) {
        fprintf(t, "{\"tipo\":\"errore\",\"messaggio\":");
        stringa(messaggio);
        fprintf(t, "}\n");
        fflush(t);
    }
    fprintf(stderr, "Errore nei dati: %s\n", messaggio);
}
