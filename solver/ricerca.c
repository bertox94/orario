#include "ricerca.h"

#include <stdlib.h>

// La ricerca (il metodo e le parole sono spiegati in ricerca.h). Il file, dall'alto:
//   1. creare la ricerca            nuova_ricerca, libera_ricerca
//   2. mettere e togliere una scelta  segna
//   3. le scelte di una classe      puo_adesso, scelte_della_classe, quando_provarla, ordina_le_scelte,
//                                   scegli_la_classe
//   4. la ricerca                   scelte_del_nodo, prova_le_scelte, nodo, cerca_orario
//   5. riprendere una ricerca       ripercorri, riprendi_la_ricerca

// ---- 1. creare la ricerca ----

Ricerca *nuova_ricerca(const Scuola *s) {
    // nomi corti per le quattro misure, solo qui dove servono a ogni riga
    const int C = s->classi, T = s->docenti, G = s->giorni, O = s->ore;
    Ricerca *r = calloc(1, sizeof(Ricerca));
    if (!r) return NULL;
    r->s = s;
    r->celle = G * O * C;
    // (che cosa c'e' in ogni tabella, e come si trova un posto, e' scritto in ricerca.h accanto a ogni campo)
    r->orario = malloc(sizeof(int) * r->celle);
    r->resto_voce = calloc(s->voci, sizeof(int));
    r->resto_classe = calloc(C, sizeof(int));
    r->resto_docente = calloc(T, sizeof(int));
    r->resto_in_classe = calloc(C * T, sizeof(int));
    r->occupato = calloc(G * O * T, 1);
    r->ore_oggi = calloc(G * C * T, 1);
    r->classi_oggi = calloc(G * T, 1);
    r->chiusa = calloc(G * C, 1);
    r->docente_di = calloc(C * T, sizeof(int));
    r->n_docenti_di = calloc(C, sizeof(int));
    r->classe_di = calloc(T * C, sizeof(int));
    r->n_classi_di = calloc(T, sizeof(int));
    r->deve = calloc(G * O * T, 1);
    r->margine = calloc(G * O * T, sizeof(int));
    r->visto = calloc(T, sizeof(long long));
    r->posto = malloc(sizeof(int) * 2 * s->voci);
    r->ricordo = calloc(RICORDI, sizeof(Ricordo));
    r->ritorni_in = calloc(r->celle, sizeof(long long));
    // se anche una sola manca si libera tutto (libera_ricerca accetta i puntatori vuoti)
    if (!r->orario || !r->resto_voce || !r->resto_classe || !r->resto_docente || !r->resto_in_classe || !r->occupato
        || !r->ore_oggi || !r->classi_oggi || !r->chiusa || !r->docente_di || !r->n_docenti_di || !r->classe_di
        || !r->n_classi_di || !r->deve || !r->margine || !r->visto || !r->posto || !r->ricordo || !r->ritorni_in) {
        libera_ricerca(r);
        return NULL;
    }
    // l'orario vuoto
    for (int i = 0; i < r->celle; i++) r->orario[i] = VUOTA;
    // Una voce alla volta: le sue ore sono tutte da mettere, e contano per la sua classe e per i suoi docenti. Si
    // costruiscono anche gli elenchi che non cambiano piu': i docenti di ogni classe e le classi di ogni docente.
    for (int v = 0; v < s->voci; v++) {
        const Voce *voce = &s->voce[v];
        const int k = voce->classe;
        r->resto_voce[v] = voce->ore;
        r->resto_classe[k] += voce->ore;
        r->posto[2 * v + 1] = -1;  // il codocente: non c'e', finche' non lo si trova qui sotto
        for (int i = 0; i < docenti_della_voce(voce); i++) {
            const int t = voce->docente[i];
            r->resto_docente[t] += voce->ore;
            // La prima voce del docente in questa classe (finora qui non aveva ore): entra tra i docenti della
            // classe, e la classe tra le sue. Un docente puo' avere in una classe piu' voci: da solo e in coppia.
            if (!r->resto_in_classe[k * T + t]) {
                r->docente_di[k * T + r->n_docenti_di[k]++] = t;
                r->classe_di[t * C + r->n_classi_di[t]++] = k;
            }
            r->resto_in_classe[k * T + t] += voce->ore;
            // il posto del docente tra quelli della classe: serve a classe_sola.c, che li conta da 0
            for (int j = 0; j < r->n_docenti_di[k]; j++)
                if (r->docente_di[k * T + j] == t) r->posto[2 * v + i] = j;
        }
        // le scelte di una classe sono al massimo le sue voci, piu' l'ora buca
        r->max_scelte = MAX(r->max_scelte, s->prima_voce[k + 1] - s->prima_voce[k] + 1);
    }
    return r;
}

