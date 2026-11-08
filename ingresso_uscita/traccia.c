#include "traccia.h"

#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"
#include "../controllo_in_avanti/controllo_in_avanti.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// la ricerca e i suoi controlli, del solver con cui si compila: si richiamano, non si copiano
#include "../controlli/controlli.h"

// ---- l'ordine delle celle (solver che non decidono le celle in ordine di indice, vedi traccia_imposta_ordine) ----
// Nella traccia "pos" e' il passo s della ricerca; la cella decisa al passo s e' cella(s). Senza ordine, cella(s) = s.
static const int *ordine_cella = NULL, *ordine_indice = NULL;
static int cella(const int s) { return ordine_cella ? ordine_cella[s] : s; }
static int indice(const int c) { return ordine_indice ? ordine_indice[c] : c; }
static int s_traccia;  // il passo del backtrack che si sta scrivendo

void traccia_imposta_ordine(const int *cella_di, const int *indice_di) {
    ordine_cella = cella_di;
    ordine_indice = indice_di;
}

traccia_controllo_fn traccia_controllo_extra = NULL;

const int *traccia_giorni = NULL, *traccia_posti = NULL, *traccia_classi = NULL;
int traccia_giro = 0, traccia_fissati = 0;

// il posto del giorno g nell'ordine in cui si riempiono i giorni (senza traccia_posti: il calendario)
static int posto(const int g) { return traccia_posti ? traccia_posti[g] : g; }

long long int traccia_nodi = 0;
long long int traccia_ferma_nodi = 0;
long long int traccia_prossimo = 0x10000;
int traccia_pos_max = 0;
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
    nodi_pezzo[nodi_n][1] = pos > 0 ? orario[cella(pos - 1)].insegnante1 : -1;
    nodi_pezzo[nodi_n][2] = pos > 0 ? orario[cella(pos - 1)].insegnante2 : -1;
    nodi_registrati++;
    if (++nodi_n == NODI_PER_RIGA) scrivi_nodi();
}

