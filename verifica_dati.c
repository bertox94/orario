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
static int le_ore_del_docente_in_una_classe_stanno_nella_settimana(void) {
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
static int le_ore_del_docente_stanno_nella_settimana(void) {
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
static int le_classi_del_docente_stanno_in_tre_al_giorno(void) {
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

// Le lezioni che i docenti della classe c possono fare in una settimana tra la prima ora e l'ora h: la somma di
// lezioni_fino_a per ogni docente da solo (le sue ore senza codocente) e per ogni coppia (le ore insieme; entra quando
// entra il piu' tardivo dei due). Con dati != NULL scrive anche i numeri: una riga per ogni docente o coppia che
// in quelle ore puo' fare almeno una lezione, e in fondo una frase per gli altri (hanno ore nella classe ma in quelle
// ore non sono ancora entrati: e' l'unico modo di non poterne fare nessuna). In per_i_ritardi (se non e' NULL) mette 1
// quando almeno uno ne farebbe di piu' entrando alla prima ora: se resta 0, i ritardi non c'entrano.
static int lezioni_possibili(const int c, const int h, char *dati, const size_t n, int *per_i_ritardi) {
    int possibili = 0, con_ore = 0, non_entrati = 0;
    size_t scritti = 0;
    for (int p = 0; p < actual_insegnanti; p++) {
        if (!ore_in_classe(c, p)) continue;  // non e' un docente di questa classe
        for (int q = p; q < actual_insegnanti; q++) {  // q == p: il docente da solo; q > p: la coppia p, q
            const int ore = q == p ? ore_classi_docenti_alone[c * INSEGNANTE_SZ + p]
                                   : get_ore_insegnanti_codocenza_in_una_classe(c, p, q);
            if (!ore) continue;
            const int tardi = ingresso(q) > ingresso(p) ? q : p;  // chi dei due entra piu' tardi
            const int fa = lezioni_fino_a(ore, ingresso(tardi), h);
            possibili += fa;
            if (per_i_ritardi && fa < lezioni_fino_a(ore, 0, h)) *per_i_ritardi = 1;  // senza ritardo ne farebbe di piu'
            if (fa) con_ore++;
            else non_entrati++;
            if (!dati || !fa || scritti >= n) continue;  // nei numeri solo chi puo' fare almeno un'ora
            char chi[2 * MAX_LEN_WORDS + 10], entra[2 * MAX_LEN_WORDS + 20];
            if (q == p) snprintf(chi, sizeof chi, "%s da solo", insegnanti[p]);
            else snprintf(chi, sizeof chi, "%s con %s", insegnanti[p], insegnanti[q]);
            if (!ingresso(tardi)) snprintf(entra, sizeof entra, "dalla prima ora");
            else if (q == p) snprintf(entra, sizeof entra, "entra alle %s", ore_giornaliere[ingresso(p)]);
            else snprintf(entra, sizeof entra, "%s entra alle %s", insegnanti[tardi], ore_giornaliere[ingresso(tardi)]);
            scritti += snprintf(dati + scritti, n - scritti, "\n- %s: %d %s, %s: ne può fare %d", chi, ore,
                                ore == 1 ? "ora" : "ore", entra, fa);
        }
    }
    if (dati && non_entrati && scritti < n) {
        const char *quando = h ? "in queste ore" : "alla prima ora";
        if (con_ore) snprintf(dati + scritti, n - scritti, "\nGli altri %s non sono ancora entrati.", quando);
        else snprintf(dati + scritti, n - scritti, "\nNessuno dei suoi docenti %s è già entrato.", quando);
    }
    return possibili;
}

// 4. Una classe e le ore obbligatorie. Per ogni ora obbligatoria h la classe ha lezione dalla prima ora fino ad h
// tutti i giorni: (h + 1) x G celle da riempire. Se le lezioni possibili (lezioni_possibili) non bastano, qualche
// cella resta senza docente. E' il controllo che scarta i casi con molti docenti che entrano tardi.
// Il motivo: una frase, e sotto i numeri di ogni docente e di ogni coppia della classe (le sue ore, da quando c'e',
// quante ne puo' fare in quelle ore).
static int le_lezioni_bastano_per_le_ore_obbligatorie(void) {
    const int G = actual_giorni_settimana;
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int h = 0; h < MIN(PRIME_N_ORE_OBBLIGATORIE, actual_ore_giornaliere); h++) {
            int ritardi = 0;  // la causa: i ritardi, o (se entrando tutti alla prima ora non cambia niente) le poche ore
            const int possibili = lezioni_possibili(c, h, NULL, 0, &ritardi);
            if (possibili >= (h + 1) * G) continue;
            char quando[40];
            if (h) snprintf(quando, sizeof quando, "nelle prime %d ore", h + 1);
            else snprintf(quando, sizeof quando, "nella prima ora");
            const int n = snprintf(errore, dimensione, "%s: %s, in una settimana, %s è possibile fare solo %d ore "
                                   "invece delle %d richieste", classe_sezione[c], quando,
                                   ritardi ? "a causa delle entrate in ritardo"
                                           : "perché i suoi docenti hanno poche ore (al massimo 3 al giorno ciascuno)",
                                   possibili, (h + 1) * G);
            if (n > 0 && (size_t) n < dimensione) lezioni_possibili(c, h, errore + n, dimensione - n, NULL);
            return 0;
        }
    }
    return 1;
}

// 5. Ora per ora. A un'ora obbligatoria ogni classe ha lezione e ogni lezione ha almeno un docente tutto suo: i
// docenti gia' entrati a quell'ora (tra quelli che hanno lezioni) devono essere almeno quante le classi.
// Il motivo: una frase, e sotto i nomi dei docenti gia' entrati a quell'ora e una frase per gli altri.
static int i_docenti_entrati_bastano_per_le_classi(void) {
    const int C = actual_classi_sezione;
    for (int h = 0; h < MIN(PRIME_N_ORE_OBBLIGATORIE, actual_ore_giornaliere); h++) {
        int entrati = 0, docenti = 0;  // docenti: quelli che hanno lezioni
        for (int p = 0; p < actual_insegnanti; p++) {
            if (!ore_in_tutto(p)) continue;
            docenti++;
            entrati += ingresso(p) <= h;
        }
        if (entrati >= C) continue;
        // la causa: i ritardi, se qualcuno deve ancora entrare; se no i docenti sono proprio pochi
        int n = snprintf(errore, dimensione, "Alle %s, %s, ci sono solo %d docenti per %d classi: almeno %d "
                         "%s senza docente", ore_giornaliere[h], entrati < docenti ? "a causa delle entrate in ritardo"
                                                                         : "perché i docenti sono pochi",
                         entrati, C, C - entrati, C - entrati == 1 ? "classe resta" : "classi restano");
        for (int p = 0; p < actual_insegnanti && n > 0 && (size_t) n < dimensione; p++)
            if (ore_in_tutto(p) && ingresso(p) <= h) n += snprintf(errore + n, dimensione - n, "\n- %s", insegnanti[p]);
        if (entrati < docenti && n > 0 && (size_t) n < dimensione)
            snprintf(errore + n, dimensione - n, entrati ? "\nGli altri alle %s non sono ancora entrati."
                                                         : "\nNessun docente alle %s è già entrato.",
                     ore_giornaliere[h]);
        return 0;
    }
    return 1;
}

// I cinque controlli in ordine: si ferma al primo che non vale, con il suo motivo in testo.
int verifica_dati(char *testo, const size_t lunghezza) {
    errore = testo;
    dimensione = lunghezza;
    return le_ore_del_docente_in_una_classe_stanno_nella_settimana()
           && le_ore_del_docente_stanno_nella_settimana()
           && le_classi_del_docente_stanno_in_tre_al_giorno()
           && le_lezioni_bastano_per_le_ore_obbligatorie()
           && i_docenti_entrati_bastano_per_le_classi();
}