void libera_ricerca(Ricerca *r) {
    if (!r) return;
    free(r->orario), free(r->resto_voce), free(r->resto_classe), free(r->resto_docente), free(r->resto_in_classe);
    free(r->occupato), free(r->ore_oggi), free(r->classi_oggi), free(r->chiusa);
    free(r->docente_di), free(r->n_docenti_di), free(r->classe_di), free(r->n_classi_di);
    free(r->deve), free(r->margine), free(r->visto), free(r->posto), free(r->ricordo);
    free(r->ritorni_in);
    free(r);
}

// ---- 2. mettere e togliere una scelta ----

// Mette nella cella la voce v o l'ora buca (verso = 1), oppure la toglie (verso = -1), e tiene i conti: mettere e
// togliere fanno le stesse righe con il segno opposto, cosi' dopo un ritorno indietro tutto e' com'era.
void segna(Ricerca *r, const int cella, const int v, const int verso) {
    const Scuola *s = r->s;
    // dalla cella: la classe, l'ora nella settimana (da 0 a giorni * ore - 1) e il giorno
    const int k = cella % s->classi, ora_della_settimana = cella / s->classi, giorno = ora_della_settimana / s->ore;
    r->orario[cella] = verso > 0 ? v : VUOTA;
    if (v == BUCA) {
        // un'ora buca non consuma ore di nessuno: chiude solo la giornata della classe
        r->chiusa[giorno * s->classi + k] += verso;
        return;
    }
    // una lezione: un'ora in meno da mettere per la voce e per la classe...
    r->resto_voce[v] -= verso;
    r->resto_classe[k] -= verso;
    // ...e per ognuno dei suoi docenti (uno, o i due della coppia)
    for (int i = 0; i < docenti_della_voce(&s->voce[v]); i++) {
        const int t = s->voce[v].docente[i];
        unsigned char *ore_oggi = &r->ore_oggi[(giorno * s->classi + k) * s->docenti + t];
        r->resto_docente[t] -= verso;
        r->resto_in_classe[k * s->docenti + t] -= verso;
        r->occupato[ora_della_settimana * s->docenti + t] = verso > 0;
        // la prima ora di oggi in questa classe (o l'ultima che se ne va) cambia anche le classi di oggi
        if (verso > 0 && (*ore_oggi)++ == 0) r->classi_oggi[giorno * s->docenti + t]++;
        if (verso < 0 && --(*ore_oggi) == 0) r->classi_oggi[giorno * s->docenti + t]--;
    }
}

// ---- 3. le scelte di una classe in un'ora ----

// Perche' il docente t non puo' insegnare adesso nella classe k (PUO, cioe' 0, se puo'): deve essere entrato, essere
// libero, e rispettare le regole della giornata. La ricerca guarda solo se e' PUO; il perche' serve al sito.
PercheNo perche_non_puo(const Ricerca *r, const int giorno, const int ora, const int k, const int t) {
    const Scuola *s = r->s;
    if (s->ingresso[t] > ora) return NON_ENTRATO;
    // in quest'ora e' gia' in un'altra classe
    if (r->occupato[(giorno * s->ore + ora) * s->docenti + t]) return ALTROVE;
    const int fatte = r->ore_oggi[(giorno * s->classi + k) * s->docenti + t];
    // oggi qui non ha ancora insegnato: sarebbe una classe in piu' nella sua giornata
    if (!fatte) return r->classi_oggi[giorno * s->docenti + t] < MAX_CLASSI_AL_GIORNO ? PUO : TROPPE_CLASSI;
    // oggi qui ha gia' insegnato: non oltre il massimo, e solo di seguito (c'era nell'ora prima)
    if (fatte >= MAX_ORE_DI_SEGUITO) return MASSIMO_RAGGIUNTO;
    return presente(r, giorno, ora - 1, k, t) ? PUO : INTERROTTO;
}

static int puo_adesso(const Ricerca *r, const int giorno, const int ora, const int k, const int t) {
    return perche_non_puo(r, giorno, ora, k, t) == PUO;
}

