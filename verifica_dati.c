#include "verifica_dati.h"
#include "data_initialization/database_initializers.h"

#include <stdio.h>

// La verifica dei dati (vedi verifica_dati.h): cinque controlli, uno per funzione, nell'ordine in cui si fanno. Ognuno
// ritorna 1 se i dati lo passano; se no scrive il motivo in errore e ritorna 0. Leggono soltanto i dati.
//
// Le regole della ricerca su cui si basano (vincoli.h): un docente e' in una sola classe per volta, non insegna prima
// della sua ora di ingresso, fa al massimo MAX_ORE_PROF_IN_UNA_CLASSE (3) ore al giorno in una classe e insegna in al
// massimo MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO (3) classi al giorno; le prime PRIME_N_ORE_OBBLIGATORIE (5) ore
// di ogni giorno ogni classe ha lezione.

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// dove va il motivo del primo controllo che non vale (gli argomenti di verifica_dati)
static char *errore;
static size_t dimensione;

// ---- i dati, come servono ai controlli ----

// l'ora (indice) da cui il docente p e' a scuola: 0 la prima
static int ingresso(const int p) {
    return ora_di_entrata_prof[p];
}

// le ore del docente p nella classe c, da solo e in coppia (anche quelle che fa come codocente)
static int ore_in_classe(const int c, const int p) {
    return ore_classi_docenti[c * INSEGNANTE_SZ + p];
}

// le ore del docente p in tutte le classi
static int ore_in_tutto(const int p) {
    int ore = 0;
    for (int c = 0; c < actual_classi_sezione; c++) ore += ore_in_classe(c, p);
    return ore;
}

// per i messaggi: «entrando alle 10 » se il docente entra dopo la prima ora, se no niente
static const char *entrando(const int p) {
    static char testo[MAX_LEN_WORDS + 20];
    if (ingresso(p)) snprintf(testo, sizeof testo, "entrando alle %s ", ore_giornaliere[ingresso(p)]);
    else testo[0] = 0;
    return testo;
}

// ---- i controlli ----

// 1. Un docente in una classe. Al giorno ci puo' fare al massimo 3 ore, e meno se entra cosi' tardi che gliene restano
// meno di 3 (O ore al giorno, ingresso k: O - k). In G giorni: ore <= G x min(3, O - k).
// Per una coppia non serve un controllo a parte: le ore insieme sono anche ore di chi dei due entra piu' tardi.
static int docente_in_una_classe(void) {
    const int G = actual_giorni_settimana;
    for (int p = 0; p < actual_insegnanti; p++) {
        const int al_giorno = MIN(MAX_ORE_PROF_IN_UNA_CLASSE, actual_ore_giornaliere - ingresso(p));
        for (int c = 0; c < actual_classi_sezione; c++) {
            if (ore_in_classe(c, p) <= G * al_giorno) continue;
            snprintf(errore, dimensione, "%s in %s: %d ore, ma %sne può fare al massimo %d (%d al giorno per %d giorni)",
                     insegnanti[p], classe_sezione[c], ore_in_classe(c, p), entrando(p), G * al_giorno, al_giorno, G);
            return 0;
        }
    }
    return 1;
}

// 2. Un docente in tutto. E' in una sola classe per volta, quindi fa al massimo un'ora per ogni ora che e' a scuola:
// ore in tutte le classi <= G x (O - k).
static int docente_in_tutto(void) {
    const int G = actual_giorni_settimana;
    for (int p = 0; p < actual_insegnanti; p++) {
        const int al_giorno = actual_ore_giornaliere - ingresso(p);
        if (ore_in_tutto(p) <= G * al_giorno) continue;
        snprintf(errore, dimensione, "%s: %d ore in tutto, ma %sne può fare al massimo %d (%d al giorno per %d giorni)",
                 insegnanti[p], ore_in_tutto(p), entrando(p), G * al_giorno, al_giorno, G);
        return 0;
    }
    return 1;
}

// 3. Un docente e le classi al giorno. In una classe fa al massimo 3 ore al giorno: una classe dove ha n ore lo vede
// in almeno n / 3 giorni (arrotondato in su). Ogni giorno insegna in al massimo 3 classi, quindi in una settimana ha
// 3 x G «giorni in una classe» da spendere: la somma dei giorni che servono alle sue classi deve starci.
static int docente_classi_al_giorno(void) {
    const int disponibili = MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO * actual_giorni_settimana;
    for (int p = 0; p < actual_insegnanti; p++) {
        int servono = 0;
        for (int c = 0; c < actual_classi_sezione; c++)
            servono += (ore_in_classe(c, p) + MAX_ORE_PROF_IN_UNA_CLASSE - 1) / MAX_ORE_PROF_IN_UNA_CLASSE;
        if (servono <= disponibili) continue;
        snprintf(errore, dimensione, "%s: con al massimo %d ore al giorno in una classe, le sue classi gli prendono in "
                 "tutto almeno %d giorni, ma con %d classi al giorno per %d giorni ne ha %d", insegnanti[p],
                 MAX_ORE_PROF_IN_UNA_CLASSE, servono, MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO,
                 actual_giorni_settimana, disponibili);
        return 0;
    }
    return 1;
}

