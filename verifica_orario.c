#include "verifica_orario.h"

#include <stdio.h>
#include <string.h>

// Vedi verifica_orario.h. Qui dentro niente della ricerca: solo i dati (ore da solo, ore in coppia, ingressi, nomi)
// e l'orario da verificare.

static const teaching_slot *orario;
static int G, O, C, P;  // giorni, ore al giorno, classi, docenti

// la cella di (giorno, ora, classe)
static const teaching_slot *cella(const int g, const int o, const int c) {
    return &orario[(g * O + o) * C + c];
}

// il docente p e' in quella cella (come primo o come secondo)
static int presente(const int g, const int o, const int c, const int p) {
    return cella(g, o, c)->insegnante1 == p || cella(g, o, c)->insegnante2 == p;
}

// la cella ha lezione (almeno un docente)
static int lezione(const int g, const int o, const int c) {
    return cella(g, o, c)->insegnante1 >= 0 || cella(g, o, c)->insegnante2 >= 0;
}

// per i dettagli: «Lun 08, 3A»
static const char *dove(const int g, const int o, const int c) {
    static char testo[3 * MAX_LEN_WORDS + 8];
    snprintf(testo, sizeof testo, "%s %s, %s", giorni_settimana[g], ore_giornaliere[o], classe_sezione[c]);
    return testo;
}

// Prima di tutto: le celle sono scritte bene? Una cella con un docente che non esiste, due volte lo stesso, o solo
// il secondo, non e' un orario: gli altri controlli non si possono fare.
static int celle_scritte_bene(char *d, const size_t n) {
    for (int g = 0; g < G; g++)
        for (int o = 0; o < O; o++)
            for (int c = 0; c < C; c++) {
                const int a = cella(g, o, c)->insegnante1, b = cella(g, o, c)->insegnante2;
                if (a >= -1 && a < P && b >= -1 && b < P && (a >= 0 || b < 0) && (a < 0 || a != b)) continue;
                snprintf(d, n, "%s: la cella è scritta male (docenti %d e %d)", dove(g, o, c), a, b);
                return 0;
            }
    return 1;
}

// 0. Le ore. Classe per classe si contano nell'orario le ore di ogni docente da solo e di ogni coppia, e si
// confrontano con i dati: devono essere uguali (non una di piu', non una di meno).
static int ore_giuste(char *d, const size_t n) {
    static int da_solo[INSEGNANTE_SZ], insieme[INSEGNANTE_SZ][INSEGNANTE_SZ];
    for (int c = 0; c < C; c++) {
        memset(da_solo, 0, sizeof da_solo);
        memset(insieme, 0, sizeof insieme);
        for (int g = 0; g < G; g++)
            for (int o = 0; o < O; o++) {
                const int a = cella(g, o, c)->insegnante1, b = cella(g, o, c)->insegnante2;
                if (a < 0) continue;
                if (b < 0) da_solo[a]++;
                else insieme[a < b ? a : b][a < b ? b : a]++;
            }
        for (int p = 0; p < P; p++) {
            if (da_solo[p] != ore_classi_docenti_alone[c * INSEGNANTE_SZ + p]) {
                snprintf(d, n, "%s in %s da solo: %d ore nell'orario, %d nei dati", insegnanti[p], classe_sezione[c],
                         da_solo[p], ore_classi_docenti_alone[c * INSEGNANTE_SZ + p]);
                return 0;
            }
            for (int q = p + 1; q < P; q++) {
                if (insieme[p][q] == get_ore_insegnanti_codocenza_in_una_classe(c, p, q)) continue;
                snprintf(d, n, "%s con %s in %s: %d ore nell'orario, %d nei dati", insegnanti[p], insegnanti[q],
                         classe_sezione[c], insieme[p][q], get_ore_insegnanti_codocenza_in_una_classe(c, p, q));
                return 0;
            }
        }
    }
    return 1;
}