// Le scelte della classe k in quest'ora, in `scelte`; ritorna quante sono.
// Nelle prime ore, obbligatorie, solo lezioni. Dopo: se oggi ha gia' avuto un'ora buca, solo l'ora buca; se no la
// lezione, purche' dopo le restino abbastanza ore per le prime ore dei giorni dopo, e l'ora buca, purche' le ore che
// le restano stiano nei giorni dopo. Una voce si puo' mettere se ha ancora ore e i suoi docenti possono adesso.
static int scelte_della_classe(const Ricerca *r, const int giorno, const int ora, const int k, int *scelte) {
    const Scuola *s = r->s;
    const int dopo = s->giorni - 1 - giorno;         // quanti giorni ci sono dopo oggi
    const int obbligatoria = ora < s->obbligatorie;  // quest'ora ha lezione per forza
    const int chiusa = r->chiusa[giorno * s->classi + k];
    // si puo' mettere una lezione? (le ore che restano devono bastare a coprire le prime ore dei giorni dopo)
    const int lezione = obbligatoria || (!chiusa && r->resto_classe[k] > dopo * s->obbligatorie);
    // si puo' mettere l'ora buca? (le ore che restano devono starci tutte nei giorni dopo)
    const int buca = !obbligatoria && (chiusa || r->resto_classe[k] <= dopo * s->ore);
    int n = 0;
    for (int v = s->prima_voce[k]; lezione && v < s->prima_voce[k + 1]; v++) {
        const Voce *voce = &s->voce[v];
        if (r->resto_voce[v] && puo_adesso(r, giorno, ora, k, voce->docente[0])
            && (voce->docente[1] < 0 || puo_adesso(r, giorno, ora, k, voce->docente[1])))
            scelte[n++] = v;
    }
    if (buca) scelte[n++] = BUCA;
    return n;
}

// Quanto presto si prova una scelta: un numero, piu' basso prima.
// - Prima le voci con il docente che ha meno margine: e' quello che rischia di restare con delle ore.
// - A pari margine, prima chi oggi in questa classe ha gia' insegnato: sta continuando le sue ore di seguito, e se si
//   interrompe oggi qui non puo' piu' tornare.
// - A pari anche questo, prima la voce a cui restano meno ore: si chiudono le voci quasi finite, invece di lasciare
//   un'ora qua e una la' per la fine della settimana.
// L'ora buca viene dopo tutte le lezioni.
// I tre criteri stanno in un numero solo: il margine pesa di piu', poi chi continua, poi le ore rimaste (che sono
// sempre meno di 1000).
static long long quando_provarla(const Ricerca *r, const int giorno, const int ora, const int k, const int v) {
    if (v == BUCA) return 1LL << 60;
    const Scuola *s = r->s;
    const int *margine = &r->margine[(giorno * s->ore + ora) * s->docenti];  // i margini di quest'ora, per docente
    int meno_margine = 1 << 20, continua = 0;
    for (int i = 0; i < docenti_della_voce(&s->voce[v]); i++) {
        const int t = s->voce[v].docente[i];
        meno_margine = MIN(meno_margine, margine[t]);  // di una coppia conta chi ne ha meno
        continua |= r->ore_oggi[(giorno * s->classi + k) * s->docenti + t] > 0;
    }
    return (meno_margine * 2LL + !continua) * 1000 + r->resto_voce[v];
}

// Mette le scelte della classe k nell'ordine in cui si provano. Sono poche: si ordinano per inserimento, ognuna
// scivola indietro finche' trova una che viene prima di lei. A parita' resta l'ordine delle voci, sempre lo stesso.
static void ordina_le_scelte(const Ricerca *r, const int giorno, const int ora, const int k, int *scelte, const int n) {
    for (int i = 1; i < n; i++) {
        const int v = scelte[i];
        const long long sua = quando_provarla(r, giorno, ora, k, v);
        int j = i - 1;
        for (; j >= 0 && quando_provarla(r, giorno, ora, k, scelte[j]) > sua; j--) scelte[j + 1] = scelte[j];
        scelte[j + 1] = v;
    }
}

