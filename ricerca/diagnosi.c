#include "diagnosi.h"

#include <signal.h>
#include <stdio.h>
#include <time.h>

#include "../priority_queue/prio_queue.h"
#include "ricerca.h"
#include "vincoli.h"

long long int nodi = 0;
int pos_max = -1;
long long int nodi_a_pos_max = 0;
double limite_secondi = 0;
double inizio_ricerca;
int interrotto_per_tempo = 0;
int errore_di_scrittura = 0;
int interrotto_su_richiesta = 0;

static volatile sig_atomic_t arresto_richiesto = 0;
static int con_progresso = 0;
static double prossimo_progresso = 0;


#define N_SLOT (GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ) // slot possibili, al massimo

static int registra_motivi = 0;
static long long int marce_per_pos[N_SLOT];                // marce indietro per slot
long long int blocchi_per_pos[N_SLOT + 1];                 // vicoli ciechi per slot: nessun docente messo (vedi diagnosi.h)
long long int visite_per_pos[N_SLOT + 1];                  // nodi per slot (+1: la posizione dopo l'ultimo slot)
static stato_blocco_t ultima_marcia[N_SLOT];
static stato_blocco_t ultimo_scarto[N_SLOT][N_MOTIVI_SCARTO];
static dettaglio_scarto_t dettaglio_ultimo_scarto[N_SLOT][N_MOTIVI_SCARTO];
static long long int motivi_per_pos[N_SLOT][N_MOTIVI];     // motivi di scarto per slot
static long long int docenti_per_pos[N_SLOT];              // docenti con ore da fare, sommati
static long long int motivi_per_docente[INSEGNANTE_SZ][N_MOTIVI];
static long long int scartati_per_pos[N_SLOT][N_MOTIVI_SCARTO]; // rami scartati dopo lo slot, per motivo
static long long int scartati[N_MOTIVI_SCARTO];

static const char *nome_motivo_scarto[N_MOTIVI_SCARTO] = {
    "alla classe non bastano piu' le lezioni per gli slot da riempire",
    "le ore che mancano a un docente non stanno piu' nel tempo rimasto"
};

// Descrizione di ogni motivo, nello stesso ordine dei numeri restituiti da motivo_di_scarto.
static const char *nome_motivo[N_MOTIVI] = {
    "occupato in un'altra classe alla stessa ora",
    "non ancora entrato (ora di ingresso)",
    "gia' al massimo di ore in questa classe oggi",
    "ore non contigue",
    "gia' al massimo di classi diverse oggi",
    "solo ore in codocenza e nessun codocente utilizzabile",
    "bloccato dalla regola del sabato"
};