// 1. Una classe per volta: a ogni ora di ogni giorno un docente compare in al massimo una classe.
static int una_classe_per_volta(char *d, const size_t n) {
    static int dove_e[INSEGNANTE_SZ];  // la classe dove il docente e' a quest'ora (-1: in nessuna)
    for (int g = 0; g < G; g++)
        for (int o = 0; o < O; o++) {
            for (int p = 0; p < P; p++) dove_e[p] = -1;
            for (int c = 0; c < C; c++)
                for (int k = 0; k < 2; k++) {
                    const int p = k ? cella(g, o, c)->insegnante2 : cella(g, o, c)->insegnante1;
                    if (p < 0) continue;
                    if (dove_e[p] >= 0) {
                        snprintf(d, n, "%s %s: %s è in %s e in %s", giorni_settimana[g], ore_giornaliere[o],
                                 insegnanti[p], classe_sezione[dove_e[p]], classe_sezione[c]);
                        return 0;
                    }
                    dove_e[p] = c;
                }
        }
    return 1;
}

// 2. L'ingresso: nessun docente in una cella prima della sua ora di ingresso.
static int dopo_l_ingresso(char *d, const size_t n) {
    for (int g = 0; g < G; g++)
        for (int o = 0; o < O; o++)
            for (int c = 0; c < C; c++)
                for (int p = 0; p < P; p++)
                    if (o < ora_di_entrata_prof[p] && presente(g, o, c, p)) {
                        snprintf(d, n, "%s: %s c'è, ma entra alle %s", dove(g, o, c), insegnanti[p],
                                 ore_giornaliere[ora_di_entrata_prof[p]]);
                        return 0;
                    }
    return 1;
}

// 3 e 4. Un docente in una classe in un giorno: al massimo MAX_ORE_PROF_IN_UNA_CLASSE ore (quale == 3), e tutte di
// seguito (quale == 4: tra la prima e l'ultima non ci sono ore in cui manca).
static int ore_del_giorno_in_una_classe(const int quale, char *d, const size_t n) {
    for (int g = 0; g < G; g++)
        for (int c = 0; c < C; c++)
            for (int p = 0; p < P; p++) {
                int ore = 0, prima = -1, ultima = -1;
                for (int o = 0; o < O; o++) {
                    if (!presente(g, o, c, p)) continue;
                    ore++;
                    if (prima < 0) prima = o;
                    ultima = o;
                }
                if (quale == 3 && ore > MAX_ORE_PROF_IN_UNA_CLASSE) {
                    snprintf(d, n, "%s in %s %s: %d ore, il massimo è %d", insegnanti[p], classe_sezione[c],
                             giorni_settimana[g], ore, MAX_ORE_PROF_IN_UNA_CLASSE);
                    return 0;
                }
                if (quale == 4 && ore && ultima - prima + 1 != ore) {
                    snprintf(d, n, "%s in %s %s: %d ore tra le %s e le %s, non di seguito", insegnanti[p],
                             classe_sezione[c], giorni_settimana[g], ore, ore_giornaliere[prima],
                             ore_giornaliere[ultima]);
                    return 0;
                }
            }
    return 1;
}

