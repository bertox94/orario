#include "verifica.h"

#include <stdio.h>
#include <stdlib.h>

// Come guarda: un giorno alla volta, e dentro il giorno un'ora alla volta e una classe alla volta. Le regole che
// riguardano una cella (prime ore, ora buca, ingresso, una classe per volta) si controllano li'; quelle sulla giornata
// di un docente (ore al giorno, di seguito, classi al giorno) alla fine di ogni giorno; le ore di ogni voce alla fine.
// Non usa niente della ricerca (ne' i suoi conti, ne' le sue tabelle): solo la scuola e le celle.

// le regole, nell'ordine in cui compaiono nel risultato
enum { ORE_GIUSTE, UNA_CLASSE_PER_VOLTA, DOPO_L_INGRESSO, ORE_AL_GIORNO, DI_SEGUITO, CLASSI_AL_GIORNO, PRIME_ORE_PIENE,
       DOPO_LA_BUCA };

// segna che la regola non e' rispettata, con il primo punto in cui non lo e' (i successivi non si scrivono)
#define NON_RISPETTATA(c, ...) \
    do { \
        if ((c)->rispettata) snprintf((c)->dove, sizeof (c)->dove, __VA_ARGS__); \
        (c)->rispettata = 0; \
    } while (0)

int verifica_orario(const Scuola *s, const int *orario, Controllo c[CONTROLLI]) {
    const int G = s->giorni, O = s->ore, C = s->classi, T = s->docenti;
    const char *regole[CONTROLLI] = {
        "ogni docente e ogni coppia fanno in ogni classe le loro ore",
        "un docente e' in una sola classe per volta",
        "nessuna lezione prima dell'ingresso del docente",
        "in una classe, in un giorno, un docente fa al massimo le ore consentite",
        "le ore di un docente in una classe, in un giorno, sono di seguito",
        "un docente insegna in al massimo le classi consentite al giorno",
        "le prime ore di ogni giorno hanno lezione in ogni classe",
        "dopo un'ora buca la classe quel giorno non ha piu' lezione",
    };
    for (int i = 0; i < CONTROLLI; i++) c[i] = (Controllo){regole[i], 1, ""};
    int *fatte = calloc(s->voci, sizeof(int));  // le ore di ogni voce nell'orario
    int *dove_e = malloc(sizeof(int) * T);      // in un'ora: la classe dove il docente insegna (-1: nessuna)
    int *ore = malloc(sizeof(int) * C * T);     // in un giorno, per classe e docente: in quali ore c'e' (un bit l'una)
    if (!fatte || !dove_e || !ore) return 0;
    for (int g = 0; g < G; g++) {
        for (int i = 0; i < C * T; i++) ore[i] = 0;
        for (int o = 0; o < O; o++) {
            for (int t = 0; t < T; t++) dove_e[t] = -1;
            for (int k = 0; k < C; k++) {
                const int v = orario[(g * O + o) * C + k];  // che cosa c'e' nella cella
                const int buca_prima = o > 0 && orario[(g * O + o - 1) * C + k] < 0;  // nell'ora prima: un'ora buca
                if (v < 0) {  // un'ora buca: non deve essere una delle prime ore
                    if (o < s->obbligatorie)
                        NON_RISPETTATA(&c[PRIME_ORE_PIENE], "%s %s, %s: non c'e' lezione", s->nome_giorno[g],
                                       s->nome_ora[o], s->nome_classe[k]);
                    continue;
                }
                // una lezione: deve essere una voce che esiste, e di questa classe
                if (v >= s->voci || s->voce[v].classe != k) {
                    NON_RISPETTATA(&c[ORE_GIUSTE], "%s %s, %s: una lezione che non e' di questa classe",
                                   s->nome_giorno[g], s->nome_ora[o], s->nome_classe[k]);
                    continue;
                }
                fatte[v]++;
                if (buca_prima)
                    NON_RISPETTATA(&c[DOPO_LA_BUCA], "%s %s, %s: c'e' lezione dopo un'ora buca", s->nome_giorno[g],
                                   s->nome_ora[o], s->nome_classe[k]);
                for (int i = 0; i < docenti_della_voce(&s->voce[v]); i++) {
                    const int t = s->voce[v].docente[i];
                    if (dove_e[t] >= 0)
                        NON_RISPETTATA(&c[UNA_CLASSE_PER_VOLTA], "%s %s: %s e' in %s e in %s", s->nome_giorno[g],
                                       s->nome_ora[o], s->nome_docente[t], s->nome_classe[dove_e[t]],
                                       s->nome_classe[k]);
                    dove_e[t] = k;
                    // (i nomi delle ore sono quelli della scuola: l'ingresso e' un indice tra le sue ore)
                    if (o < s->ingresso[t])
                        NON_RISPETTATA(&c[DOPO_L_INGRESSO], "%s %s, %s: %s c'e', ma entra alle %s", s->nome_giorno[g],
                                       s->nome_ora[o], s->nome_classe[k], s->nome_docente[t],
                                       s->nome_ora[s->ingresso[t]]);
                    ore[k * T + t] |= 1 << o;  // in quest'ora c'era: si accende il bit dell'ora
                }
            }
        }
        // la giornata di ogni docente: classe per classe quante ore e se sono di seguito, poi quante classi
        for (int t = 0; t < T; t++) {
            int classi = 0;
            for (int k = 0; k < C; k++) {
                const int sue = ore[k * T + t];
                if (!sue) continue;
                classi++;
                // quante ore, la prima e l'ultima: sono di seguito se tra la prima e l'ultima ci sono solo loro
                int quante = 0, prima = -1, ultima = -1;
                for (int o = 0; o < O; o++)
                    if (sue >> o & 1) quante++, prima = prima < 0 ? o : prima, ultima = o;
                if (quante > MAX_ORE_DI_SEGUITO)
                    NON_RISPETTATA(&c[ORE_AL_GIORNO], "%s in %s, %s: %d ore, il massimo e' %d", s->nome_docente[t],
                                   s->nome_classe[k], s->nome_giorno[g], quante, MAX_ORE_DI_SEGUITO);
                if (ultima - prima + 1 != quante)
                    NON_RISPETTATA(&c[DI_SEGUITO], "%s in %s, %s: %d ore tra le %s e le %s, non di seguito",
                                   s->nome_docente[t], s->nome_classe[k], s->nome_giorno[g], quante,
                                   s->nome_ora[prima], s->nome_ora[ultima]);
            }
            if (classi > MAX_CLASSI_AL_GIORNO)
                NON_RISPETTATA(&c[CLASSI_AL_GIORNO], "%s, %s: %d classi, il massimo e' %d", s->nome_docente[t],
                               s->nome_giorno[g], classi, MAX_CLASSI_AL_GIORNO);
        }
    }
    // ogni voce: nell'orario ci sono esattamente le sue ore
    for (int v = 0; v < s->voci; v++) {
        const Voce *voce = &s->voce[v];
        if (fatte[v] != voce->ore)
            NON_RISPETTATA(&c[ORE_GIUSTE], "%s%s%s in %s: %d ore nell'orario, devono essere %d",
                           s->nome_docente[voce->docente[0]], voce->docente[1] < 0 ? "" : " con ",
                           voce->docente[1] < 0 ? "" : s->nome_docente[voce->docente[1]], s->nome_classe[voce->classe],
                           fatte[v], voce->ore);
    }
    free(fatte), free(dove_e), free(ore);
    int tutte = 1;
    for (int i = 0; i < CONTROLLI; i++) tutte = tutte && c[i].rispettata;
    return tutte;
}
