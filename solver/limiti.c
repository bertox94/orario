#include "ricerca.h"

// I limiti: conti che devono tornare perche' da un nodo possa ancora uscire un orario. Sono condizioni necessarie:
// scartano solo rami in cui un orario non c'e', quindi non fanno perdere nessun orario.
//
// Ogni conto ha la stessa forma: da una parte quello che SERVE (le ore che restano da mettere), dall'altra quello
// che e' ancora POSSIBILE (i posti dove quelle ore potrebbero andare, contati con ottimismo: senza guardare gli
// incastri con gli altri). Se serve piu' di quanto e' possibile, da qui un orario non esce, e la ricerca torna
// indietro. I due numeri finiscono nel Motivo, e il sito li mostra.
//
//   ORE_DOCENTE             un docente, in tutta la scuola       limiti_a_inizio_ora
//   ORE_DOCENTE_IN_CLASSE   un docente in una classe             docente_in_classe
//   ORE_CLASSE              una classe: le celle che le restano  classe
//   PRIME_ORE               una classe: le prime ore del giorno  classe
//   ORARIO_DELLA_CLASSE     una classe, guardata da sola         classe_sola.c
// (gli altri due, DOCENTE_SENZA_POSTO e CLASSE_SENZA_SCELTE, li trova la ricerca mentre sceglie: ricerca.c)
//
// In ogni conto «oggi» e' il giorno dell'ora che si sta decidendo, e per ogni classe la prima ora non ancora decisa
// e' quell'ora, oppure la successiva se in quell'ora la classe e' gia' stata decisa (prima_ora_libera).
//
// In fondo al file, i SEGNI: le celle di cui parla un conto che non torna, per colorarle nell'orario del sito.

// Il conto non torna: scrive il perche' in *m e ritorna 0, cosi' chi chiama puo' scrivere «return non_torna(...)».
static int non_torna(Motivo *m, const TipoMotivo tipo, const int docente, const int classe, const int ora,
                     const int servono, const int possibili) {
    *m = (Motivo){tipo, docente, classe, ora, servono, possibili};
    return 0;
}

// La prima ora di oggi che la classe k deve ancora decidere: l'ora che si sta decidendo, o quella dopo se la classe
// in quest'ora ha gia' la sua scelta (le classi di un'ora si decidono una alla volta).
static int prima_ora_libera(const Ricerca *r, const int giorno, const int ora, const int k) {
    return ora + (r->orario[cella_di(r, giorno, ora, k)] != VUOTA);
}

// Quante ore il docente t puo' ancora fare oggi nella classe k tra l'ora `da` e l'ora `fino_a`, comprese.
// (la descrizione completa e' in ricerca.h)
int ancora_oggi(const Ricerca *r, const int giorno, const int da, const int fino_a, const int k, const int t) {
    const Scuola *s = r->s;
    // la giornata della classe e' finita: non ha piu' ore, o ha gia' avuto un'ora buca
    if (da >= s->ore || r->chiusa[giorno * s->classi + k]) return 0;
    const int fatte = r->ore_oggi[(giorno * s->classi + k) * s->docenti + t];  // le ore gia' fatte oggi qui
    const int altrove = r->occupato[(giorno * s->ore + da) * s->docenti + t];  // nell'ora `da` e' in un'altra classe
    int comincia = da;                           // la prima ora in cui potrebbe insegnare qui
    const int massimo = MAX_ORE_DI_SEGUITO - fatte;  // quante ne puo' ancora fare oggi qui, al massimo
    if (fatte) {
        // Oggi qui ha gia' insegnato: le sue ore devono essere di seguito, quindi puo' solo continuare subito. Se
        // nell'ora prima non c'era, o adesso e' in un'altra classe, il filo e' rotto e oggi qui non torna.
        if (altrove || !presente(r, giorno, da - 1, k, t)) return 0;
    } else {
        // Oggi qui non ha ancora insegnato: sarebbe una classe in piu' nella sua giornata
        if (r->classi_oggi[giorno * s->docenti + t] >= MAX_CLASSI_AL_GIORNO) return 0;
        comincia = MAX(da, s->ingresso[t]);        // non prima del suo ingresso
        if (comincia == da && altrove) comincia++;  // e se adesso e' altrove, dall'ora dopo
    }
    // le ore tra `comincia` e `fino_a`, ma non piu' del massimo (e mai meno di zero)
    return MAX(0, MIN(massimo, fino_a - comincia + 1));
}

