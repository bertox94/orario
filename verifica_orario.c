#include "verifica_orario.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Vedi verifica_orario.h. Qui dentro niente della ricerca: solo i dati (ore da solo, ore in coppia, ingressi, nomi)
// e l'orario da verificare, che ogni funzione riceve insieme: un Orario.

typedef struct {
    const Scuola *s;
    const teaching_slot *celle;
    int G, O, C, P;  // giorni, ore al giorno, classi, docenti
} Orario;

// la cella di (giorno, ora, classe)
static const teaching_slot *cella(const Orario *x, const int g, const int o, const int c) {
    return &x->celle[(g * x->O + o) * x->C + c];
}

// il docente p e' in quella cella (come primo o come secondo)
static int presente(const Orario *x, const int g, const int o, const int c, const int p) {
    return cella(x, g, o, c)->insegnante1 == p || cella(x, g, o, c)->insegnante2 == p;
}

// la cella ha lezione (almeno un docente)
static int lezione(const Orario *x, const int g, const int o, const int c) {
    return cella(x, g, o, c)->insegnante1 >= 0 || cella(x, g, o, c)->insegnante2 >= 0;
}

// per i dettagli: «Lun 08, 3A» (si usa dove(x, g, o, c).testo)
typedef struct {
    char testo[3 * MAX_LEN_WORDS + 8];
} Dove;

static Dove dove(const Orario *x, const int g, const int o, const int c) {
    Dove d;
    snprintf(d.testo, sizeof d.testo, "%s %s, %s", x->s->giorni_settimana[g], x->s->ore_giornaliere[o],
             x->s->classe_sezione[c]);
    return d;
}

// Prima di tutto: le celle sono scritte bene? Una cella con un docente che non esiste, due volte lo stesso, o solo
// il secondo, non e' un orario: gli altri controlli non si possono fare.
static int celle_scritte_bene(const Orario *x, char *d, const size_t n) {
    for (int g = 0; g < x->G; g++)
        for (int o = 0; o < x->O; o++)
            for (int c = 0; c < x->C; c++) {
                const int a = cella(x, g, o, c)->insegnante1, b = cella(x, g, o, c)->insegnante2;
                if (a >= -1 && a < x->P && b >= -1 && b < x->P && (a >= 0 || b < 0) && (a < 0 || a != b)) continue;
                snprintf(d, n, "%s: la cella è scritta male (docenti %d e %d)", dove(x, g, o, c).testo, a, b);
                return 0;
            }
    return 1;
}

// 0. Le ore. Classe per classe si contano nell'orario le ore di ogni docente da solo e di ogni coppia, e si
// confrontano con i dati: devono essere uguali (non una di piu', non una di meno).
static int ore_giuste(const Orario *x, char *d, const size_t n) {
    const Scuola *s = x->s;
    const int P = x->P;
    int da_solo[INSEGNANTE_SZ];
    int *insieme = malloc(sizeof(int) * P * P);  // insieme[p * P + q], p < q: le ore della coppia in questa classe
    assert(insieme);
    int ok = 1;
    for (int c = 0; c < x->C && ok; c++) {
        for (int p = 0; p < P; p++) da_solo[p] = 0;
        for (int i = 0; i < P * P; i++) insieme[i] = 0;
        for (int g = 0; g < x->G; g++)
            for (int o = 0; o < x->O; o++) {
                const int a = cella(x, g, o, c)->insegnante1, b = cella(x, g, o, c)->insegnante2;
                if (a < 0) continue;
                if (b < 0) da_solo[a]++;
                else insieme[a < b ? a * P + b : b * P + a]++;
            }
        for (int p = 0; p < P && ok; p++) {
            const int solo = s->ore_classi_docenti_alone[c * INSEGNANTE_SZ + p];
            if (da_solo[p] != solo) {
                snprintf(d, n, "%s in %s da solo: %d ore nell'orario, %d nei dati", s->insegnanti[p],
                         s->classe_sezione[c], da_solo[p], solo);
                ok = 0;
            }
            for (int q = p + 1; q < P && ok; q++) {
                const int coppia = get_ore_insegnanti_codocenza_in_una_classe(s, c, p, q);
                if (insieme[p * P + q] == coppia) continue;
                snprintf(d, n, "%s con %s in %s: %d ore nell'orario, %d nei dati", s->insegnanti[p], s->insegnanti[q],
                         s->classe_sezione[c], insieme[p * P + q], coppia);
                ok = 0;
            }
        }
    }
    free(insieme);
    return ok;
}