// 5. Le classi al giorno: ogni docente, ogni giorno, in al massimo MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi.
static int classi_al_giorno(char *d, const size_t n) {
    for (int g = 0; g < G; g++)
        for (int p = 0; p < P; p++) {
            int classi = 0;
            for (int c = 0; c < C; c++) {
                int qui = 0;
                for (int o = 0; o < O && !qui; o++) qui = presente(g, o, c, p);
                classi += qui;
            }
            if (classi <= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) continue;
            snprintf(d, n, "%s %s: %d classi, il massimo è %d", insegnanti[p], giorni_settimana[g], classi,
                     MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
            return 0;
        }
    return 1;
}

// 6. Le ore obbligatorie: nelle prime PRIME_N_ORE_OBBLIGATORIE ore di ogni giorno ogni classe ha lezione.
static int ore_obbligatorie(char *d, const size_t n) {
    for (int g = 0; g < G; g++)
        for (int o = 0; o < O && o < PRIME_N_ORE_OBBLIGATORIE; o++)
            for (int c = 0; c < C; c++)
                if (!lezione(g, o, c)) {
                    snprintf(d, n, "%s: non c'è lezione, ma l'ora è obbligatoria", dove(g, o, c));
                    return 0;
                }
    return 1;
}

// 7. L'ora buca chiude la giornata: in una classe, dopo un'ora senza lezione, quel giorno non ce ne sono altre.
static int dopo_l_ora_buca(char *d, const size_t n) {
    for (int g = 0; g < G; g++)
        for (int c = 0; c < C; c++) {
            int buca = -1;
            for (int o = 0; o < O; o++) {
                if (!lezione(g, o, c)) {
                    if (buca < 0) buca = o;
                    continue;
                }
                if (buca < 0) continue;
                snprintf(d, n, "%s: c'è lezione dopo l'ora buca delle %s", dove(g, o, c), ore_giornaliere[buca]);
                return 0;
            }
        }
    return 1;
}

int verifica_orario(const teaching_slot *da_verificare, ControlloOrario k[VERIFICA_ORARIO_CONTROLLI]) {
    static char nomi[VERIFICA_ORARIO_CONTROLLI][100];
    orario = da_verificare;
    G = actual_giorni_settimana, O = actual_ore_giornaliere, C = actual_classi_sezione, P = actual_insegnanti;
    snprintf(nomi[0], sizeof nomi[0], "ogni docente e ogni coppia fanno in ogni classe le ore dei dati");
    snprintf(nomi[1], sizeof nomi[1], "un docente è in una sola classe per volta");
    snprintf(nomi[2], sizeof nomi[2], "nessuna lezione prima dell'ingresso del docente");
    snprintf(nomi[3], sizeof nomi[3], "al massimo %d ore al giorno di un docente nella stessa classe",
             MAX_ORE_PROF_IN_UNA_CLASSE);
    snprintf(nomi[4], sizeof nomi[4], "le ore di un docente in una classe, in un giorno, sono di seguito");
    snprintf(nomi[5], sizeof nomi[5], "al massimo %d classi al giorno per docente",
             MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
    snprintf(nomi[6], sizeof nomi[6], "le prime %d ore di ogni giorno hanno lezione in ogni classe",
             PRIME_N_ORE_OBBLIGATORIE);
    snprintf(nomi[7], sizeof nomi[7], "dopo un'ora buca la classe quel giorno non ha più lezione");
    const size_t n = sizeof k[0].dettaglio;
    for (int i = 0; i < VERIFICA_ORARIO_CONTROLLI; i++) {
        k[i].nome = nomi[i];
        k[i].ok = 0;
        k[i].dettaglio[0] = 0;
    }
    if (!celle_scritte_bene(k[0].dettaglio, n)) {  // non e' un orario: niente e' verificato
        for (int i = 1; i < VERIFICA_ORARIO_CONTROLLI; i++)
            snprintf(k[i].dettaglio, n, "non verificato: l'orario è scritto male");
        return 0;
    }
    k[0].ok = ore_giuste(k[0].dettaglio, n);
    k[1].ok = una_classe_per_volta(k[1].dettaglio, n);
    k[2].ok = dopo_l_ingresso(k[2].dettaglio, n);
    k[3].ok = ore_del_giorno_in_una_classe(3, k[3].dettaglio, n);
    k[4].ok = ore_del_giorno_in_una_classe(4, k[4].dettaglio, n);
    k[5].ok = classi_al_giorno(k[5].dettaglio, n);
    k[6].ok = ore_obbligatorie(k[6].dettaglio, n);
    k[7].ok = dopo_l_ora_buca(k[7].dettaglio, n);
    int tutti = 1;
    for (int i = 0; i < VERIFICA_ORARIO_CONTROLLI; i++) tutti = tutti && k[i].ok;
    return tutti;
}