// Tempo reale (non tempo di CPU) in secondi, da un punto di partenza qualunque: serve solo per differenze.
double secondi_reali(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void diagnosi_imposta(const double secondi, const int con_motivi) {
    limite_secondi = secondi;
    registra_motivi = con_motivi;
}

void diagnosi_imposta_progresso(const int attivo) {
    con_progresso = attivo;
}

void diagnosi_richiesta_di_arresto(const int segnale) {
    (void) segnale;
    arresto_richiesto = 1;
}

// Una riga di avanzamento: PROGRESSO {"secondi", "nodi", "marce", "scartati", "pos", "pos_max", "trovati",
// "blocchi_slot"}. La tabella per slot (i blocchi, vedi diagnosi_blocchi_nello_slot) e' piatta, nell'ordine [giorno][ora][classe]
// con la classe come indice dei dati (come nel JSON finale): indice (giorno * ore + ora) * classi + classe.
static void stampa_progresso(const int pos, const double adesso) {
    long long int scartati_tot = 0;
    for (int m = 0; m < N_MOTIVI_SCARTO; m++) scartati_tot += scartati[m];
    printf("PROGRESSO {\"secondi\": %.1f, \"nodi\": %lld, \"marce\": %lld, \"scartati\": %lld, \"pos\": %d, "
           "\"pos_max\": %d, \"trovati\": %lld, \"blocchi_slot\": [", adesso - inizio_ricerca, nodi, n_backtrack,
           scartati_tot, pos, pos_max, n_gen);
    for (int g = 0; g < actual_giorni_settimana; g++)
        for (int o = 0; o < actual_ore_giornaliere; o++)
            for (int c = 0; c < actual_classi_sezione; c++)
                printf(g + o + c ? ",%lld" : "%lld", blocchi_per_pos[slot_di(g, o, c)]);
    printf("]}\n");
    fflush(stdout);
}

int diagnosi_controllo_periodico(const int pos) {
    if (arresto_richiesto) {
        interrotto_su_richiesta = 1;
        return 1;
    }
    if (con_progresso) {
        const double adesso = secondi_reali();
        if (adesso >= prossimo_progresso) {
            prossimo_progresso = adesso + 1;
            stampa_progresso(pos, adesso);
        }
    }
    if (limite_secondi > 0 && secondi_reali() - inizio_ricerca > limite_secondi) {
        interrotto_per_tempo = 1;
        return 1;
    }
    // una scrittura nel file degli orari non e' riuscita: inutile continuare, ma la diagnosi si scrive comunque
    // (il solver ignora SIGXFSZ e SIGPIPE apposta, vedi main.c, cosi' l'errore arriva qui invece di terminarlo)
    if (file_orario && ferror(file_orario)) {
        errore_di_scrittura = 1;
        return 1;
    }
    return 0;
}

void diagnosi_avvia(void) {
    inizio_ricerca = secondi_reali();
}

/* ---- registrazione ---- */

// I motivi dell'ultima marcia indietro: (docente, motivo) per ogni docente con ore da fare nella classe.
static int motivi_ultima_marcia[INSEGNANTE_SZ][2];
static int n_motivi_ultima_marcia = 0;

// Primo motivo per cui il docente p, che in questa classe ha ancora ore da fare, non e' stato messo nello slot.
// Ripete gli stessi controlli della ricerca, nello stesso ordine; e' chiamata solo a ogni marcia indietro e solo
// con --motivi, quindi il costo non pesa sulla ricerca normale.
static int motivo_di_scarto(const teaching_slot *slot, const int pos, const int p, const int base4d_queue,
                            const int queue_size) {
    if (!prof_è_disponibile(slot->base3d_goi, p)) return 0;
    if (!verifica_ora_ingresso(p, slot->ora)) return 1;
    if (!prof_fa_al_max_n_ore(slot, p)) return 2;
    if (!prof_fa_ore_contigue(slot, p)) return 3;
    if (!ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, slot->giorno, slot->classe, p)) return 4;
    // idoneo e con ore da solo: l'unico controllo rimasto che puo' averlo scartato e' la regola del sabato
    if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(slot->base2d_ci, p)) return 6;
    // idoneo e senza ore da solo: il motivo e' la mancanza di un codocente, a meno che esista un partner utilizzabile
    // che la regola del sabato scarta
    for (int jj = base4d_queue; jj < queue_size; jj++) {
        const int q = queue_mul[jj]->teacher;
        if (q != p && prof_è_disponibile(slot->base3d_goi, q)
            && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p, q)
            && docente_idoneo(slot, pos, q))
            return 6;
    }
    return 5;
}

// Classe in cui il docente insegna nella stessa ora dello slot (tra gli slot gia' assegnati), oppure -1.
static int classe_in_cui_insegna_adesso(const teaching_slot *slot, const int pos, const int p) {
    for (int i = slot->giorno * actual_ore_giornaliere * actual_classi_sezione; i < pos; i++)
        if (orario[i].ora == slot->ora && (orario[i].insegnante1 == p || orario[i].insegnante2 == p))
            return orario[i].classe;
    return -1;
}

