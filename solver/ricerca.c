#include "ricerca.h"

#include <stdlib.h>

// ---- creare la ricerca ----

Ricerca *nuova_ricerca(const Scuola *s) {
    const int C = s->classi, T = s->docenti, G = s->giorni, O = s->ore;
    Ricerca *r = calloc(1, sizeof(Ricerca));
    if (!r) return NULL;
    r->s = s;
    r->celle = G * O * C;
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
    if (!r->orario || !r->resto_voce || !r->resto_classe || !r->resto_docente || !r->resto_in_classe || !r->occupato
        || !r->ore_oggi || !r->classi_oggi || !r->chiusa || !r->docente_di || !r->n_docenti_di || !r->classe_di
        || !r->n_classi_di || !r->deve || !r->margine || !r->visto || !r->posto || !r->ricordo) {
        libera_ricerca(r);
        return NULL;
    }
    for (int i = 0; i < r->celle; i++) r->orario[i] = VUOTA;
    for (int v = 0; v < s->voci; v++) {
        const Voce *voce = &s->voce[v];
        const int k = voce->classe;
        r->resto_voce[v] = voce->ore;
        r->resto_classe[k] += voce->ore;
        r->posto[2 * v + 1] = -1;
        for (int i = 0; i < docenti_della_voce(voce); i++) {
            const int t = voce->docente[i];
            r->resto_docente[t] += voce->ore;
            if (!r->resto_in_classe[k * T + t]) {  // la prima voce del docente in questa classe
                r->docente_di[k * T + r->n_docenti_di[k]++] = t;
                r->classe_di[t * C + r->n_classi_di[t]++] = k;
            }
            r->resto_in_classe[k * T + t] += voce->ore;
            for (int j = 0; j < r->n_docenti_di[k]; j++)
                if (r->docente_di[k * T + j] == t) r->posto[2 * v + i] = j;
        }
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
    free(r);
}

// ---- mettere e togliere una scelta ----

// Mette nella cella la voce v o l'ora buca (verso = 1), oppure la toglie (verso = -1), e tiene i conti.
static void segna(Ricerca *r, const int cella, const int v, const int verso) {
    const Scuola *s = r->s;
    const int k = cella % s->classi, ora_della_settimana = cella / s->classi, giorno = ora_della_settimana / s->ore;
    r->orario[cella] = verso > 0 ? v : VUOTA;
    if (v == BUCA) {
        r->chiusa[giorno * s->classi + k] += verso;
        return;
    }
    r->resto_voce[v] -= verso;
    r->resto_classe[k] -= verso;
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

// ---- le scelte di una classe in un'ora ----

// il docente t puo' insegnare adesso nella classe k: e' entrato, e' libero, e rispetta le regole della giornata
static int puo_adesso(const Ricerca *r, const int giorno, const int ora, const int k, const int t) {
    const Scuola *s = r->s;
    if (s->ingresso[t] > ora || r->occupato[(giorno * s->ore + ora) * s->docenti + t]) return 0;
    const int fatte = r->ore_oggi[(giorno * s->classi + k) * s->docenti + t];
    if (!fatte) return r->classi_oggi[giorno * s->docenti + t] < MAX_CLASSI_AL_GIORNO;
    return fatte < MAX_ORE_DI_SEGUITO && presente(r, giorno, ora - 1, k, t);
}

// Le scelte della classe k in quest'ora, in `scelte`; ritorna quante sono.
// Nelle prime ore, obbligatorie, solo lezioni. Dopo: se oggi ha gia' avuto un'ora buca, solo l'ora buca; se no la
// lezione, purche' dopo le restino abbastanza ore per le prime ore dei giorni dopo, e l'ora buca, purche' le ore che
// le restano stiano nei giorni dopo. Una voce si puo' mettere se ha ancora ore e i suoi docenti possono adesso.
static int scelte_della_classe(const Ricerca *r, const int giorno, const int ora, const int k, int *scelte) {
    const Scuola *s = r->s;
    const int dopo = s->giorni - 1 - giorno, obbligatoria = ora < s->obbligatorie;
    const int chiusa = r->chiusa[giorno * s->classi + k];
    const int lezione = obbligatoria || (!chiusa && r->resto_classe[k] > dopo * s->obbligatorie);
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
static long long quando_provarla(const Ricerca *r, const int giorno, const int ora, const int k, const int v) {
    if (v == BUCA) return 1LL << 60;
    const Scuola *s = r->s;
    const int *margine = &r->margine[(giorno * s->ore + ora) * s->docenti];
    int meno_margine = 1 << 20, continua = 0;
    for (int i = 0; i < docenti_della_voce(&s->voce[v]); i++) {
        const int t = s->voce[v].docente[i];
        meno_margine = MIN(meno_margine, margine[t]);
        continua |= r->ore_oggi[(giorno * s->classi + k) * s->docenti + t] > 0;
    }
    return (meno_margine * 2LL + !continua) * 1000 + r->resto_voce[v];
}

// mette le scelte della classe k nell'ordine in cui si provano
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
static int scegli_la_classe(Ricerca *r, const int giorno, const int ora, int *classe, int *scelte, Motivo *m) {
    const Scuola *s = r->s;
    const int ora_della_settimana = giorno * s->ore + ora;
    int prova[r->max_scelte], meno = -1;
    for (int k = 0; k < s->classi; k++) {
        if (r->orario[cella_di(r, giorno, ora, k)] != VUOTA) continue;
        const int n = scelte_della_classe(r, giorno, ora, k, prova);
        if (!n) {
            *m = (Motivo){CLASSE_SENZA_SCELTE, -1, k, ora, 0, 0};
            return -1;
        }
        for (int i = 0; i < n; i++)  // i docenti che compaiono tra le scelte di qualcuno
            for (int j = 0; prova[i] >= 0 && j < docenti_della_voce(&s->voce[prova[i]]); j++)
                r->visto[s->voce[prova[i]].docente[j]] = r->nodi;
        if (meno >= 0 && n >= meno) continue;
        meno = n;
        *classe = k;
        for (int i = 0; i < n; i++) scelte[i] = prova[i];
    }
    for (int t = 0; t < s->docenti; t++) {
        const int qui = ora_della_settimana * s->docenti + t;
        if (r->deve[qui] && !r->occupato[qui] && r->visto[t] != r->nodi) {
            *m = (Motivo){DOCENTE_SENZA_POSTO, t, -1, ora, r->resto_docente[t], ore_libere_da(r, giorno, ora + 1, t)};
            return -1;
        }
    }
    ordina_le_scelte(r, giorno, ora, *classe, scelte, meno);
    return meno;
}

// ---- la ricerca ----

// Un nodo: `profondita` celle decise, l'ultima e' `cella` con dentro `voce`. Guarda i limiti, sceglie la classe da
// decidere e ne prova le scelte una per una, scendendo. Quando la ricerca e' finita (orario trovato, o limite di
// nodi) si esce senza togliere niente: l'orario resta com'e'.
static void nodo(Ricerca *r, const int profondita, const int cella, const int voce) {
    const Scuola *s = r->s;
    if (r->limite && r->nodi >= r->limite) {
        r->esito = LIMITE_DI_NODI;
        return;
    }
    r->nodi++;
    r->profondita_massima = MAX(r->profondita_massima, profondita);
    const int ora_della_settimana = profondita / s->classi, giorno = ora_della_settimana / s->ore;
    const int ora = ora_della_settimana % s->ore;
    Motivo m = {NESSUNO, -1, -1, -1, 0, 0};
    int scelte[r->max_scelte], n_scelte = -1, classe = -1;
    if (profondita == r->celle) r->esito = TROVATO;
    else if ((cella < 0 || limiti_dopo_la_scelta(r, cella, &m))
             && (profondita % s->classi || limiti_a_inizio_ora(r, giorno, ora, &m)))
        n_scelte = scegli_la_classe(r, giorno, ora, &classe, scelte, &m);
    if (r->a_ogni_nodo) r->a_ogni_nodo(r, profondita, cella, voce, &m);
    if (!profondita) r->motivo_radice = m;
    for (int i = 0; i < n_scelte; i++) {
        const int qui = cella_di(r, giorno, ora, classe);
        segna(r, qui, scelte[i], 1);
        nodo(r, profondita + 1, qui, scelte[i]);
        if (r->esito != IN_CORSO) return;
        segna(r, qui, scelte[i], -1);
    }
}

void cerca_orario(Ricerca *r) {
    r->esito = IN_CORSO;
    nodo(r, 0, -1, VUOTA);
    if (r->esito == IN_CORSO) r->esito = IMPOSSIBILE;  // arrivata in fondo senza trovare niente: un orario non c'e'
}
