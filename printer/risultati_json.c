#include "risultati_json.h"

#include <stdio.h>

#include "../data_initialization/database_initializers.h"
#include "../orario/orario.h"
#include "../priority_queue/prio_queue.h"
#include "../ricerca/diagnosi.h"
#include "../ricerca/ricerca.h"
#include "../ricerca/vincoli.h"

/* ---- pezzi di JSON ---- */

// Una stringa tra virgolette, con i caratteri speciali di JSON preceduti da \ (o scritti come \u00XX).
static void stringa(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; s++) {
        const unsigned char ch = (unsigned char) *s;
        if (ch == '"' || ch == '\\') fprintf(f, "\\%c", ch);
        else if (ch < 0x20) fprintf(f, "\\u%04x", ch);
        else fputc(ch, f);
    }
    fputc('"', f);
}

// Un elenco di stringhe: ["a", "b", ...].
static void elenco_di_stringhe(FILE *f, const char nomi[][MAX_LEN_WORDS], const int n) {
    fputc('[', f);
    for (int i = 0; i < n; i++) {
        if (i) fputs(", ", f);
        stringa(f, nomi[i]);
    }
    fputc(']', f);
}

// Una tabella [giorno][ora][classe]: valore(pos, arg) per lo slot di quel giorno, ora e classe.
static void tabella_per_slot(FILE *f, long long int (*valore)(int pos, int arg), const int arg) {
    fputc('[', f);
    for (int g = 0; g < actual_giorni_settimana; g++) {
        fputs(g ? ", [" : "[", f);
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            fputs(o ? ", [" : "[", f);
            for (int c = 0; c < actual_classi_sezione; c++)
                fprintf(f, c ? ", %lld" : "%lld", valore(slot_di(g, o, c), arg));
            fputc(']', f);
        }
        fputc(']', f);
    }
    fputc(']', f);
}

static long long int marce(const int pos, const int arg) {
    (void) arg;
    return diagnosi_marce_nello_slot(pos);
}

static long long int scartati(const int pos, const int motivo_scarto) {
    return diagnosi_scartati_nello_slot(pos, motivo_scarto);
}

/* ---- le parti del file ---- */

static void scrivi_dati(FILE *f) {
    fputs("  \"dati\": {\n    \"giorni\": ", f);
    elenco_di_stringhe(f, giorni_settimana, actual_giorni_settimana);
    fputs(",\n    \"ore\": ", f);
    elenco_di_stringhe(f, ore_giornaliere, actual_ore_giornaliere);

    fputs(",\n    \"classi\": [", f);
    for (int c = 0; c < actual_classi_sezione; c++) {
        fputs(c ? ",\n      {\"nome\": " : "\n      {\"nome\": ", f);
        stringa(f, classe_sezione[c]);
        fprintf(f, ", \"ore\": %d}", classe_info[c].ore);
    }

    fputs("\n    ],\n    \"docenti\": [", f);
    for (int p = 0; p < actual_insegnanti; p++) {
        fputs(p ? ",\n      {\"nome\": " : "\n      {\"nome\": ", f);
        stringa(f, insegnanti[p]);
        fprintf(f, ", \"ore\": %d, \"ingresso\": %d}", docente_info[p].ore, docente_info[p].ora_entrata);
    }

    // le lezioni: ore da solo di ogni docente e ore di ogni coppia, classe per classe
    fputs("\n    ],\n    \"lezioni\": [", f);
    int prima = 1;
    for (int c = 0; c < actual_classi_sezione; c++)
        for (int p = 0; p < actual_insegnanti; p++) {
            const int da_solo = classe_docente[c * INSEGNANTE_SZ + p].alone;
            if (da_solo > 0) {
                fprintf(f, "%s\n      {\"classe\": %d, \"docente\": %d, \"codocente\": null, \"ore\": %d}",
                        prima ? "" : ",", c, p, da_solo);
                prima = 0;
            }
            for (int q = p + 1; q < actual_insegnanti; q++) {
                const int insieme = codocenza[idx_codocenza(c, p, q)].ore;
                if (insieme <= 0) continue;
                fprintf(f, "%s\n      {\"classe\": %d, \"docente\": %d, \"codocente\": %d, \"ore\": %d}",
                        prima ? "" : ",", c, p, q, insieme);
                prima = 0;
            }
        }
    // ordine iniziale della coda dei docenti: a parita' di priorita' decide chi si prova prima, quindi serve per
    // rifare esattamente la stessa ricerca rileggendo questi dati con --input
    fputs("\n    ],\n    \"ordine_coda\": [", f);
    for (int i = 0; i < queue_sz; i++) fprintf(f, i ? ", %d" : "%d", ordine_iniziale_coda[i]);
    fputs("]\n  },\n", f);
}