// Perche' il docente non e' idoneo (numeri come in motivo_di_scarto: 1 ingresso, 2 massimo di ore, 3 ore non contigue,
// 4 massimo di classi) oppure 0 se lo e'.
static int ragione_non_idoneo(const teaching_slot *slot, const int pos, const int q) {
    if (!verifica_ora_ingresso(q, slot->ora)) return 1;
    if (!prof_fa_al_max_n_ore(slot, q)) return 2;
    if (!prof_fa_ore_contigue(slot, q)) return 3;
    if (!ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, slot->giorno, slot->classe, q)) return 4;
    return 0;
}

// Esito di ogni vincolo per il docente p nello slot, valutati uno per uno (non si ferma al primo che non va): 1 rispettato,
// 0 non rispettato, -1 non si applica. Ordine: 0 libero (non in un'altra classe a quest'ora), 1 ora di ingresso, 2 massimo
// di ore al giorno nella classe, 3 ore attaccate, 4 massimo di classi diverse al giorno, 5 da solo o con un codocente
// utilizzabile, 6 regola del sabato. Servono al sito per scrivere una frase per vincolo (vedi orario.js, vincoliDocente).
static void vincoli_docente(const teaching_slot *slot, const int pos, const int p, const int base4d_queue,
                            const int queue_size, FILE *f) {
    const int da_solo = deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(slot->base2d_ci, p);
    int v[7];
    v[0] = prof_è_disponibile(slot->base3d_goi, p);
    v[1] = verifica_ora_ingresso(p, slot->ora);
    v[2] = prof_fa_al_max_n_ore(slot, p);
    v[3] = prof_fa_ore_contigue(slot, p);
    v[4] = ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(pos, slot->giorno, slot->classe, p);
    // codocenza: puo' fare lezione da solo, oppure esiste un codocente libero e idoneo
    int partner_utilizzabile = -1;
    for (int jj = base4d_queue; jj < queue_size; jj++) {
        const int q = queue_mul[jj]->teacher;
        if (q != p && prof_è_disponibile(slot->base3d_goi, q)
            && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p, q) && docente_idoneo(slot, pos, q)) {
            partner_utilizzabile = q;
            break;
        }
    }
    v[5] = da_solo || partner_utilizzabile >= 0;
    // regola del sabato (solo l'ultimo giorno): l'ora si puo' riempire in modo da non lasciare a meta' le ore dell'ora prima
    if (slot->giorno != actual_giorni_settimana - 1) {
        v[6] = -1;
    } else {
        int ok = da_solo && sabato_esaurisci_priorita_subito(pos, slot->giorno, slot->ora, slot->base2d_ci, p, -1);
        for (int jj = base4d_queue; jj < queue_size && !ok; jj++) {
            const int q = queue_mul[jj]->teacher;
            ok = q != p && prof_è_disponibile(slot->base3d_goi, q)
                 && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p, q)
                 && sabato_esaurisci_priorita_subito(pos, slot->giorno, slot->ora, slot->base2d_ci, p, q);
        }
        v[6] = ok;
    }
    fprintf(f, "[");
    for (int k = 0; k < 7; k++) fprintf(f, "%s%d", k ? ", " : "", v[k]);
    fprintf(f, "], \"codocente_libero\": %d", partner_utilizzabile);
}