// Sceglie la classe da decidere in quest'ora: quella, tra le non decise, che ha meno scelte. Le sue scelte, gia' in
// ordine, vanno in `scelte`; ritorna quante sono, o -1 se da qui non si va avanti (una classe non ha scelte, oppure
// un docente che in quest'ora deve insegnare non compare tra le scelte di nessuna classe), con il perche' in *m.
//
// Perche' la classe con meno scelte: una classe senza scelte ferma tutto, e conviene scoprirlo subito; una con una
// scelta sola la deve prendere per forza, e conviene dargliela prima che un'altra classe le porti via il docente.
static int scegli_la_classe(Ricerca *r, const int giorno, const int ora, int *classe, int *scelte, Motivo *m) {
    const Scuola *s = r->s;
    const int ora_della_settimana = giorno * s->ore + ora;
    r->timbro++;               // un timbro nuovo per questa volta (vedi sotto: r->visto)
    int prova[r->max_scelte];  // le scelte della classe che si sta guardando
    int meno = -1;             // quante ne ha la classe con meno scelte vista finora (-1: nessuna vista)
    for (int k = 0; k < s->classi; k++) {
        if (r->orario[cella_di(r, giorno, ora, k)] != VUOTA) continue;  // in quest'ora e' gia' decisa
        const int n = scelte_della_classe(r, giorno, ora, k, prova);
        if (!n) {
            *m = (Motivo){CLASSE_SENZA_SCELTE, -1, k, ora, 0, 0};
            return -1;
        }
        // I docenti che compaiono tra le scelte di qualcuno: accanto a ognuno si scrive il timbro di questa volta.
        // (Cosi' non c'e' da azzerare niente ogni volta: un timbro vecchio vuol dire «questa volta non visto».)
        for (int i = 0; i < n; i++)
            for (int j = 0; prova[i] >= 0 && j < docenti_della_voce(&s->voce[prova[i]]); j++)
                r->visto[s->voce[prova[i]].docente[j]] = r->timbro;
        if (meno >= 0 && n >= meno) continue;  // ne ha quante un'altra o di piu': resta la prima trovata
        meno = n;
        *classe = k;
        for (int i = 0; i < n; i++) scelte[i] = prova[i];
    }
    // Un docente che in quest'ora deve insegnare (limiti_a_inizio_ora), non e' ancora in nessuna classe, e nessuna
    // classe non decisa lo ha tra le sue scelte: quest'ora la perde, e le sue ore non ci stanno piu'.
    for (int t = 0; t < s->docenti; t++) {
        const int qui = ora_della_settimana * s->docenti + t;
        if (r->deve[qui] && !r->occupato[qui] && r->visto[t] != r->timbro) {
            *m = (Motivo){DOCENTE_SENZA_POSTO, t, -1, ora, r->resto_docente[t], ore_libere_da(r, giorno, ora + 1, t)};
            return -1;
        }
    }
    ordina_le_scelte(r, giorno, ora, *classe, scelte, meno);
    return meno;
}

// ---- 4. la ricerca ----

static void nodo(Ricerca *r, int profondita, int cella, int voce);

// Le scelte di un nodo (`profondita` celle decise, l'ultima e' `cella`): guarda i limiti e sceglie la classe da
// decidere. Le scelte, in ordine, vanno in `scelte` e la classe in *classe; ritorna quante sono, o -1 se da qui non si
// va avanti (con il perche' in *m) o se l'orario e' gia' pieno.
static int scelte_del_nodo(Ricerca *r, const int profondita, const int cella, int *classe, int *scelte, Motivo *m) {
    const Scuola *s = r->s;
    if (profondita == r->celle) return -1;
    // L'ora che si sta decidendo. Le celle si decidono un'ora alla volta, quindi con `profondita` celle decise le
    // ore gia' piene sono profondita / classi, e quella e' la prossima (quali classi di quell'ora siano gia' decise
    // lo dice l'orario).
    const int ora_della_settimana = profondita / s->classi, giorno = ora_della_settimana / s->ore;
    const int ora = ora_della_settimana % s->ore;
    // I conti, nell'ordine: quelli che l'ultima scelta ha cambiato (alla radice non c'e' scelta); poi, se si sta
    // cominciando un'ora nuova (nessuna classe decisa: profondita multipla delle classi), quelli di inizio ora; e
    // solo se tornano tutti si sceglie la classe.
    if ((cella < 0 || limiti_dopo_la_scelta(r, cella, m))
        && (profondita % s->classi || limiti_a_inizio_ora(r, giorno, ora, m)))
        return scegli_la_classe(r, giorno, ora, classe, scelte, m);
    return -1;
}

// Prova le scelte di un nodo dalla numero `da` in poi, scendendo in ognuna. Quando la ricerca e' finita (orario
// trovato, o limite di nodi) si esce senza togliere niente: l'orario resta com'e'.
static void prova_le_scelte(Ricerca *r, const int profondita, const int classe, const int *scelte, const int da,
                            const int n_scelte) {
    // la cella che questo nodo decide: la classe scelta, nell'ora che si sta decidendo
    const int qui = profondita / r->s->classi * r->s->classi + classe;
    for (int i = da; i < n_scelte; i++) {
        segna(r, qui, scelte[i], 1);              // si mette la scelta...
        nodo(r, profondita + 1, qui, scelte[i]);  // ...si scende...
        if (r->esito != IN_CORSO) return;         // (finita: si esce da tutti i nodi lasciando l'orario com'e')
        segna(r, qui, scelte[i], -1);             // ...e, se di li' non e' uscito niente, la si toglie
    }
    // finite le scelte senza trovare niente: si torna al nodo sopra, che prova la sua scelta dopo
}

