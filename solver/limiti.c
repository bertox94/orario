#include "ricerca.h"

// I limiti: conti che devono tornare perche' da un nodo possa ancora uscire un orario. Sono condizioni necessarie:
// scartano solo rami in cui un orario non c'e', quindi non fanno perdere nessun orario.
//
// In ogni conto «oggi» e' il giorno dell'ora che si sta decidendo, e per ogni classe la prima ora non ancora decisa
// e' quell'ora, oppure la successiva se in quell'ora la classe e' gia' stata decisa (prima_ora_libera).

static int non_torna(Motivo *m, const TipoMotivo tipo, const int docente, const int classe, const int ora,
                     const int servono, const int possibili) {
    *m = (Motivo){tipo, docente, classe, ora, servono, possibili};
    return 0;
}

static int prima_ora_libera(const Ricerca *r, const int giorno, const int ora, const int k) {
    return ora + (r->orario[cella_di(r, giorno, ora, k)] != VUOTA);
}

int ancora_oggi(const Ricerca *r, const int giorno, const int da, const int fino_a, const int k, const int t) {
    const Scuola *s = r->s;
    if (da >= s->ore || r->chiusa[giorno * s->classi + k]) return 0;
    const int fatte = r->ore_oggi[(giorno * s->classi + k) * s->docenti + t];
    const int altrove = r->occupato[(giorno * s->ore + da) * s->docenti + t];  // nell'ora `da` e' in un'altra classe
    int comincia = da, massimo = MAX_ORE_DI_SEGUITO - fatte;
    if (fatte) {
        if (altrove || !presente(r, giorno, da - 1, k, t)) return 0;
    } else {
        if (r->classi_oggi[giorno * s->docenti + t] >= MAX_CLASSI_AL_GIORNO) return 0;
        comincia = MAX(da, s->ingresso[t]);
        if (comincia == da && altrove) comincia++;
    }
    return MAX(0, MIN(massimo, fino_a - comincia + 1));
}

int ore_libere_da(const Ricerca *r, const int giorno, const int da, const int t) {
    const Scuola *s = r->s;
    const int dopo = s->giorni - 1 - giorno;
    return dopo * (s->ore - s->ingresso[t]) + MAX(0, s->ore - MAX(da, s->ingresso[t]));
}

// Un docente in una classe: le ore che gli restano li' devono stare in quelle che ci puo' ancora fare, oggi e nei
// giorni dopo (in un giorno intero: il massimo di ore di seguito, o meno se entra tardi).
static int docente_in_classe(const Ricerca *r, const int giorno, const int ora, const int k, const int t, Motivo *m) {
    const Scuola *s = r->s;
    const int restano = r->resto_in_classe[k * s->docenti + t];
    if (!restano) return 1;
    const int dopo = s->giorni - 1 - giorno, al_giorno = MIN(MAX_ORE_DI_SEGUITO, s->ore - s->ingresso[t]);
    const int da = prima_ora_libera(r, giorno, ora, k);
    const int possibili = dopo * al_giorno + ancora_oggi(r, giorno, da, s->ore - 1, k, t);
    return restano <= possibili ? 1 : non_torna(m, ORE_DOCENTE_IN_CLASSE, t, k, -1, restano, possibili);
}

// Una classe.
// - Le ore che le restano devono stare nelle celle che le restano (oggi, se non ha gia' avuto un'ora buca, e nei
//   giorni dopo).
// - Le prime ore: per ogni ora obbligatoria h, le celle che deve ancora riempire tra la prima ora e h non possono
//   essere piu' delle lezioni che le sue voci possono ancora fare in quelle ore. Una voce ne puo' fare: oggi quanto
//   possono i suoi docenti (per una coppia, chi dei due puo' meno), nei giorni dopo le ore tra l'ingresso e h (per una
//   coppia, di chi entra piu' tardi), non piu' del massimo di ore di seguito; in tutto non piu' delle ore che le
//   restano. E' il conto che si accorge dei docenti che entrano tardi: alle prime ore non ci sono.
static int classe(const Ricerca *r, const int giorno, const int ora, const int k, Motivo *m) {
    const Scuola *s = r->s;
    const int dopo = s->giorni - 1 - giorno, da = prima_ora_libera(r, giorno, ora, k);
    const int oggi = r->chiusa[giorno * s->classi + k] ? 0 : s->ore - da;
    if (r->resto_classe[k] > dopo * s->ore + oggi)
        return non_torna(m, ORE_CLASSE, -1, k, -1, r->resto_classe[k], dopo * s->ore + oggi);
    for (int h = 0; h < s->obbligatorie; h++) {
        const int celle = dopo * (h + 1) + MAX(0, h - da + 1);
        int lezioni = 0;
        for (int v = s->prima_voce[k]; v < s->prima_voce[k + 1] && lezioni < celle; v++) {
            if (!r->resto_voce[v]) continue;
            int oggi_voce = s->ore, entra = 0;
            for (int i = 0; i < docenti_della_voce(&s->voce[v]); i++) {
                const int t = s->voce[v].docente[i];
                oggi_voce = MIN(oggi_voce, ancora_oggi(r, giorno, da, h, k, t));
                entra = MAX(entra, s->ingresso[t]);
            }
            const int al_giorno = MIN(MAX_ORE_DI_SEGUITO, MAX(0, h + 1 - entra));
            lezioni += MIN(r->resto_voce[v], oggi_voce + dopo * al_giorno);
        }
        if (lezioni < celle) return non_torna(m, PRIME_ORE, -1, k, h, celle, lezioni);
    }
    return 1;
}

int limiti_a_inizio_ora(Ricerca *r, const int giorno, const int ora, Motivo *m) {
    const Scuola *s = r->s;
    // Un docente in tutto: le ore che gli restano devono stare nelle ore in cui e' ancora a scuola. Se ci stanno
    // solo contando anche quest'ora, in quest'ora deve insegnare.
    for (int t = 0; t < s->docenti; t++) {
        const int restano = r->resto_docente[t], libere = ore_libere_da(r, giorno, ora, t);
        if (restano > libere) return non_torna(m, ORE_DOCENTE, t, -1, -1, restano, libere);
        const int qui = (giorno * s->ore + ora) * s->docenti + t;
        r->deve[qui] = restano > ore_libere_da(r, giorno, ora + 1, t);
        r->margine[qui] = libere - restano;
    }
    for (int k = 0; k < s->classi; k++) {
        if (!classe(r, giorno, ora, k, m)) return 0;
        for (int i = 0; i < r->n_docenti_di[k]; i++)
            if (!docente_in_classe(r, giorno, ora, k, r->docente_di[k * s->docenti + i], m)) return 0;
        // l'orario della classe da sola cambia solo quando si decide una sua cella: qui serve solo in partenza
        if (!giorno && !ora && !la_classe_ha_un_orario(r, giorno, ora, k))
            return non_torna(m, ORARIO_DELLA_CLASSE, -1, k, -1, r->resto_classe[k], 0);
    }
    return 1;
}

int limiti_dopo_la_scelta(const Ricerca *r, const int cella, Motivo *m) {
    const Scuola *s = r->s;
    const int k = cella % s->classi, ora = cella / s->classi % s->ore, giorno = cella / s->classi / s->ore;
    // la classe appena decisa: la sua prima ora libera e' avanzata, per lei e per tutti i suoi docenti
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