// I dettagli di un docente nello slot: dove ha insegnato oggi, dove e' ora, quando entra, con chi puo' fare coppia.
static void dettaglio_docente(const teaching_slot *slot, const int pos, const int p, const int base4d_queue,
                              const int queue_size, FILE *f) {
    fprintf(f, "{\"ingresso\": %d, \"occupato_in\": %d, \"da_solo\": %s, \"oggi\": [", docente_info[p].ora_entrata,
            classe_in_cui_insegna_adesso(slot, pos, p),
            deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(slot->base2d_ci, p) ? "true" : "false");
    int primo = 1;
    for (int i = slot->giorno * actual_ore_giornaliere * actual_classi_sezione; i < pos; i++)
        if (orario[i].insegnante1 == p || orario[i].insegnante2 == p) {
            fprintf(f, "%s[%d, %d]", primo ? "" : ", ", orario[i].ora, orario[i].classe);
            primo = 0;
        }
    fprintf(f, "], \"partner\": [");
    primo = 1;
    for (int jj = base4d_queue; jj < queue_size; jj++) {
        const int q = queue_mul[jj]->teacher;
        if (q == p || !docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p, q)) continue;
        // stato: -1 libero, 0 occupato in un'altra classe (classe), 1..4 non idoneo (come ragione_non_idoneo)
        int stato = -1, classe = -1;
        if (!prof_è_disponibile(slot->base3d_goi, q)) {
            stato = 0;
            classe = classe_in_cui_insegna_adesso(slot, pos, q);
        } else if (ragione_non_idoneo(slot, pos, q)) {
            stato = ragione_non_idoneo(slot, pos, q);
        }
        fprintf(f, "%s[%d, %d, %d]", primo ? "" : ", ", q, stato, classe);
        primo = 0;
    }
    fprintf(f, "]");
    // regola del sabato: i docenti dell'ora precedente che devono continuare
    fprintf(f, ", \"ora_precedente\": [");
    if (slot->giorno == actual_giorni_settimana - 1 && slot->ora > 0) {
        const teaching_slot *prev = slot_precedente(slot->giorno, slot->ora, slot->classe);
        int n = 0;
        const int doc[2] = {prev->insegnante1, prev->insegnante2};
        for (int k = 0; k < 2; k++)
            if (doc[k] >= 0 && classe_docente[slot->base2d_ci + doc[k]].ore - classe_docente[slot->base2d_ci + doc[k]].svolte > 0)
                fprintf(f, "%s%d", n++ ? ", " : "", doc[k]);
    }
    fprintf(f, "]}");
}