int traccia_apri(const char *percorso, const long long int max_backtrack, const long long int max_nodi,
                 const int progresso) {
    if (traccia_ferma_nodi > 0 && traccia_ferma_nodi < traccia_prossimo) traccia_prossimo = traccia_ferma_nodi;
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
    if (ordine_cella) {  // la cella decisa a ogni passo: nella traccia "pos" e gli orari sono in quest'ordine
        fprintf(t, "],\"ordine\":[");
        for (int i = 0; i < orario_len; i++) fprintf(t, "%s%d", i ? "," : "", ordine_cella[i]);
    }
    fprintf(t, "],\"parametri\":{\"ore_obbligatorie\":%d,\"max_ore_prof_in_una_classe\":%d,"
            "\"max_classi_diverse_al_giorno\":%d}}\n", PRIME_N_ORE_OBBLIGATORIE, MAX_ORE_PROF_IN_UNA_CLASSE,
            MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
    fflush(t);
}

// ---- pezzi di un record ----

static void orario_fino_a(const int pos) {
    fprintf(t, "\"orario\":[");
    for (int i = 0; i < pos; i++)
        fprintf(t, "%s[%d,%d]", i ? "," : "", orario[cella(i)].insegnante1, orario[cella(i)].insegnante2);
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

static void fine_controllo(const int uscita, int *fermato_a, const int k) {
    fprintf(t, "},\"uscita\":%d}", uscita ? 1 : 0);
    if (!uscita && *fermato_a < 0) *fermato_a = k;
}

// per i controlli in piu' dei solver (traccia_controllo_extra)
void traccia_inizia_controllo(const char *nome) { inizia_controllo(nome); }
void traccia_fine_controllo(const int uscita, int *fermato_a, const int k) { fine_controllo(uscita, fermato_a, k); }
FILE *traccia_file(void) { return t; }

static int idx_giorno_classe(const int giorno, const int cla, const int prof) {
    return giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + cla * INSEGNANTE_SZ + prof;
}

// I controlli, ognuno con quello che legge: uscita = il valore della funzione di main.

static int c_disponibile(const int pos, const int giorno, const int ora, const int base3d_goi, const int p,
                         int *fermato_a, const int k) {
    inizia_controllo("prof_è_disponibile");
    const int inizio_ora = giorno * actual_ore_giornaliere * actual_classi_sezione + ora * actual_classi_sezione;
    int in_classe = -1;  // in quale classe e' gia', tra le celle di quest'ora gia' decise
    for (int c = 0; c < actual_classi_sezione; c++) {
        const int i = inizio_ora + c;
        if (indice(i) < s_traccia && (orario[i].insegnante1 == p || orario[i].insegnante2 == p)) in_classe = c;
    }
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
    for (int i = giorno * actual_ore_giornaliere * actual_classi_sezione;
         i < (giorno + 1) * actual_ore_giornaliere * actual_classi_sezione; i++) {
        if (indice(i) >= s_traccia || (orario[i].insegnante1 != p && orario[i].insegnante2 != p)) continue;
        const int c = g_classe(i);
        int gia = 0;
        for (int j = 0; j < n; j++) gia |= classi[j] == c;
        if (!gia) classi[n++] = c;
    }
    fprintf(t, "\"classi_oggi_con_questa\":[");
    for (int j = 0; j < n; j++) fprintf(t, "%s%d", j ? "," : "", classi[j]);
    fprintf(t, "],\"massimo\":%d", MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
    const int u = ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(s_traccia, giorno, cla, p);
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
    const int ultimo = posto(giorno) == actual_giorni_settimana - 1;
    fprintf(t, "\"ultimo_giorno\":%d,\"ora\":%d", ultimo, ora);
    if (ultimo && ora > 0) {
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
    const int occupati = posto(giorno) * PRIME_N_ORE_OBBLIGATORIE + MIN(ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    const int mancanti = PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana - occupati;
    fprintf(t, "\"slot\":{\"nome\":\"activities_for_this_class_and_hour\",\"ingressi\":{\"ore_rimanenti\":%d,"
            "\"slot_obbligatori_mancanti\":%d,\"ore_buche_oggi\":%d},\"uscita\":%d}", ore_rimanenti, mancanti,
            n_ore_buche[giorno * CLASSE_SEZIONE_SZ + cla], activities_for_this_class_and_hour(pos, giorno, ora, cla));
}

static long long int registrati = 0, primo_del_giro = 0;  // backtrack registrati; il primo del giro (giorni-liberi)

static int registra(void) {
    traccia_backtrack_n++;
    if (!t || traccia_backtrack_n - primo_del_giro > max_registrati) return 0;
    registrati++;
    return 1;
}

void traccia_nuovo_giro(void) { primo_del_giro = traccia_backtrack_n; }

static void testa_backtrack(const int pos, const int giorno, const int ora, const int cla, const int buca_dopo) {
    scrivi_nodi();
    fprintf(t, "{\"tipo\":\"backtrack\",\"n\":%lld,\"nodo\":%lld,\"pos\":%d,\"giorno\":%d,\"ora\":%d,\"classe\":%d,"
            "\"buca_dopo\":%d,", traccia_backtrack_n, traccia_nodi, pos, giorno, ora, cla, buca_dopo);
    if (traccia_giorni) {
        fprintf(t, "\"giro\":%d,\"fissati\":%d,\"giorni\":[", traccia_giro, traccia_fissati);
        for (int r = 0; r < actual_giorni_settimana; r++) fprintf(t, "%s%d", r ? "," : "", traccia_giorni[r]);
        fprintf(t, "],");
        if (traccia_classi) {
            fprintf(t, "\"classi\":[");
            for (int k = 0; k < actual_classi_sezione; k++) fprintf(t, "%s%d", k ? "," : "", traccia_classi[k]);
            fprintf(t, "],");
        }
    }
}

// ---- i record ----

static void scrivi_cella(int s, int giorno, int ora, int cla, int base2d_ci, int base3d_goi, int base4d_queue,
                         int buca_dopo);

void traccia_backtrack(const int s, const int giorno, const int ora, const int cla, const int base2d_ci,
                       const int base3d_goi, const int base4d_queue, const int buca_dopo) {
    if (!registra()) return;
    scrivi_cella(s, giorno, ora, cla, base2d_ci, base3d_goi, base4d_queue, buca_dopo);
}

// i docenti della cella con i loro controlli (un backtrack)
static void scrivi_cella(const int s, const int giorno, const int ora, const int cla, const int base2d_ci,
                         const int base3d_goi, const int base4d_queue, const int buca_dopo) {
    const int pos = cella(s);  // la cella; nella traccia "pos" e' il passo s
    s_traccia = s;
    testa_backtrack(s, giorno, ora, cla, buca_dopo);
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
        fprintf(t, "%s{\"docente\":%d,\"priorita\":%.0f,\"margine\":%.0f,\"controlli\":[", primo ? "" : ",", p1,
                it->priority, margine_docente(it, ora, actual_giorni_settimana - 1 - posto(giorno)));
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
            // per gli altri (anche il controllo in piu' del solver, che mette la coppia nella cella per prova)
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
                if (traccia_controllo_extra)
                    traccia_controllo_extra(pos, giorno, ora, cla, base2d_ci, base3d_goi, queue_mul[ii], queue_mul[jj], &f2, 7);
                fprintf(t, "],\"fermato_a\":%d}", f2);
                incoerente |= fermato < 0 && f2 < 0;
            }
            int f3 = -1;
            n_controllo = 0;
            fprintf(t, "],\"da_solo\":{\"controlli\":[");
            c_senza_cod(base2d_ci, p1, &f3, 0);
            c_sabato(pos, giorno, ora, base2d_ci, p1, &f3, 1);
            if (traccia_controllo_extra)
                traccia_controllo_extra(pos, giorno, ora, cla, base2d_ci, base3d_goi, queue_mul[ii], NULL, &f3, 2);
            fprintf(t, "],\"fermato_a\":%d}", f3);
            incoerente |= fermato < 0 && f3 < 0;
        }
        fputc('}', t);
    }
    // incoerente: una combinazione risulterebbe valida, ma la ricerca non ne ha trovata nessuna. Non deve succedere: se
    // succede, la traccia non rispecchia la ricerca.
    fprintf(t, "],\"altri_docenti\":%d,\"incoerente\":%d,", altri, incoerente);
    orario_fino_a(s);
    fprintf(t, "}\n");
    fflush(t);
}

void traccia_slot_chiuso(const int s, const int giorno, const int ora, const int cla) {
    if (!registra()) return;
    testa_backtrack(s, giorno, ora, cla, 0);
    controllo_slot(cella(s), giorno, ora, cla);
    fprintf(t, ",\"docenti\":[],\"altri_docenti\":0,\"incoerente\":0,");
    orario_fino_a(s);
    fprintf(t, "}\n");
    fflush(t);
}

// Una potatura (vedi potature/potature.h) all'ingresso della cella pos: la scelta nella cella prima lascia un resto
// impossibile. Nella traccia e' un backtrack in questa cella, con il controllo e i suoi numeri.
void traccia_potatura(const int s, const Potatura *p) {
    if (!registra()) return;
    const int c = cella(s);
    testa_backtrack(s, g_giorno(c), g_ora(c), g_classe(c), 0);
    fprintf(t, "\"nella_cella\":1,");
    if (p->controllo == POTATURA_IN_AVANTI) {  // il controllo in avanti, entrando nella cella dopo la scelta
        fprintf(t, "\"slot\":{\"nome\":\"situazione_risolvibile\",\"ingressi\":{\"cella_bloccata\":%d,"
                "\"motivo\":\"%s\",\"giorno\":%d,\"ora\":%d,\"classe\":%d", p->cella, nome_motivo_in_avanti(p->motivo),
                p->giorno, p->ora, g_classe(p->cella));
        if (p->motivo >= 2)
            fprintf(t, ",\"docente\":%d,\"classe_conto\":%d,\"restano\":%d,\"possibili\":%d", p->docente, p->classe,
                    p->restano, p->possibili);
        if (p->motivo == 0) {  // la cella senza docenti: i docenti della sua classe, con perche' non possono starci
            static DocenteCella dc[INSEGNANTE_SZ];
            const int n = docenti_della_cella(s + 1, p->cella, dc, INSEGNANTE_SZ);
            fprintf(t, ",\"docenti_cella\":[");
            for (int k = 0; k < n; k++)
                fprintf(t, "%s{\"docente\":%d,\"restano\":%d,\"ingresso\":%d,\"occupato\":%d,\"fatte_oggi\":%d,"
                        "\"blocco_chiuso\":%d,\"classi_oggi\":%d,\"massimo_oggi\":%d}", k ? "," : "", dc[k].docente,
                        dc[k].restano, dc[k].ingresso, dc[k].occupato, dc[k].fatte_oggi, dc[k].blocco_chiuso,
                        dc[k].classi_oggi, dc[k].massimo_oggi);
            fputc(']', t);
        }
        fprintf(t, "},\"uscita\":0},\"docenti\":[],\"altri_docenti\":0,\"incoerente\":0,");
        orario_fino_a(s + 1);
        fprintf(t, "}\n");
        fflush(t);
        return;
    }
    fprintf(t, "\"slot\":{\"nome\":\"%s\",\"ingressi\":{\"docente\":%d,\"classe\":%d,\"restano\":%d,"
            "\"possibili\":%d,\"fino_ora\":%d,\"giorno\":%d,\"ora\":%d},\"uscita\":0}",
            p->controllo == POTATURA_ORE_DOCENTI ? "ore_dei_docenti_stanno_nel_tempo" : "lezioni_della_classe_bastano",
            p->docente, p->classe, p->restano, p->possibili, p->fino_ora, p->giorno, p->ora);
    fprintf(t, ",\"docenti\":[],\"altri_docenti\":0,\"incoerente\":0,");
    orario_fino_a(s + 1);
    fprintf(t, "}\n");
    fflush(t);
}

void traccia_orario_trovato(void) {
    if (!t || orari_scritti >= MAX_ORARI_SCRITTI) return;
    orari_scritti++;
    scrivi_nodi();
    fprintf(t, "{\"tipo\":\"orario\",\"n\":%lld,\"nodo\":%lld,", n_gen, traccia_nodi);
    if (traccia_giorni) {  // in ordine di calendario
        fprintf(t, "\"giro\":%d,\"orario\":[", traccia_giro);
        for (int i = 0; i < orario_len; i++)
            fprintf(t, "%s[%d,%d]", i ? "," : "", orario[i].insegnante1, orario[i].insegnante2);
        fputc(']', t);
    } else orario_fino_a(orario_len);
    fprintf(t, "}\n");
    fflush(t);
}

// ",\"giri\":N" nella fine, per giorni-liberi (vuoto per gli altri solver)
static const char *giri(void) {
    static char testo[32];
    testo[0] = 0;
    if (traccia_giorni) snprintf(testo, sizeof testo, ",\"giri\":%d", traccia_giro);
    return testo;
}

// Con --ferma-nodi N la ricerca si ferma qui, esattamente al nodo N (o prima, se finisce prima): la ricerca chiama
// traccia_progresso quando il contatore dei nodi arriva a traccia_prossimo, che e' il prossimo multiplo di 65.536 (il
// progresso) oppure N, se viene prima. Sempre allo stesso nodo (non dipende dal tempo): per i test notturni, che
// devono dare ogni volta gli stessi numeri. Fino a quel nodo la ricerca e' la stessa.
static void ferma_per_limite(void) {
    if (t) {
        scrivi_nodi();
        fprintf(t, "{\"tipo\":\"fine\",\"esito\":\"limite di nodi\",\"orari\":%lld,\"orari_massimi\":%lld,"
                "\"nodi\":%lld,\"backtrack\":%lld,\"backtrack_registrati\":%lld,\"pos_max\":%d,\"secondi\":%.3f%s}\n",
                n_gen, n_gen_max, traccia_nodi, traccia_backtrack_n, registrati,
                traccia_pos_max, (double) (clock() - inizio) / CLOCKS_PER_SEC, giri());
        fclose(t);
    }
    printf("Fermato dopo %lld nodi (--ferma-nodi)\n", traccia_nodi);
    exit(0);
}

void traccia_progresso(const int pos) {
    if (traccia_ferma_nodi > 0 && traccia_nodi >= traccia_ferma_nodi) ferma_per_limite();
    traccia_prossimo = (traccia_nodi | 0xFFFF) + 1;  // il prossimo multiplo di 65.536...
    if (traccia_ferma_nodi > traccia_nodi && traccia_ferma_nodi < traccia_prossimo)
        traccia_prossimo = traccia_ferma_nodi;  // ...o il limite, se viene prima
    if (!con_progresso) return;
    const clock_t ora = clock();
    if ((double) (ora - ultimo_progresso) / CLOCKS_PER_SEC < 0.5) return;
    ultimo_progresso = ora;
    fprintf(stderr, "{\"tipo\":\"progresso\",\"nodi\":%lld,\"backtrack\":%lld,\"orari\":%lld,\"pos\":%d,"
            "\"pos_max\":%d,\"secondi\":%.1f}\n", traccia_nodi, traccia_backtrack_n, n_gen, pos, traccia_pos_max,
            (double) (ora - inizio) / CLOCKS_PER_SEC);
    fflush(stderr);
}

void traccia_fine(const double secondi) {
    if (!t) return;
    scrivi_nodi();
    fprintf(t, "{\"tipo\":\"fine\",\"esito\":\"%s\",\"orari\":%lld,\"orari_massimi\":%lld,\"nodi\":%lld,"
            "\"backtrack\":%lld,\"backtrack_registrati\":%lld,\"pos_max\":%d,\"secondi\":%.3f%s}\n",
            n_gen > 0 ? "trovato" : "nessuna soluzione", n_gen, n_gen_max, traccia_nodi, traccia_backtrack_n,
            registrati, traccia_pos_max, secondi, giri());
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