static void scrivi_esito(FILE *f, const char *impossibile, const double secondi) {
    fputs("  \"ordine_classi\": [", f);
    for (int k = 0; k < actual_classi_sezione; k++) fprintf(f, k ? ", %d" : "%d", orario[k].classe);
    fputs("],\n  \"impossibile\": ", f);
    if (impossibile) stringa(f, impossibile);
    else fputs("null", f);
    fprintf(f, ",\n  \"esito\": {\"orari_trovati\": %lld, \"interrotto_per_tempo\": %s, \"interrotto_per_spazio\": %s, "
               "\"errore_di_scrittura\": %s, \"interrotto_su_richiesta\": %s, "
               "\"secondi\": %.3f, "
               "\"nodi\": %lld, \"marce_indietro\": %lld, \"posizione_massima\": %d, \"slot\": %d},\n",
            n_gen, interrotto_per_tempo ? "true" : "false", interrotto_per_spazio ? "true" : "false",
            errore_di_scrittura ? "true" : "false", interrotto_su_richiesta ? "true" : "false", secondi, nodi, n_backtrack, pos_max, orario_len);
}

// Il primo orario trovato, come [g][o][c] = [docente, codocente].
static void scrivi_orario(FILE *f) {
    fputs("  \"orario\": ", f);
    if (n_gen == 0) {
        fputs("null,\n", f);
        return;
    }
    fputc('[', f);
    for (int g = 0; g < actual_giorni_settimana; g++) {
        fputs(g ? ",\n    [" : "\n    [", f);
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            fputs(o ? ", [" : "[", f);
            for (int c = 0; c < actual_classi_sezione; c++) {
                const int i = slot_di(g, o, c);
                fprintf(f, c ? ", [%d, %d]" : "[%d, %d]", primo_orario_1[i], primo_orario_2[i]);
            }
            fputc(']', f);
        }
        fputc(']', f);
    }
    fputs("\n  ],\n", f);
}

static void scrivi_diagnosi(FILE *f) {
    fputs("  \"diagnosi\": {\n    \"marce\": ", f);
    tabella_per_slot(f, marce, 0);
    fputs(",\n    \"scartati\": {", f);
    for (int m = 0; m < N_MOTIVI_SCARTO; m++) {
        fputs(m ? ",\n      " : "\n      ", f);
        stringa(f, diagnosi_nome_motivo_scarto(m));
        fputs(": ", f);
        tabella_per_slot(f, scartati, m);
    }
    fputs("\n    },\n    \"nomi_motivi\": [", f);
    for (int m = 0; m < N_MOTIVI; m++) {
        if (m) fputs(", ", f);
        stringa(f, diagnosi_nome_motivo(m));
    }
    fputs("],\n    \"motivi\": ", f);
    if (!diagnosi_con_motivi()) {
        fputs("null,\n    \"motivi_docenti\": null\n  }\n", f);
        return;
    }
    fputc('[', f);
    for (int g = 0; g < actual_giorni_settimana; g++) {
        fputs(g ? ", [" : "[", f);
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            fputs(o ? ", [" : "[", f);
            for (int c = 0; c < actual_classi_sezione; c++) {
                const int i = slot_di(g, o, c);
                fputs(c ? ", [" : "[", f);
                for (int m = 0; m < N_MOTIVI; m++) fprintf(f, m ? ", %lld" : "%lld", diagnosi_motivo_nello_slot(i, m));
                fputc(']', f);
            }
            fputc(']', f);
        }
        fputc(']', f);
    }
    fputs("],\n    \"motivi_docenti\": [", f);
    for (int p = 0; p < actual_insegnanti; p++) {
        fputs(p ? ", [" : "[", f);
        for (int m = 0; m < N_MOTIVI; m++) fprintf(f, m ? ", %lld" : "%lld", diagnosi_motivo_del_docente(p, m));
        fputc(']', f);
    }
    fputs("]\n  }\n", f);
}