// Analisi di uno slot (per il sito): lo stato e' quello dell'orario parziale che la ricerca ha costruito fino a lui.
// Per ogni docente della coda, nell'ordine di priorita': la priorita' (ore ancora da sistemare nella settimana), le ore
// che gli restano in questa classe e se era utilizzabile nello slot o quale vincolo lo escludeva.
void diagnosi_analizza_slot(const teaching_slot *slot, const int pos, const int base4d_queue, const int queue_size,
                            FILE *f, const char *scarto) {
    fprintf(f, "{\"slot\": %d, \"lezione_ammessa\": %s, \"coda\": [", pos,
            activities_for_this_class_and_hour(slot) ? "true" : "false");
    for (int ii = base4d_queue; ii < queue_size; ii++) {
        const int p = queue_mul[ii]->teacher;
        const int ore_rimaste = classe_docente[slot->base2d_ci + p].ore - classe_docente[slot->base2d_ci + p].svolte;
        int motivo; // -1 = utilizzabile, 7 = nessuna ora da fare in questa classe
        int potatura = -2; // solo per gli utilizzabili: -1 il nodo successivo passerebbe le verifiche, 0/1 il motivo dello scarto
        if (!deve_ancora_svolgere_ore_in_questa_classe(slot->base2d_ci, p)) {
            motivo = 7;
        } else {
            motivo = motivo_di_scarto(slot, pos, p, base4d_queue, queue_size);
            // utilizzabile: gli stessi controlli di prova_tutti_i_docenti (prima in coppia, poi da solo). Per ogni
            // modo in cui si puo' mettere (da solo, con ciascun codocente) si calcola anche se il nodo successivo
            // passerebbe le verifiche di potatura (potatura_se_messo): basta un modo che le passa. Solo qui
            // nell'analisi, mai nella ricerca: sono una ventina di simulazioni per finestra.
            if (prof_è_disponibile(slot->base3d_goi, p) && docente_idoneo(slot, pos, p)) {
                if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(slot->base2d_ci, p)
                    && sabato_esaurisci_priorita_subito(pos, slot->giorno, slot->ora, slot->base2d_ci, p, -1))
                    potatura = potatura_se_messo(pos, ii, -1);
                for (int jj = base4d_queue; jj < queue_size && potatura != -1; jj++) {
                    const int q = queue_mul[jj]->teacher;
                    if (q != p && prof_è_disponibile(slot->base3d_goi, q)
                        && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p, q)
                        && sabato_esaurisci_priorita_subito(pos, slot->giorno, slot->ora, slot->base2d_ci, p, q)
                        && docente_idoneo(slot, pos, q)) {
                        potatura = potatura_se_messo(pos, ii, jj);  // l'ultimo fallito: il suo dettaglio e' quello fresco
                    }
                }
                if (potatura != -2) motivo = -1;  // utilizzabile (anche se poi il ramo verrebbe scartato)
            }
            // docente qui non utilizzabile: l'esito della potatura si calcola lo stesso, come ipotesi (le lezioni che
            // avrebbe da fare, da solo o con ciascun codocente con ore insieme, senza i vincoli di contorno): la
            // finestra del sito lo mostra per tutti i docenti con ore da fare
            if (potatura == -2) {
                if (deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(slot->base2d_ci, p))
                    potatura = potatura_se_messo(pos, ii, -1);
                for (int jj = base4d_queue; jj < queue_size && potatura != -1; jj++) {
                    const int q = queue_mul[jj]->teacher;
                    if (q != p && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(slot->classe, p, q))
                        potatura = potatura_se_messo(pos, ii, jj);
                }
            }
        }
        fprintf(f, "%s{\"docente\": %d, \"priorita\": %g, \"ore_classe\": %d, \"ore_totali_classe\": %d, \"motivo\": %d",
                ii > base4d_queue ? ", " : "", p, queue_mul[ii]->priority, ore_rimaste,
                classe_docente[slot->base2d_ci + p].ore, motivo);
        if (potatura != -2) fprintf(f, ", \"potatura_dopo\": %d", potatura);
        if (potatura >= 0) {
            // il conto fallito dell'ultima simulazione, per la finestra: come il blocco "potatura" del nodo scartato
            fprintf(f, ", \"potatura_dettaglio\": {\"classe\": %d, \"ore_classe\": %d, \"ore_classe_fatte\": %d",
                    slot->classe, classe_info[slot->classe].ore, classe_info[slot->classe].svolte + 1);
            diagnosi_scrivi_dettaglio_scarto(f, potatura, &dettaglio_scarto);
            fprintf(f, "}");
        }
        if (ore_rimaste > 0) {
            fprintf(f, ", \"vincoli\": ");
            vincoli_docente(slot, pos, p, base4d_queue, queue_size, f);
            fprintf(f, ", \"dettaglio\": ");
            dettaglio_docente(slot, pos, p, base4d_queue, queue_size, f);
        }
        fprintf(f, "}");
    }
    fprintf(f, "], \"limiti\": {\"ore_in_classe\": %d, \"classi_al_giorno\": %d, \"ora\": %d, \"giorno\": %d}, \"motivi\": [",
            MAX_ORE_PROF_IN_UNA_CLASSE, MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO, slot->ora, slot->giorno);
    for (int m = 0; m < N_MOTIVI; m++) fprintf(f, "%s\"%s\"", m ? ", " : "", nome_motivo[m]);
    fprintf(f, ", \"nessuna ora da fare in questa classe\"]");
    if (scarto) fprintf(f, ", \"potatura\": %s", scarto);
    fputs("}\n", f);
}