// Le ore in cui il docente t e' a scuola dall'ora `da` di quel giorno alla fine della settimana: nei giorni dopo
// tutte quelle dal suo ingresso, oggi quelle dal suo ingresso ma non prima di `da`.
int ore_libere_da(const Ricerca *r, const int giorno, const int da, const int t) {
    const Scuola *s = r->s;
    const int dopo = s->giorni - 1 - giorno;  // quanti giorni ci sono dopo oggi
    return dopo * (s->ore - s->ingresso[t]) + MAX(0, s->ore - MAX(da, s->ingresso[t]));
}

// ORE_DOCENTE_IN_CLASSE. Un docente in una classe: le ore che gli restano li' devono stare in quelle che ci puo'
// ancora fare.
//   servono    le ore che gli restano nella classe (tutte le sue voci li', da solo e in coppia)
//   possibili  oggi quelle di ancora_oggi; in ognuno dei giorni dopo il massimo di ore di seguito, o meno se entra
//              cosi' tardi che nella giornata non ci stanno
static int docente_in_classe(const Ricerca *r, const int giorno, const int ora, const int k, const int t, Motivo *m) {
    const Scuola *s = r->s;
    const int restano = r->resto_in_classe[k * s->docenti + t];
    if (!restano) return 1;  // qui ha finito: niente da contare
    const int dopo = s->giorni - 1 - giorno;                                       // i giorni dopo oggi
    const int al_giorno = MIN(MAX_ORE_DI_SEGUITO, s->ore - s->ingresso[t]);        // in un giorno intero
    const int da = prima_ora_libera(r, giorno, ora, k);
    const int possibili = dopo * al_giorno + ancora_oggi(r, giorno, da, s->ore - 1, k, t);
    return restano <= possibili ? 1 : non_torna(m, ORE_DOCENTE_IN_CLASSE, t, k, -1, restano, possibili);
}

// Una classe: due conti.
//
// ORE_CLASSE. Le ore che le restano devono stare nelle celle che le restano.
//   servono    le ore che restano alla classe
//   possibili  le celle di oggi dalla sua prima ora libera (nessuna se oggi ha gia' avuto un'ora buca: dopo un'ora
//              buca la giornata e' finita) piu' tutte quelle dei giorni dopo
//
// PRIME_ORE. Le prime ore di ogni giorno hanno sempre lezione. Per ogni ora obbligatoria h si guarda la fascia dalla
// prima ora all'ora h: tutte le celle della fascia vanno riempite, e a riempirle possono essere solo le lezioni che
// le voci della classe riescono a fare dentro la fascia.
//   servono    le celle della fascia ancora da riempire: oggi dalla prima ora libera ad h, nei giorni dopo h + 1
//   possibili  le lezioni che le voci possono fare nella fascia. Una voce: oggi quanto possono i suoi docenti (per
//              una coppia, chi dei due puo' meno), in ognuno dei giorni dopo le ore tra l'ingresso e h (per una
//              coppia, di chi entra piu' tardi) ma non piu' del massimo di ore di seguito; in tutto, non piu' delle
//              ore che le restano.
// E' il conto che si accorge dei docenti che entrano tardi: alle prime ore non ci sono, e se la classe ha quasi solo
// loro le prime ore restano scoperte. Si guarda ogni h, e non solo l'ultima, perche' la fascia stretta (solo la
// prima ora, per esempio) puo' non tornare anche quando quella larga torna.
// Le lezioni che la voce v puo' fare nella fascia fino all'ora h (compresa), con la prima ora libera della sua classe
// in `da`: oggi (*oggi) piu' quelle di ognuno dei giorni dopo (*al_giorno), ma non piu' delle ore che le restano.
static int lezioni_nella_fascia(const Ricerca *r, const int giorno, const int da, const int h, const int v, int *oggi,
                                int *al_giorno) {
    const Scuola *s = r->s;
    const int dopo = s->giorni - 1 - giorno, k = s->voce[v].classe;
    int entra = 0;
    *oggi = s->ore;
    for (int i = 0; i < docenti_della_voce(&s->voce[v]); i++) {
        const int t = s->voce[v].docente[i];
        *oggi = MIN(*oggi, ancora_oggi(r, giorno, da, h, k, t));  // oggi: chi dei due puo' meno
        entra = MAX(entra, s->ingresso[t]);                       // nei giorni dopo: chi entra dopo
    }
    *al_giorno = MIN(MAX_ORE_DI_SEGUITO, MAX(0, h + 1 - entra));
    return MIN(r->resto_voce[v], *oggi + dopo * *al_giorno);
}