// Quante lezioni puo' fare in una settimana, tra la prima ora e l'ora h, un docente da solo o una coppia che ha `ore`
// ore nella classe ed entra all'ora `entra` (per una coppia: chi dei due entra piu' tardi): ogni giorno le ore tra
// l'ingresso e h, al massimo 3, e in tutto non piu' delle sue ore.
static int lezioni_fino_a(const int ore, const int entra, const int h) {
    return MIN(ore, actual_giorni_settimana * MIN(MAX_ORE_PROF_IN_UNA_CLASSE, MAX(0, h + 1 - entra)));
}

// 4. Una classe e le ore obbligatorie. Per ogni ora obbligatoria h la classe ha lezione dalla prima ora fino ad h
// tutti i giorni: (h + 1) x G celle da riempire. Le lezioni che possono starci sono la somma di lezioni_fino_a per
// ogni docente da solo (le sue ore senza codocente) e per ogni coppia (le ore insieme): se non bastano, qualche cella
// resta senza docente. E' il controllo che scarta i casi con molti docenti che entrano tardi.
static int classe_ore_obbligatorie(void) {
    const int G = actual_giorni_settimana;
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int h = 0; h < MIN(PRIME_N_ORE_OBBLIGATORIE, actual_ore_giornaliere); h++) {
            int possibili = 0;
            for (int p = 0; p < actual_insegnanti; p++) {
                if (!ore_in_classe(c, p)) continue;  // non e' un docente di questa classe
                possibili += lezioni_fino_a(ore_classi_docenti_alone[c * INSEGNANTE_SZ + p], ingresso(p), h);
                for (int q = p + 1; q < actual_insegnanti; q++)
                    possibili += lezioni_fino_a(get_ore_insegnanti_codocenza_in_una_classe(c, p, q),
                                                MAX(ingresso(p), ingresso(q)), h);
            }
            if (possibili >= (h + 1) * G) continue;
            // nel messaggio: «alle 08» per la prima ora, «dalle 08 alle 10» per le altre
            char quando[2 * MAX_LEN_WORDS + 20];
            if (h) snprintf(quando, sizeof quando, "dalle %s alle %s", ore_giornaliere[0], ore_giornaliere[h]);
            else snprintf(quando, sizeof quando, "alle %s", ore_giornaliere[0]);
            snprintf(errore, dimensione, "%s: %s servono %d lezioni in una settimana (%d al giorno), ma con le ore e "
                     "gli ingressi dei suoi docenti ce ne possono essere al massimo %d", classe_sezione[c], quando,
                     (h + 1) * G, h + 1, possibili);
            return 0;
        }
    }
    return 1;
}

// 5. Ora per ora. A un'ora obbligatoria ogni classe ha lezione e ogni lezione ha almeno un docente tutto suo: i
// docenti gia' entrati a quell'ora (tra quelli che hanno lezioni) devono essere almeno quante le classi.
static int docenti_entrati_a_ogni_ora(void) {
    for (int h = 0; h < MIN(PRIME_N_ORE_OBBLIGATORIE, actual_ore_giornaliere); h++) {
        int entrati = 0;
        for (int p = 0; p < actual_insegnanti; p++) entrati += ingresso(p) <= h && ore_in_tutto(p) > 0;
        if (entrati >= actual_classi_sezione) continue;
        snprintf(errore, dimensione, "alle %s sono entrati solo %d docenti, ma le classi sono %d e l'ora è obbligatoria",
                 ore_giornaliere[h], entrati, actual_classi_sezione);
        return 0;
    }
    return 1;
}

// I cinque controlli in ordine: si ferma al primo che non vale, con il suo motivo in testo.
int verifica_dati(char *testo, const size_t lunghezza) {
    errore = testo;
    dimensione = lunghezza;
    return docente_in_una_classe()
           && docente_in_tutto()
           && docente_classi_al_giorno()
           && classe_ore_obbligatorie()
           && docenti_entrati_a_ogni_ora();
}