// Un nodo: `profondita` celle decise, l'ultima e' `cella` con dentro `voce`. Lo conta, ne trova le scelte, avvisa chi
// segue la ricerca, e prova le scelte una per una.
static void nodo(Ricerca *r, const int profondita, const int cella, const int voce) {
    if (r->limite && r->nodi >= r->limite) {
        r->esito = LIMITE_DI_NODI;
        return;
    }
    r->nodi++;
    r->profondita_massima = MAX(r->profondita_massima, profondita);
    Motivo m = {NESSUNO, -1, -1, -1, 0, 0};
    int scelte[r->max_scelte];
    int classe = -1;  // la classe che questo nodo decide
    if (profondita == r->celle) r->esito = TROVATO;  // tutte le celle decise: l'orario c'e'
    // quante scelte ha questo nodo; -1: da qui non si va avanti (o l'orario e' finito)
    const int n_scelte = scelte_del_nodo(r, profondita, cella, &classe, scelte, &m);
    // un conto non torna: e' un ritorno indietro, contato in tutto e per la cella dell'ultima scelta
    if (m.tipo != NESSUNO) {
        r->ritorni++;
        if (cella >= 0) r->ritorni_in[cella]++;
    }
    // chi segue la ricerca (i passi su file, lo stato) viene avvisato a ogni nodo, anche quando non si va avanti
    if (r->a_ogni_nodo) r->a_ogni_nodo(r, profondita, cella, voce, &m);
    if (!profondita) r->motivo_radice = m;  // alla radice: se non si va avanti, un orario non c'e' in partenza
    prova_le_scelte(r, profondita, classe, scelte, 0, n_scelte);
}

void cerca_orario(Ricerca *r) {
    r->esito = IN_CORSO;
    nodo(r, 0, -1, VUOTA);
    if (r->esito == IN_CORSO) r->esito = IMPOSSIBILE;  // arrivata in fondo senza trovare niente: un orario non c'e'
}

// ---- 5. riprendere una ricerca fermata ----

// Ripercorre la strada dell'ultimo nodo visitato senza contare niente: i nodi lungo la strada sono gia' stati
// visitati. A ogni profondita' si ritrovano le scelte del nodo (la ricerca e' sempre la stessa: sono quelle di
// allora, nello stesso ordine) e tra quelle la scelta fatta; si scende; e quando da li' si torna si continua con le
// scelte dopo, come avrebbe fatto la ricerca. L'ultimo nodo della strada era stato visitato ma non le sue scelte:
// si provano tutte. Ritorna 0 se la strada non e' una strada di questa ricerca.
static int ripercorri(Ricerca *r, const int profondita, const int *strada, const int fine) {
    Motivo m = {NESSUNO, -1, -1, -1, 0, 0};
    int scelte[r->max_scelte];
    int classe = -1, da = 0;
    const int cella = profondita ? strada[2 * (profondita - 1)] : -1;
    const int n_scelte = scelte_del_nodo(r, profondita, cella, &classe, scelte, &m);
    if (profondita < fine) {
        // la scelta fatta allora: deve essere una delle scelte di questo nodo, nella cella che questo nodo decide
        const int qui = profondita / r->s->classi * r->s->classi + classe;
        int i = 0;
        while (i < n_scelte && scelte[i] != strada[2 * profondita + 1]) i++;
        if (i >= n_scelte || qui != strada[2 * profondita]) return 0;
        segna(r, qui, scelte[i], 1);
        if (!ripercorri(r, profondita + 1, strada, fine)) return 0;
        if (r->esito != IN_CORSO) return 1;
        segna(r, qui, scelte[i], -1);
        da = i + 1;
    }
    prova_le_scelte(r, profondita, classe, scelte, da, n_scelte);
    return 1;
}

int riprendi_la_ricerca(Ricerca *r, const int *strada, const int profondita) {
    r->esito = IN_CORSO;
    if (!ripercorri(r, 0, strada, profondita)) return 0;
    if (r->esito == IN_CORSO) r->esito = IMPOSSIBILE;
    return 1;
}