static int classe(const Ricerca *r, const int giorno, const int ora, const int k, Motivo *m) {
    const Scuola *s = r->s;
    const int dopo = s->giorni - 1 - giorno, da = prima_ora_libera(r, giorno, ora, k);
    // ORE_CLASSE
    const int oggi = r->chiusa[giorno * s->classi + k] ? 0 : s->ore - da;  // le celle di oggi che contano ancora
    if (r->resto_classe[k] > dopo * s->ore + oggi)
        return non_torna(m, ORE_CLASSE, -1, k, -1, r->resto_classe[k], dopo * s->ore + oggi);
    // PRIME_ORE, una fascia alla volta
    for (int h = 0; h < s->obbligatorie; h++) {
        const int celle = dopo * (h + 1) + MAX(0, h - da + 1);  // da riempire: nei giorni dopo, e oggi
        int lezioni = 0;                                         // quante se ne possono fare
        // (ci si ferma appena le lezioni bastano: non serve contarle tutte)
        for (int v = s->prima_voce[k]; v < s->prima_voce[k + 1] && lezioni < celle; v++) {
            if (!r->resto_voce[v]) continue;  // voce finita
            int oggi_voce, al_giorno;
            lezioni += lezioni_nella_fascia(r, giorno, da, h, v, &oggi_voce, &al_giorno);
        }
        if (lezioni < celle) return non_torna(m, PRIME_ORE, -1, k, h, celle, lezioni);
    }
    return 1;
}

// I limiti all'inizio di un'ora, quando nessuna classe di quell'ora e' ancora decisa: si guardano tutti i docenti e
// tutte le classi. Prepara anche, per i nodi di quest'ora, chi deve insegnare per forza adesso (r->deve) e il
// margine di ognuno (r->margine), che servono alla ricerca per scegliere (ricerca.c).
int limiti_a_inizio_ora(Ricerca *r, const int giorno, const int ora, Motivo *m) {
    const Scuola *s = r->s;
    // ORE_DOCENTE. Un docente in tutto: le ore che gli restano devono stare nelle ore in cui e' ancora a scuola.
    //   servono    le ore che gli restano, in tutte le sue classi
    //   possibili  le ore da adesso alla fine della settimana in cui e' a scuola (ore_libere_da)
    for (int t = 0; t < s->docenti; t++) {
        const int restano = r->resto_docente[t], libere = ore_libere_da(r, giorno, ora, t);
        if (restano > libere) return non_torna(m, ORE_DOCENTE, t, -1, -1, restano, libere);
        const int qui = (giorno * s->ore + ora) * s->docenti + t;
        // se le sue ore ci stanno solo contando anche quest'ora, in quest'ora deve insegnare
        r->deve[qui] = restano > ore_libere_da(r, giorno, ora + 1, t);
        // il margine: quante ore libere ha in piu' di quelle che deve fare (0: non ne puo' saltare nessuna)
        r->margine[qui] = libere - restano;
    }
    for (int k = 0; k < s->classi; k++) {
        if (!classe(r, giorno, ora, k, m)) return 0;
        for (int i = 0; i < r->n_docenti_di[k]; i++)
            if (!docente_in_classe(r, giorno, ora, k, r->docente_di[k * s->docenti + i], m)) return 0;
        // l'orario della classe da sola cambia solo quando si decide una sua cella: qui serve solo in partenza,
        // per accorgersi subito di una classe che un orario non ce l'ha proprio
        if (!giorno && !ora && !la_classe_ha_un_orario(r, giorno, ora, k))
            return non_torna(m, ORARIO_DELLA_CLASSE, -1, k, -1, r->resto_classe[k], 0);
    }
    return 1;
}