void diagnosi_scrivi_dettaglio_scarto(FILE *f, const int motivo, const dettaglio_scarto_t *d) {
    if (motivo == SCARTATO_LEZIONI_DELLA_CLASSE) {
        fprintf(f, ", \"fino_ora\": %d, \"da_riempire\": %d, \"possibili\": %d, \"oggi\": %d, "
                   "\"slot_oggi\": %d, \"in_un_giorno\": %d, \"slot_al_giorno\": %d, \"giorni_dopo\": %d, "
                   "\"lezioni\": [", d->fino_ora, d->da_riempire, d->possibili, d->oggi, d->slot_oggi,
                d->in_un_giorno, d->slot_al_giorno, d->giorni_dopo);
        for (int k = 0; k < d->n_lezioni; k++)
            fprintf(f, k ? ", [%d, %d, %d, %d]" : "[%d, %d, %d, %d]", d->lezioni[k][0], d->lezioni[k][1],
                    d->lezioni[k][2], d->lezioni[k][3]);
        fputs("]", f);
    } else
        fprintf(f, ", \"docente\": %d, \"classe_docente\": %d, \"ore_rimaste\": %d, \"ore_possibili\": %d",
                d->docente, d->classe_docente, d->ore_rimaste, d->ore_possibili);
}

void diagnosi_ramo_scartato(const int pos, const int motivo) {
    scartati_per_pos[pos][motivo]++;
    scartati[motivo]++;
    const int cla = orario[pos].classe;
    stato_blocco_t *u = &ultimo_scarto[pos][motivo];
    u->ore_classe_fatte = classe_info[cla].svolte;
    u->scelta[0] = orario[pos].insegnante1;
    u->scelta[1] = orario[pos].insegnante2;
    dettaglio_ultimo_scarto[pos][motivo] = dettaglio_scarto;
}

long long int diagnosi_visite_nello_slot(const int pos) { return visite_per_pos[pos]; }

stato_blocco_t diagnosi_ultima_marcia(const int pos) {
    if (!marce_per_pos[pos]) return (stato_blocco_t){.ore_classe_fatte = -1};
    return ultima_marcia[pos];
}

stato_blocco_t diagnosi_ultimo_scarto(const int pos, const int motivo, dettaglio_scarto_t *dettaglio) {
    *dettaglio = dettaglio_ultimo_scarto[pos][motivo];
    if (!scartati_per_pos[pos][motivo]) return (stato_blocco_t){.ore_classe_fatte = -1};
    return ultimo_scarto[pos][motivo];
}

// Conta la marcia indietro nello slot e, con --motivi, perche' ogni docente con ore da fare nella classe e' stato
// scartato (per lo slot e per il docente).
void diagnosi_marcia_indietro(const teaching_slot *slot, const int pos, const int base4d_queue, const int queue_size) {
    marce_per_pos[pos]++;
    blocchi_per_pos[pos]++;
    // lo stato in questo momento, per la diagnosi: ore della classe gia' in orario, docenti con ore ancora da fare
    int con_ore = 0;
    for (int ii = base4d_queue; ii < queue_size; ii++)
        if (deve_ancora_svolgere_ore_in_questa_classe(slot->base2d_ci, queue_mul[ii]->teacher)) con_ore++;
    stato_blocco_t *u = &ultima_marcia[pos];
    u->ore_classe_fatte = classe_info[slot->classe].svolte;
    u->docenti_con_ore = con_ore;
    u->n_docenti = 0;
    n_motivi_ultima_marcia = 0;
    if (!registra_motivi) return;
    for (int ii = base4d_queue; ii < queue_size; ii++) {
        const int p = queue_mul[ii]->teacher;
        if (!deve_ancora_svolgere_ore_in_questa_classe(slot->base2d_ci, p)) continue;
        const int m = motivo_di_scarto(slot, pos, p, base4d_queue, queue_size);
        motivi_per_pos[pos][m]++;
        motivi_per_docente[p][m]++;
        if (u->n_docenti < MAX_DOCENTI_BLOCCO) {
            int *d = u->docenti[u->n_docenti++];
            d[0] = p;
            d[1] = m;
            d[2] = m == 0 ? classe_in_cui_insegna_adesso(slot, pos, p) : -1;
        }
        docenti_per_pos[pos]++;
        if (n_motivi_ultima_marcia < INSEGNANTE_SZ) {
            motivi_ultima_marcia[n_motivi_ultima_marcia][0] = p;
            motivi_ultima_marcia[n_motivi_ultima_marcia][1] = m;
            n_motivi_ultima_marcia++;
        }
    }
}

