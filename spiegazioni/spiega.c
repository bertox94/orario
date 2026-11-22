#include "spiega.h"

#include "../solver/limiti.h"

// Le spiegazioni di un nodo, per il sito (che cosa sono e' scritto in spiega.h). Tre cose: i SEGNI (le celle di cui
// parla un conto che non torna), il CONTO in dettaglio (i pezzi dei suoi due numeri), e i conti DOCENTE PER DOCENTE.

// ---- i segni: le celle di cui parla un conto che non torna ----
//
// Per il tasto «Mostra nell'orario» del sito. Devono dire la stessa cosa dei conti: le ore di oggi si segnano
// chiedendole alla stessa ancora_oggi che le conta (solver/limiti.h).

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
// I pezzi da cui vengono i due numeri del motivo, in JSON. Ogni conto ha i suoi:
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
// A un nodo qualunque, per ogni docente, gli stessi due conti che i limiti fanno su di lui, con
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