// 1. Una classe per volta: a ogni ora di ogni giorno un docente compare in al massimo una classe.
static int una_classe_per_volta(const Orario *x, char *d, const size_t n) {
    const Scuola *s = x->s;
    int dove_e[INSEGNANTE_SZ];  // la classe dove il docente e' a quest'ora (-1: in nessuna)
    for (int g = 0; g < x->G; g++)
        for (int o = 0; o < x->O; o++) {
            for (int p = 0; p < x->P; p++) dove_e[p] = -1;
            for (int c = 0; c < x->C; c++)
                for (int k = 0; k < 2; k++) {
                    const int p = k ? cella(x, g, o, c)->insegnante2 : cella(x, g, o, c)->insegnante1;
                    if (p < 0) continue;
                    if (dove_e[p] >= 0) {
                        snprintf(d, n, "%s %s: %s è in %s e in %s", s->giorni_settimana[g], s->ore_giornaliere[o],
                                 s->insegnanti[p], s->classe_sezione[dove_e[p]], s->classe_sezione[c]);
                        return 0;
                    }
                    dove_e[p] = c;
                }
        }
    return 1;
}

// 2. L'ingresso: nessun docente in una cella prima della sua ora di ingresso.
static int dopo_l_ingresso(const Orario *x, char *d, const size_t n) {
    const Scuola *s = x->s;
    for (int g = 0; g < x->G; g++)
        for (int o = 0; o < x->O; o++)
            for (int c = 0; c < x->C; c++)
                for (int p = 0; p < x->P; p++)
                    if (o < s->ora_di_entrata_prof[p] && presente(x, g, o, c, p)) {
                        snprintf(d, n, "%s: %s c'è, ma entra alle %s", dove(x, g, o, c).testo, s->insegnanti[p],
                                 s->ore_giornaliere[s->ora_di_entrata_prof[p]]);
                        return 0;
                    }
    return 1;
}

// 3 e 4. Un docente in una classe in un giorno: al massimo MAX_ORE_PROF_IN_UNA_CLASSE ore (quale == 3), e tutte di
// seguito (quale == 4: tra la prima e l'ultima non ci sono ore in cui manca).
static int ore_del_giorno_in_una_classe(const Orario *x, const int quale, char *d, const size_t n) {
    const Scuola *s = x->s;
    for (int g = 0; g < x->G; g++)
        for (int c = 0; c < x->C; c++)
            for (int p = 0; p < x->P; p++) {
                int ore = 0, prima = -1, ultima = -1;
                for (int o = 0; o < x->O; o++) {
                    if (!presente(x, g, o, c, p)) continue;
                    ore++;
                    if (prima < 0) prima = o;
                    ultima = o;
                }
                if (quale == 3 && ore > MAX_ORE_PROF_IN_UNA_CLASSE) {
                    snprintf(d, n, "%s in %s %s: %d ore, il massimo è %d", s->insegnanti[p], s->classe_sezione[c],
                             s->giorni_settimana[g], ore, MAX_ORE_PROF_IN_UNA_CLASSE);
                    return 0;
                }
                if (quale == 4 && ore && ultima - prima + 1 != ore) {
                    snprintf(d, n, "%s in %s %s: %d ore tra le %s e le %s, non di seguito", s->insegnanti[p],
                             s->classe_sezione[c], s->giorni_settimana[g], ore, s->ore_giornaliere[prima],
                             s->ore_giornaliere[ultima]);
                    return 0;
                }
            }
    return 1;
}