void diagnosi_scrivi_motivi_ultima_marcia(FILE *f) {
    fputc('M', f);
    for (int i = 0; i < n_motivi_ultima_marcia; i++)
        fprintf(f, " %d:%d", motivi_ultima_marcia[i][0], motivi_ultima_marcia[i][1]);
    fputc('\n', f);
}

/* ---- lettura dei contatori ---- */

long long int diagnosi_marce_nello_slot(const int pos) { return marce_per_pos[pos]; }
long long int diagnosi_blocchi_nello_slot(const int pos) { return blocchi_per_pos[pos]; }

long long int diagnosi_scartati_nello_slot(const int pos, const int motivo_scarto) {
    return scartati_per_pos[pos][motivo_scarto];
}

long long int diagnosi_motivo_nello_slot(const int pos, const int motivo) { return motivi_per_pos[pos][motivo]; }

long long int diagnosi_motivo_del_docente(const int prof, const int motivo) { return motivi_per_docente[prof][motivo]; }

int diagnosi_con_motivi(void) { return registra_motivi; }

const char *diagnosi_nome_motivo(const int motivo) { return nome_motivo[motivo]; }

const char *diagnosi_nome_motivo_scarto(const int motivo_scarto) { return nome_motivo_scarto[motivo_scarto]; }

/* ---- stampa ---- */

static void stampa_nome_slot(const int pos) {
    const teaching_slot *s = &orario[pos];
    printf("giorno %s, ora %s, classe %s", giorni_settimana[s->giorno], ore_giornaliere[s->ora],
           classe_sezione[s->classe]);
}

// Quanto si e' esplorato e fin dove si e' arrivati. Se la posizione piu' avanzata e' stata raggiunta presto (piccola
// percentuale dei nodi) la ricerca da li' in poi non e' piu' andata avanti: e' bloccata.
static void stampa_riepilogo(const long long int n_backtrack) {
    printf("Nodi esplorati: %lld\n", nodi);
    if (pos_max >= 0) {
        printf("Posizione piu' avanzata: %d su %d (", pos_max, orario_len);
        stampa_nome_slot(pos_max < orario_len ? pos_max : orario_len - 1);
        printf("), raggiunta al nodo %lld (%.1f%% dei nodi)\n", nodi_a_pos_max,
               nodi ? 100.0 * nodi_a_pos_max / nodi : 0.0);
    }
    int distinti = 0;
    for (int i = 0; i < orario_len; i++) if (marce_per_pos[i]) distinti++;
    printf("Marce indietro: %lld in %d slot diversi\n", n_backtrack, distinti);
}

// Rami scartati dopo lo slot pos, per qualunque motivo.
static long long int scartati_nello_slot(const int pos) {
    long long int n = 0;
    for (int m = 0; m < N_MOTIVI_SCARTO; m++) n += scartati_per_pos[pos][m];
    return n;
}

// Come per le marce indietro: se i rami scartati sono quasi tutti dopo pochi slot, la ricerca e' bloccata li'.
static void stampa_rami_scartati(void) {
    long long int totale = 0;
    for (int m = 0; m < N_MOTIVI_SCARTO; m++) totale += scartati[m];
    int distinti = 0;
    for (int i = 0; i < orario_len; i++) if (scartati_nello_slot(i)) distinti++;
    printf("Rami scartati: %lld dopo %d slot diversi\n", totale, distinti);
    for (int m = 0; m < N_MOTIVI_SCARTO; m++)
        if (scartati[m]) printf("  %lld: %s\n", scartati[m], nome_motivo_scarto[m]);
    char usato[N_SLOT] = {0};
    for (int k = 0; k < 5; k++) {
        int best = -1;
        for (int i = 0; i < orario_len; i++)
            if (!usato[i] && scartati_nello_slot(i) && (best < 0 || scartati_nello_slot(i) > scartati_nello_slot(best)))
                best = i;
        if (best < 0) break;
        usato[best] = 1;
        printf("  Rami scartati dopo lo slot %d: posizione %d (", k + 1, best);
        stampa_nome_slot(best);
        printf("): %lld (%.1f%%)\n", scartati_nello_slot(best), 100.0 * scartati_nello_slot(best) / totale);
    }
}