// I limiti dopo una scelta: si riguarda solo quello che la scelta ha cambiato, cioe' la classe della cella e i
// docenti che ci sono stati messi. Tutto il resto e' come all'inizio dell'ora, quando i conti tornavano.
int limiti_dopo_la_scelta(const Ricerca *r, const int cella, Motivo *m) {
    const Scuola *s = r->s;
    const int k = cella % s->classi, ora = cella / s->classi % s->ore, giorno = cella / s->classi / s->ore;
    // la classe appena decisa: la sua prima ora libera e' avanzata, per lei e per tutti i suoi docenti (anche quelli
    // che non sono stati messi: per loro quest'ora, qui, e' persa)
    if (!classe(r, giorno, ora, k, m)) return 0;
    for (int i = 0; i < r->n_docenti_di[k]; i++)
        if (!docente_in_classe(r, giorno, ora, k, r->docente_di[k * s->docenti + i], m)) return 0;
    if (!la_classe_ha_un_orario(r, giorno, ora, k))
        return non_torna(m, ORARIO_DELLA_CLASSE, -1, k, -1, r->resto_classe[k], 0);
    // i docenti messi: adesso sono occupati, e oggi hanno una classe in piu'. Cambia quello che possono fare oggi
    // nelle altre loro classi.
    const int v = r->orario[cella];
    for (int i = 0; v >= 0 && i < docenti_della_voce(&s->voce[v]); i++) {
        const int t = s->voce[v].docente[i];
        for (int j = 0; j < r->n_classi_di[t]; j++) {
            const int altra = r->classe_di[t * s->classi + j];
            if (altra != k && !docente_in_classe(r, giorno, ora, altra, t, m)) return 0;
        }
    }
    return 1;
}

// ---- i segni: le celle di cui parla un conto che non torna ----
//
// Solo per il sito (il tasto «Mostra nell'orario»): la ricerca non li usa. Stanno qui, accanto ai conti, perche'
// devono dire la stessa cosa: le ore di oggi si segnano chiedendole alla stessa ancora_oggi che le conta.

// Segna la riga della classe k per il docente t (o per nessuno, t < 0: allora contano solo le celle).
// - FATTA: dove il docente gia' insegna in questa classe, in tutta la settimana;
// - oggi, dalla prima ora libera: PUO le ore che ancora_oggi conta, NO le altre (per t < 0: PUO tutte, o NO tutte se
//   la classe ha gia' avuto un'ora buca);
// - nei giorni dopo: PUO dal suo ingresso, NO prima.
static void segna_la_riga(const Ricerca *r, const int giorno, const int ora, const int k, const int t,
                          signed char *segni) {
    const Scuola *s = r->s;
    const int da = prima_ora_libera(r, giorno, ora, k);
    for (int g = 0; g < s->giorni; g++)
        for (int h = 0; h < s->ore; h++) {
            signed char *segno = &segni[cella_di(r, g, h, k)];
            if (t >= 0 && presente(r, g, h, k, t)) *segno = SEGNO_FATTA;
            else if (g < giorno || (g == giorno && h < da)) continue;  // gia' decisa: non conta
            else if (g > giorno) *segno = t < 0 || h >= s->ingresso[t] ? SEGNO_PUO : SEGNO_NO;
            else if (t < 0) *segno = r->chiusa[giorno * s->classi + k] ? SEGNO_NO : SEGNO_PUO;
            // oggi quest'ora conta se, arrivando fino a lei, ancora_oggi ne conta una in piu'
            else *segno = ancora_oggi(r, giorno, da, h, k, t) > ancora_oggi(r, giorno, da, h - 1, k, t)
                          ? SEGNO_PUO : SEGNO_NO;
        }
}