// Per ogni slot con blocchi: quante volte la ricerca ci e' arrivata e lo stato dell'ultimo blocco (vedi
// risultati_json.h). Solo gli slot con blocchi, per non appesantire il file.
static void scrivi_dettagli_slot(FILE *f) {
    fputs(",\n  \"dettagli_slot\": [", f);
    int primo = 1;
    for (int g = 0; g < actual_giorni_settimana; g++)
        for (int o = 0; o < actual_ore_giornaliere; o++)
            for (int c = 0; c < actual_classi_sezione; c++) {
                const int i = slot_di(g, o, c);
                long long int scartati_tot = 0;
                for (int m = 0; m < N_MOTIVI_SCARTO; m++) scartati_tot += diagnosi_scartati_nello_slot(i, m);
                const long long int marce_n = diagnosi_marce_nello_slot(i);
                if (!marce_n && !scartati_tot) continue;
                fprintf(f, "%s\n    {\"giorno\": %d, \"ora\": %d, \"classe\": %d, \"visite\": %lld, \"ore_classe\": %d, "
                           "\"marce\": %lld", primo ? "" : ",", g, o, c, diagnosi_visite_nello_slot(i), classe_info[c].ore,
                        marce_n);
                primo = 0;
                const stato_blocco_t um = diagnosi_ultima_marcia(i);
                if (um.ore_classe_fatte >= 0) {
                    fprintf(f, ", \"ultima_marcia\": {\"ore_classe_fatte\": %d, \"docenti_con_ore\": %d, \"docenti\": [",
                            um.ore_classe_fatte, um.docenti_con_ore);
                    for (int k = 0; k < um.n_docenti; k++)
                        fprintf(f, k ? ", [%d, %d, %d]" : "[%d, %d, %d]", um.docenti[k][0], um.docenti[k][1], um.docenti[k][2]);
                    fputs("]}", f);
                }
                fputs(", \"scarti\": [", f);
                int primo_scarto = 1;
                for (int m = 0; m < N_MOTIVI_SCARTO; m++) {
                    dettaglio_scarto_t d;
                    const stato_blocco_t us = diagnosi_ultimo_scarto(i, m, &d);
                    if (us.ore_classe_fatte < 0) continue;
                    fprintf(f, "%s{\"motivo\": ", primo_scarto ? "" : ", ");
                    primo_scarto = 0;
                    stringa(f, diagnosi_nome_motivo_scarto(m));
                    fprintf(f, ", \"volte\": %lld, \"ore_classe_fatte\": %d, \"scelta\": [%d, %d]",
                            diagnosi_scartati_nello_slot(i, m), us.ore_classe_fatte, us.scelta[0], us.scelta[1]);
                    if (m == SCARTATO_LEZIONI_DELLA_CLASSE) {
                        fprintf(f, ", \"fino_ora\": %d, \"da_riempire\": %d, \"possibili\": %d, \"oggi\": %d, "
                                   "\"slot_oggi\": %d, \"in_un_giorno\": %d, \"slot_al_giorno\": %d, \"giorni_dopo\": %d, "
                                   "\"lezioni\": [", d.fino_ora, d.da_riempire, d.possibili, d.oggi, d.slot_oggi,
                                d.in_un_giorno, d.slot_al_giorno, d.giorni_dopo);
                        for (int k = 0; k < d.n_lezioni; k++)
                            fprintf(f, k ? ", [%d, %d, %d, %d]" : "[%d, %d, %d, %d]", d.lezioni[k][0], d.lezioni[k][1],
                                    d.lezioni[k][2], d.lezioni[k][3]);
                        fputs("]}", f);
                    }
                    else
                        fprintf(f, ", \"docente\": %d, \"classe_docente\": %d, \"ore_rimaste\": %d, \"ore_possibili\": %d}",
                                d.docente, d.classe_docente, d.ore_rimaste, d.ore_possibili);
                }
                fputs("]}", f);
            }
    fputs("\n  ]\n", f);
}

void scrivi_json(const char *file, const int argc, char **argv, const char *impossibile, const double secondi) {
    if (!file) return;
    FILE *f = fopen(file, "w");
    if (!f) {
        fprintf(stderr, "Impossibile scrivere %s\n", file);
        return;
    }
    fputs("{\n  \"versione\": 1,\n  \"argomenti\": [", f);
    for (int i = 1; i < argc; i++) {
        if (i > 1) fputs(", ", f);
        stringa(f, argv[i]);
    }
    fputs("],\n", f);
    scrivi_dati(f);
    scrivi_esito(f, impossibile, secondi);
    scrivi_orario(f);
    scrivi_diagnosi(f);
    scrivi_dettagli_slot(f);
    fputs("}\n", f);
    fclose(f);
}