// Con --motivi: in percentuale, perche' i docenti sono stati scartati nello slot pos.
static void stampa_motivi_dello_slot(const int pos) {
    if (!registra_motivi || !docenti_per_pos[pos]) return;
    printf("      motivi per cui i docenti con ore da fare in quella classe sono stati scartati (%.1f per marcia):\n",
           (double) docenti_per_pos[pos] / marce_per_pos[pos]);
    for (int m = 0; m < N_MOTIVI; m++)
        if (motivi_per_pos[pos][m])
            printf("        %5.1f%%  %s\n", 100.0 * motivi_per_pos[pos][m] / docenti_per_pos[pos], nome_motivo[m]);
}

// Lo slot con piu' marce indietro tra quelli non ancora stampati (-1 se non ce ne sono).
static int slot_con_piu_marce(const char usato[]) {
    int best = -1;
    for (int i = 0; i < orario_len; i++)
        if (!usato[i] && marce_per_pos[i] && (best < 0 || marce_per_pos[i] > marce_per_pos[best])) best = i;
    return best;
}

// I 5 slot con piu' marce indietro. Se pochi slot hanno quasi tutte le marce, il blocco e' li' (o poco prima).
static void stampa_slot_piu_frequenti(const long long int n_backtrack) {
    char usato[N_SLOT] = {0};
    for (int k = 0; k < 5; k++) {
        const int best = slot_con_piu_marce(usato);
        if (best < 0) break;
        usato[best] = 1;
        printf("  Slot piu' frequente %d: posizione %d (", k + 1, best);
        stampa_nome_slot(best);
        printf("): %lld marce (%.1f%%)\n", marce_per_pos[best],
               n_backtrack ? 100.0 * marce_per_pos[best] / n_backtrack : 0.0);
        stampa_motivi_dello_slot(best);
    }
}

// I docenti scartati piu' spesso, in tutta la ricerca.
static void stampa_docenti_scartati_piu_spesso(void) {
    if (!registra_motivi) return;
    printf("  Docenti scartati piu' spesso (docente: motivo, volte):\n");
    char usato[INSEGNANTE_SZ * N_MOTIVI] = {0};
    for (int k = 0; k < 6; k++) {
        int bp = -1, bm = -1;
        for (int p = 0; p < actual_insegnanti; p++)
            for (int m = 0; m < N_MOTIVI; m++)
                if (!usato[p * N_MOTIVI + m] && motivi_per_docente[p][m]
                    && (bp < 0 || motivi_per_docente[p][m] > motivi_per_docente[bp][bm])) {
                    bp = p;
                    bm = m;
                }
        if (bp < 0) break;
        usato[bp * N_MOTIVI + bm] = 1;
        printf("    %s: %s, %lld\n", insegnanti[bp], nome_motivo[bm], motivi_per_docente[bp][bm]);
    }
}

// Riepilogo, marce indietro per slot, docenti scartati (con --motivi) e rami scartati dalla potatura.
// tests/casuali.py legge questo testo: se si cambiano le frasi, va aggiornato anche lui.
void stampa_diagnosi(const long long int n_backtrack) {
    printf("\nDiagnosi della ricerca\n");
    if (interrotto_per_tempo) printf("Interrotto per tempo\n");
    if (interrotto_su_richiesta) printf("Interrotto su richiesta\n");
    if (errore_di_scrittura)
        printf("Errore di scrittura del file degli orari (disco pieno o file troppo grande): il file e' incompleto\n");
    stampa_riepilogo(n_backtrack);
    stampa_slot_piu_frequenti(n_backtrack);
    stampa_docenti_scartati_piu_spesso();
    stampa_rami_scartati();
}