// Le celle di cui parla il motivo, con i conti fatti all'ora (giorno, ora): in `segni`, per ogni cella, 0 o un SEGNO_.
static void segni_del_limite(const Ricerca *r, const int giorno, const int ora, const Motivo *m, signed char *segni) {
    const Scuola *s = r->s;
    const int t = m->docente, k = m->classe;
    switch (m->tipo) {
        case ORE_DOCENTE:
            // le ore in cui e' ancora a scuola, segnate in tutte le sue classi (ogni ora conta una volta sola), e
            // dove insegna gia'
            for (int j = 0; j < r->n_classi_di[t]; j++) {
                const int sua = r->classe_di[t * s->classi + j];
                for (int g = 0; g < s->giorni; g++)
                    for (int h = 0; h < s->ore; h++) {
                        signed char *segno = &segni[cella_di(r, g, h, sua)];
                        if (presente(r, g, h, sua, t)) *segno = SEGNO_FATTA;
                        else if (g > giorno || (g == giorno && h >= ora))
                            *segno = h >= s->ingresso[t] ? SEGNO_PUO : SEGNO_NO;
                    }
            }
            break;
        case ORE_DOCENTE_IN_CLASSE:
            segna_la_riga(r, giorno, ora, k, t, segni);
            break;
        case ORE_CLASSE:
        case ORARIO_DELLA_CLASSE:
            segna_la_riga(r, giorno, ora, k, -1, segni);  // le celle che restano alla classe
            break;
        case PRIME_ORE:
            // la fascia da riempire: fino all'ora m->ora, oggi dalla prima ora libera e nei giorni dopo
            for (int g = giorno; g < s->giorni; g++)
                for (int h = g == giorno ? prima_ora_libera(r, giorno, ora, k) : 0; h <= m->ora; h++)
                    segni[cella_di(r, g, h, k)] = SEGNO_SERVE;
            break;
        case DOCENTE_SENZA_POSTO:
            // le sue classi, in quest'ora: nessuna lo puo' prendere
            for (int j = 0; j < r->n_classi_di[t]; j++)
                segni[cella_di(r, giorno, ora, r->classe_di[t * s->classi + j])] = SEGNO_NO;
            break;
        case CLASSE_SENZA_SCELTE:
            segni[cella_di(r, giorno, ora, k)] = SEGNO_NO;
            break;
        default:
            break;
    }
}

int segni_del_nodo(const Ricerca *r, const int profondita, const int cella, const Motivo *m, signed char *segni) {
    const Scuola *s = r->s;
    for (int i = 0; i < r->celle; i++) segni[i] = 0;
    // A che ora e' stato fatto il conto. Di solito e' l'ora che si sta decidendo (profondita / classi). Ma quando
    // la scelta ha chiuso un'ora, i conti dopo la scelta si fanno ancora all'ora della cella, e solo se tornano si
    // passa a quelli dell'ora nuova: per sapere quale dei due non e' tornato si rifanno i primi.
    int ora_della_settimana = MIN(profondita / s->classi, s->giorni * s->ore - 1);
    Motivo dopo_la_scelta;
    if (cella >= 0 && m->tipo != ORE_DOCENTE && m->tipo != DOCENTE_SENZA_POSTO && m->tipo != CLASSE_SENZA_SCELTE
        && !limiti_dopo_la_scelta(r, cella, &dopo_la_scelta))
        ora_della_settimana = cella / s->classi;
    segni_del_limite(r, ora_della_settimana / s->ore, ora_della_settimana % s->ore, m, segni);
    return ora_della_settimana;
}

// ---- il conto in dettaglio ----
//
// Anche questo solo per il sito: i pezzi da cui vengono i due numeri del motivo, in JSON. Ogni conto ha i suoi:
//   ORE_DOCENTE            {"ingresso", "oggi", "dopo", "al_giorno"}: le ore di oggi, e i giorni dopo per le ore di
//                          ognuno
//   ORE_DOCENTE_IN_CLASSE  lo stesso, piu' "perche_oggi": perche' oggi sono cosi' poche ("" se non c'e' un perche')
//   ORE_CLASSE             {"oggi", "dopo", "al_giorno", "chiusa"}
//   PRIME_ORE              {"oggi", "dopo", "al_giorno"} per le celle, e "voci": [[voce, ore che restano, lezioni
//                          possibili oggi, in ognuno dei giorni dopo, in tutto], ...]
//   ORARIO_DELLA_CLASSE    {"celle", "voci": [[voce, ore che restano], ...]}
//   DOCENTE_SENZA_POSTO    {"classi": [[classe, perche' non lo prende, [[voce, perche', docente], ...]], ...]}: un
//                          perche' per tutta la classe, oppure ("") uno per ogni sua voce li' che ha ancora ore
//   CLASSE_SENZA_SCELTE    {"voci": [[voce, perche' non si puo' mettere, il docente di cui si parla], ...],
//                           "buca": perche' non si puo' mettere l'ora buca}
// I perche' sono parole: quelle di PercheNo (ricerca.h) in minuscolo, piu' "decisa" (la classe in quest'ora ha gia' la
// sua scelta), "finite" (non ha piu' ore da fare li'), "senza_lezione" (la classe adesso non puo' avere lezione),
// "obbligatoria" (e' una delle prime ore) e "restano_ore" (le ore che restano non stanno nei giorni dopo).