// 5. Le classi al giorno: ogni docente, ogni giorno, in al massimo MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi.
static int classi_al_giorno(const Orario *x, char *d, const size_t n) {
    for (int g = 0; g < x->G; g++)
        for (int p = 0; p < x->P; p++) {
            int classi = 0;
            for (int c = 0; c < x->C; c++) {
                int qui = 0;
                for (int o = 0; o < x->O && !qui; o++) qui = presente(x, g, o, c, p);
                classi += qui;
            }
            if (classi <= MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO) continue;
            snprintf(d, n, "%s %s: %d classi, il massimo è %d", x->s->insegnanti[p], x->s->giorni_settimana[g], classi,
                     MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
            return 0;
        }
    return 1;
}

// 6. Le ore obbligatorie: nelle prime PRIME_N_ORE_OBBLIGATORIE ore di ogni giorno ogni classe ha lezione.
static int ore_obbligatorie(const Orario *x, char *d, const size_t n) {
    for (int g = 0; g < x->G; g++)
        for (int o = 0; o < x->O && o < PRIME_N_ORE_OBBLIGATORIE; o++)
            for (int c = 0; c < x->C; c++)
                if (!lezione(x, g, o, c)) {
                    snprintf(d, n, "%s: non c'è lezione, ma l'ora è obbligatoria", dove(x, g, o, c).testo);
                    return 0;
                }
    return 1;
}

// 7. L'ora buca chiude la giornata: in una classe, dopo un'ora senza lezione, quel giorno non ce ne sono altre.
static int dopo_l_ora_buca(const Orario *x, char *d, const size_t n) {
    for (int g = 0; g < x->G; g++)
        for (int c = 0; c < x->C; c++) {
            int buca = -1;
            for (int o = 0; o < x->O; o++) {
                if (!lezione(x, g, o, c)) {
                    if (buca < 0) buca = o;
                    continue;
                }
                if (buca < 0) continue;
                snprintf(d, n, "%s: c'è lezione dopo l'ora buca delle %s", dove(x, g, o, c).testo,
                         x->s->ore_giornaliere[buca]);
                return 0;
            }
        }
    return 1;
}

int verifica_orario(const Scuola *s, const teaching_slot *celle, ControlloOrario k[VERIFICA_ORARIO_CONTROLLI]) {
    const Orario x = {s, celle, s->actual_giorni_settimana, s->actual_ore_giornaliere, s->actual_classi_sezione,
                      s->actual_insegnanti};
    const size_t n = sizeof k[0].dettaglio, m = sizeof k[0].nome;
    for (int i = 0; i < VERIFICA_ORARIO_CONTROLLI; i++) {
        k[i].ok = 0;
        k[i].dettaglio[0] = 0;
    }
    snprintf(k[0].nome, m, "ogni docente e ogni coppia fanno in ogni classe le ore dei dati");
    snprintf(k[1].nome, m, "un docente è in una sola classe per volta");
    snprintf(k[2].nome, m, "nessuna lezione prima dell'ingresso del docente");
    snprintf(k[3].nome, m, "al massimo %d ore al giorno di un docente nella stessa classe", MAX_ORE_PROF_IN_UNA_CLASSE);
    snprintf(k[4].nome, m, "le ore di un docente in una classe, in un giorno, sono di seguito");
    snprintf(k[5].nome, m, "al massimo %d classi al giorno per docente", MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO);
    snprintf(k[6].nome, m, "le prime %d ore di ogni giorno hanno lezione in ogni classe", PRIME_N_ORE_OBBLIGATORIE);
    snprintf(k[7].nome, m, "dopo un'ora buca la classe quel giorno non ha più lezione");
    if (!celle_scritte_bene(&x, k[0].dettaglio, n)) {  // non e' un orario: niente e' verificato
        for (int i = 1; i < VERIFICA_ORARIO_CONTROLLI; i++)
            snprintf(k[i].dettaglio, n, "non verificato: l'orario è scritto male");
        return 0;
    }
    k[0].ok = ore_giuste(&x, k[0].dettaglio, n);
    k[1].ok = una_classe_per_volta(&x, k[1].dettaglio, n);
    k[2].ok = dopo_l_ingresso(&x, k[2].dettaglio, n);
    k[3].ok = ore_del_giorno_in_una_classe(&x, 3, k[3].dettaglio, n);
    k[4].ok = ore_del_giorno_in_una_classe(&x, 4, k[4].dettaglio, n);
    k[5].ok = classi_al_giorno(&x, k[5].dettaglio, n);
    k[6].ok = ore_obbligatorie(&x, k[6].dettaglio, n);
    k[7].ok = dopo_l_ora_buca(&x, k[7].dettaglio, n);
    int tutti = 1;
    for (int i = 0; i < VERIFICA_ORARIO_CONTROLLI; i++) tutti = tutti && k[i].ok;
    return tutti;
}