static const char *const PERCHE_NO[] = {"", "non_entrato", "altrove", "troppe_classi", "massimo", "interrotto"};

// Perche' la voce v non si puo' mettere adesso nella sua classe: "" se si puo'; in *chi il docente che non puo'.
static const char *perche_la_voce_no(const Ricerca *r, const int giorno, const int ora, const int v, int *chi) {
    const Voce *voce = &r->s->voce[v];
    *chi = -1;
    if (!r->resto_voce[v]) return "finite";
    for (int i = 0; i < docenti_della_voce(voce); i++) {
        const PercheNo perche = perche_non_puo(r, giorno, ora, voce->classe, voce->docente[i]);
        if (perche != PUO) {
            *chi = voce->docente[i];
            return PERCHE_NO[perche];
        }
    }
    return "";
}

void scrivi_il_conto(FILE *f, const Ricerca *r, const int ora_della_settimana, const Motivo *m) {
    const Scuola *s = r->s;
    const int giorno = ora_della_settimana / s->ore, ora = ora_della_settimana % s->ore;
    const int dopo = s->giorni - 1 - giorno, t = m->docente, k = m->classe;
    const int da = k >= 0 ? prima_ora_libera(r, giorno, ora, k) : ora;
    int primo = 1, chi;
    switch (m->tipo) {
        case ORE_DOCENTE:
            fprintf(f, "{\"ingresso\":%d,\"oggi\":%d,\"dopo\":%d,\"al_giorno\":%d}", s->ingresso[t],
                    MAX(0, s->ore - MAX(ora, s->ingresso[t])), dopo, s->ore - s->ingresso[t]);
            break;
        case ORE_DOCENTE_IN_CLASSE: {
            // perche' oggi puo' cosi' poco: le stesse strade di ancora_oggi
            const int fatte = da < s->ore ? r->ore_oggi[(giorno * s->classi + k) * s->docenti + t] : 0;
            const char *perche = "";
            if (da >= s->ore || r->chiusa[giorno * s->classi + k]) perche = "finita";
            else if (fatte >= MAX_ORE_DI_SEGUITO) perche = "massimo";
            else if (fatte && r->occupato[(giorno * s->ore + da) * s->docenti + t]) perche = "altrove";
            else if (fatte && !presente(r, giorno, da - 1, k, t)) perche = "interrotto";
            else if (!fatte && r->classi_oggi[giorno * s->docenti + t] >= MAX_CLASSI_AL_GIORNO) perche = "troppe_classi";
            fprintf(f, "{\"ingresso\":%d,\"oggi\":%d,\"dopo\":%d,\"al_giorno\":%d,\"perche_oggi\":\"%s\"}",
                    s->ingresso[t], ancora_oggi(r, giorno, da, s->ore - 1, k, t), dopo,
                    MIN(MAX_ORE_DI_SEGUITO, s->ore - s->ingresso[t]), perche);
            break;
        }
        case ORE_CLASSE: {
            const int chiusa = r->chiusa[giorno * s->classi + k];
            fprintf(f, "{\"oggi\":%d,\"dopo\":%d,\"al_giorno\":%d,\"chiusa\":%d}", chiusa ? 0 : s->ore - da, dopo,
                    s->ore, chiusa);
            break;
        }
        case PRIME_ORE:
            fprintf(f, "{\"oggi\":%d,\"dopo\":%d,\"al_giorno\":%d,\"voci\":[", MAX(0, m->ora - da + 1), dopo,
                    m->ora + 1);
            for (int v = s->prima_voce[k]; v < s->prima_voce[k + 1]; v++) {
                if (!r->resto_voce[v]) continue;
                int oggi, al_giorno;
                const int lezioni = lezioni_nella_fascia(r, giorno, da, m->ora, v, &oggi, &al_giorno);
                fprintf(f, "%s[%d,%d,%d,%d,%d]", primo ? "" : ",", v, r->resto_voce[v], oggi, al_giorno, lezioni);
                primo = 0;
            }
            fputs("]}", f);
            break;
        case ORARIO_DELLA_CLASSE:
            fprintf(f, "{\"celle\":%d,\"voci\":[",
                    dopo * s->ore + (r->chiusa[giorno * s->classi + k] ? 0 : s->ore - da));
            for (int v = s->prima_voce[k]; v < s->prima_voce[k + 1]; v++) {
                if (!r->resto_voce[v]) continue;
                fprintf(f, "%s[%d,%d]", primo ? "" : ",", v, r->resto_voce[v]);
                primo = 0;
            }
            fputs("]}", f);
            break;
        case DOCENTE_SENZA_POSTO:
            // ogni sua classe: perche' in quest'ora non lo prende
            fputs("{\"classi\":[", f);
            for (int j = 0; j < r->n_classi_di[t]; j++) {
                const int sua = r->classe_di[t * s->classi + j];
                // prima i perche' che valgono per tutta la classe; se non ce ne sono, voce per voce
                const char *perche = "";
                if (r->orario[cella_di(r, giorno, ora, sua)] != VUOTA) perche = "decisa";
                else if (!r->resto_in_classe[sua * s->docenti + t]) perche = "finite";
                else if (ora >= s->obbligatorie && (r->chiusa[giorno * s->classi + sua]
                                                    || r->resto_classe[sua] <= dopo * s->obbligatorie))
                    perche = "senza_lezione";
                fprintf(f, "%s[%d,\"%s\",[", j ? "," : "", sua, perche);
                primo = 1;
                for (int v = s->prima_voce[sua]; !*perche && v < s->prima_voce[sua + 1]; v++)
                    if (r->resto_voce[v] && nella_voce(&s->voce[v], t)) {
                        const char *della_voce = perche_la_voce_no(r, giorno, ora, v, &chi);
                        fprintf(f, "%s[%d,\"%s\",%d]", primo ? "" : ",", v, della_voce, chi);
                        primo = 0;
                    }
                fputs("]]", f);
            }
            fputs("]}", f);
            break;
        case CLASSE_SENZA_SCELTE:
            fputs("{\"voci\":[", f);
            for (int v = s->prima_voce[k]; v < s->prima_voce[k + 1]; v++) {
                if (!r->resto_voce[v]) continue;  // le voci finite non si elencano
                const char *perche = perche_la_voce_no(r, giorno, ora, v, &chi);
                fprintf(f, "%s[%d,\"%s\",%d]", primo ? "" : ",", v, perche, chi);
                primo = 0;
            }
            fprintf(f, "],\"buca\":\"%s\"}", ora < s->obbligatorie ? "obbligatoria" : "restano_ore");
            break;
        default:
            fputs("{}", f);
    }
}

// ---- i conti docente per docente ----
//
// Ancora per il sito: a un nodo qualunque, per ogni docente, gli stessi due conti che i limiti fanno su di lui, con
// i loro numeri, che tornino o no:
//   [[docente, ore che gli restano, ore in cui e' ancora a scuola e libero,
//     [[classe, ore che gli restano li', ore che ci puo' ancora fare], ...]], ...]
// (le classi sono quelle dove ha ancora ore). Un conto non torna quando il primo numero e' piu' grande del secondo.
void scrivi_i_docenti(FILE *f, const Ricerca *r, const int ora_della_settimana) {
    const Scuola *s = r->s;
    const int giorno = ora_della_settimana / s->ore, ora = ora_della_settimana % s->ore;
    const int dopo = s->giorni - 1 - giorno;
    fputc('[', f);
    for (int t = 0; t < s->docenti; t++) {
        // se in quest'ora e' gia' stato messo in una classe, quest'ora non e' piu' libera: si conta da quella dopo
        const int occupato = r->occupato[ora_della_settimana * s->docenti + t];
        fprintf(f, "%s[%d,%d,%d,[", t ? "," : "", t, r->resto_docente[t], ore_libere_da(r, giorno, ora + occupato, t));
        for (int j = 0, primo = 1; j < r->n_classi_di[t]; j++) {
            const int k = r->classe_di[t * s->classi + j], restano = r->resto_in_classe[k * s->docenti + t];
            if (!restano) continue;
            // lo stesso conto di docente_in_classe
            const int possibili = dopo * MIN(MAX_ORE_DI_SEGUITO, s->ore - s->ingresso[t])
                                  + ancora_oggi(r, giorno, prima_ora_libera(r, giorno, ora, k), s->ore - 1, k, t);
            fprintf(f, "%s[%d,%d,%d]", primo ? "" : ",", k, restano, possibili);
            primo = 0;
        }
        fputs("]]", f);
    }
    fputc(']', f);
}
