// La ricerca dell'orario: backtracking cella per cella.
//
// L'orario e' un elenco di celle (giorno, ora, classe) in quest'ordine: per ogni giorno, per ogni ora, tutte le classi.
// La cella pos e' giorno = pos / (ore * classi), ora = (pos / classi) % ore, classe = pos % classi.
// cerca_orario(pos) decide la cella pos e scende a pos + 1: prova, nell'ordine della coda dei docenti (chi ha piu' ore
// da fare prima), ogni docente in coppia con un altro (una codocenza) e poi da solo; dopo le ore obbligatorie prova
// anche l'ora buca. Quando tutte le celle sono decise (pos == orario_len) l'orario e' completo e la ricerca finisce
// (si cerca un orario solo): cerca_orario ritorna 1 e chi l'ha chiamata ritorna subito anche lui, senza provare
// altro, cosi' l'orario trovato resta in r->orario. La lanciano main.c, prove.c e caso.c.
//
// Niente variabili globali: la scuola e lo stato della ricerca arrivano a ogni funzione nella Ricerca r (ricerca.h).

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ricerca.h"
#include "vincoli.h"

// ---- creare la ricerca ----

Ricerca *nuova_ricerca(const Scuola *s) {
    Ricerca *r = calloc(1, sizeof(Ricerca));
    assert(r);
    r->scuola = s;
    r->orario_len = s->actual_giorni_settimana * s->actual_ore_giornaliere * s->actual_classi_sezione;
    for (int i = 0; i < r->orario_len; i++) r->orario[i] = (teaching_slot){-1, -1};
    // la coda: ogni docente con tutte le sue ore
    for (int p = 0; p < s->actual_insegnanti; p++) {
        r->queue_items[p] = (QueueItem){p, 0};
        for (int c = 0; c < s->actual_classi_sezione; c++)
            r->queue_items[p].priority += get_ore_insegnante_in_una_classe(s, c, p);
    }
    // ogni cella ha il suo blocco di puntatori: i docenti della sua classe (gli altri li' non possono insegnare)
    for (int c = 0; c < s->actual_classi_sezione; c++) {
        for (int p = 0; p < s->actual_insegnanti; p++)
            r->queue_sz_classe[c] += get_ore_insegnante_in_una_classe(s, c, p) > 0;
        if (r->queue_sz_classe[c] > r->queue_sz) r->queue_sz = r->queue_sz_classe[c];
    }
    r->queue_mul = malloc(sizeof(QueueItem *) * r->orario_len * r->queue_sz);
    assert(r->queue_mul);
    for (int pos = 0; pos < r->orario_len; pos++) {
        const int c = pos % s->actual_classi_sezione;
        QueueItem **coda = &r->queue_mul[pos * r->queue_sz];
        for (int p = 0; p < s->actual_insegnanti; p++)
            if (get_ore_insegnante_in_una_classe(s, c, p) > 0) *coda++ = &r->queue_items[p];
    }
    return r;
}

void libera_ricerca(Ricerca *r) {
    free(r->queue_mul);
    free(r);
}

// ---- la cella ----

Cella cella(const Ricerca *r, const int pos) {
    const Scuola *s = r->scuola;
    Cella c = {.pos = pos,
               .giorno = pos / s->actual_classi_sezione / s->actual_ore_giornaliere,
               .ora = pos / s->actual_classi_sezione % s->actual_ore_giornaliere,
               .cla = pos % s->actual_classi_sezione};
    c.base2d_ci = c.cla * INSEGNANTE_SZ;
    c.base3d_goi = c.giorno * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ + c.ora * INSEGNANTE_SZ;
    c.base2d_gc = c.giorno * CLASSE_SEZIONE_SZ + c.cla;
    c.base3d_gci = c.giorno * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ + c.cla * INSEGNANTE_SZ;
    c.coda = &r->queue_mul[pos * r->queue_sz];
    c.n_coda = r->queue_sz_classe[c.cla];
    return c;
}

// le ore che restano al docente p nella classe della cella (da solo e in codocenza)
int ore_rimaste(const Ricerca *r, const Cella *c, const int p) {
    return r->scuola->ore_classi_docenti[c->base2d_ci + p] - r->ore_classi_docenti_svolte[c->base2d_ci + p];
}

// ---- i vincoli della classe ----

// le ore che la classe deve ancora fare, contando la cella che si sta decidendo
int ore_rimaste_alla_classe(const Ricerca *r, const Cella *c) {
    return r->scuola->ore_classi[c->cla] - r->ore_classe_svolte[c->cla];
}

// Le celle obbligatorie (le prime PRIME_N_ORE_OBBLIGATORIE ore di ogni giorno) che la classe deve ancora riempire,
// contando questa. Prima di questa cella ce ne sono giorno * PRIME_N_ORE_OBBLIGATORIE + ora (oggi); dopo l'ultima ora
// obbligatoria il conto si ferma a PRIME_N_ORE_OBBLIGATORIE - 1: cosi' in un'ora facoltativa risulta una cella in
// piu', quella che una lezione qui consumerebbe senza riempirne una obbligatoria.
int celle_obbligatorie_rimaste(const Ricerca *r, const Cella *c) {
    const int prima = c->giorno * PRIME_N_ORE_OBBLIGATORIE + MIN(c->ora, PRIME_N_ORE_OBBLIGATORIE - 1);
    return PRIME_N_ORE_OBBLIGATORIE * r->scuola->actual_giorni_settimana - prima;
}

// La classe puo' avere lezione in questa cella?
// - le ore che le restano (contando questa) devono bastare per le celle obbligatorie che restano: una lezione in
//   un'ora facoltativa deve lasciare almeno un'ora per ognuna delle celle obbligatorie dei giorni dopo;
// - dopo un'ora buca la giornata della classe e' finita.
int la_classe_puo_avere_lezione(const Ricerca *r, const Cella *c) {
    if (ore_rimaste_alla_classe(r, c) < celle_obbligatorie_rimaste(r, c)) return 0;
    return r->n_ore_buche[c->base2d_gc] == 0;
}

// I posti che restano alla classe nei giorni dopo oggi: tutte le ore, obbligatorie e no.
int posti_nei_giorni_dopo(const Ricerca *r, const Cella *c) {
    return (r->scuola->actual_giorni_settimana - 1 - c->giorno) * r->scuola->actual_ore_giornaliere;
}

// La classe puo' avere un'ora buca in questa cella?
// - solo dopo le ore obbligatorie;
// - le ore che le restano devono stare nei giorni dopo: dopo un'ora buca oggi non ha piu' lezione, quindi quello che
//   non sta nei giorni dopo non si potrebbe piu' mettere. Se non ci stanno, la lezione a quest'ora e' tassativa.
// Cosi' l'ultimo giorno l'ora buca e' permessa solo a una classe che ha finito le ore: a orario completo ogni classe
// le ha fatte tutte, e quindi (nessuno puo' farne piu' delle sue) anche ogni docente e ogni coppia.
int la_classe_puo_avere_ora_buca(const Ricerca *r, const Cella *c) {
    if (c->ora < PRIME_N_ORE_OBBLIGATORIE) return 0;
    return ore_rimaste_alla_classe(r, c) <= posti_nei_giorni_dopo(r, c);
}

// Lo stato della classe in questa cella, dalle due domande sopra: decide che cosa prova la ricerca.
//   deve fare attivita'      puo' avere lezione, non un'ora buca    si provano i docenti
//   puo' fare attivita'      puo' avere lezione e un'ora buca       si provano i docenti, poi l'ora buca
//   non puo' fare attivita'  non lezione, solo un'ora buca          si mette l'ora buca
// (Ne' lezione ne' ora buca non succede con dati che hanno passato la verifica: verifica_dati.h.)
StatoClasse stato_della_classe(const Ricerca *r, const Cella *c) {
    const int lezione = la_classe_puo_avere_lezione(r, c), buca = la_classe_puo_avere_ora_buca(r, c);
    assert(lezione || buca);
    return lezione && buca ? PUO_FARE_ATTIVITA : lezione ? DEVE_FARE_ATTIVITA : NON_PUO_FARE_ATTIVITA;
}

// ---- i vincoli del docente ----

// a quest'ora non insegna gia' in un'altra classe
int prof_è_disponibile(const Ricerca *r, const Cella *c, const int p) {
    return !r->prof_gia_impegnato_giorno_ora[c->base3d_goi + p];
}

// ha ancora ore da fare in questa classe
int deve_ancora_svolgere_ore_in_questa_classe(const Ricerca *r, const Cella *c, const int p) {
    return ore_rimaste(r, c, p) > 0;
}

// oggi in questa classe ha fatto meno di MAX_ORE_PROF_IN_UNA_CLASSE ore
int prof_fa_al_max_n_ore(const Ricerca *r, const Cella *c, const int p) {
    return r->ore_fatte_da_un_prof_in_una_classe_in_giorno[c->base3d_gci + p] < MAX_ORE_PROF_IN_UNA_CLASSE;
}

// le sue ore di oggi in questa classe sono di seguito: se ne ha gia' fatte, l'ultima e' l'ora prima (la cella
// pos - classi, stessa classe un'ora prima)
int prof_fa_ore_contigue(const Ricerca *r, const Cella *c, const int p) {
    if (!r->ore_fatte_da_un_prof_in_una_classe_in_giorno[c->base3d_gci + p]) return 1;
    const teaching_slot *prima = &r->orario[c->pos - r->scuola->actual_classi_sezione];
    return prima->insegnante1 == p || prima->insegnante2 == p;
}

// oggi insegna al massimo in MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO classi diverse, contando questa: va bene se
// in questa classe oggi c'e' gia' stato (non e' una classe in piu'), o se le classi di oggi sono meno del massimo
int ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(const Ricerca *r, const Cella *c, const int p) {
    return r->ore_fatte_da_un_prof_in_una_classe_in_giorno[c->base3d_gci + p] > 0
           || r->classi_fatte_da_un_prof_in_giorno[c->giorno * INSEGNANTE_SZ + p]
              < MAX_CLASSI_DIVERSE_STESSO_PROF_OGNI_GIORNO;
}

// e' gia' entrato a scuola
int verifica_ora_ingresso(const Ricerca *r, const int p, const int ora) {
    return ora >= r->scuola->ora_di_entrata_prof[p];
}

// dove stanno le ore gia' messe della coppia p1, p2 nella classe (come le ore nei dati: prima l'indice minore)
static int *ore_codocenza_svolte(const Ricerca *r, const int cla, int p1, int p2) {
    swap_min_first(&p1, &p2);
    return (int *) &r->ore_classi_codocenza_svolte[cla * INSEGNANTE_SZ * INSEGNANTE_SZ + p1 * INSEGNANTE_SZ + p2];
}

// p1 e p2 hanno ancora ore da fare insieme in questa classe (una codocenza: vale in tutti e due i versi, chiunque dei
// due sia il docente della lezione e chiunque il codocente)
int docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(const Ricerca *r, const Cella *c, const int p1,
                                                                   const int p2) {
    return *ore_codocenza_svolte(r, c->cla, p1, p2)
           < get_ore_insegnanti_codocenza_in_una_classe(r->scuola, c->cla, p1, p2);
}

// ha ancora ore da fare da solo (senza codocente) in questa classe
int deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(const Ricerca *r, const Cella *c, const int p) {
    return r->ore_classi_docenti_alone_svolte[c->base2d_ci + p] < r->scuola->ore_classi_docenti_alone[c->base2d_ci + p];
}

// La regola del sabato. Il sabato e' l'ultimo giorno: un docente che e' nell'ora prima in questa classe e ha ancora
// ore da fare qui non potrebbe tornare dopo (le ore di un giorno sono di seguito) e non ci sono altri giorni, quindi
// deve continuare adesso. Dice quale dei docenti dell'ora prima deve esserci: 1 il primo, 2 il secondo, 3 tutti e due,
// 0 nessuno (non e' sabato, e' la prima ora, o nessuno dei due ha ancora ore qui).
static int sabato_chi_deve_continuare(const Ricerca *r, const Cella *c, int *prev_prof1, int *prev_prof2) {
    if (c->giorno != r->scuola->actual_giorni_settimana - 1 || c->ora == 0) return 0;
    *prev_prof1 = r->orario[c->pos - r->scuola->actual_classi_sezione].insegnante1;
    *prev_prof2 = r->orario[c->pos - r->scuola->actual_classi_sezione].insegnante2;
    const int r1 = *prev_prof1 != -1 && ore_rimaste(r, c, *prev_prof1) > 0;
    const int r2 = *prev_prof2 != -1 && ore_rimaste(r, c, *prev_prof2) > 0;
    return r1 + 2 * r2;
}

// la coppia (prof1, prof2) rispetta la regola del sabato: contiene chi deve continuare (tutti e due, se tutti e due)
int sabato_esaurisci_priorita_subito_cod(const Ricerca *r, const Cella *c, const int prof1, const int prof2) {
    int prev_prof1, prev_prof2;
    switch (sabato_chi_deve_continuare(r, c, &prev_prof1, &prev_prof2)) {
        case 3: return prof1 == prev_prof1 && prof2 == prev_prof2 || prof1 == prev_prof2 && prof2 == prev_prof1;
        case 1: return prof1 == prev_prof1 || prof2 == prev_prof1;
        case 2: return prof1 == prev_prof2 || prof2 == prev_prof2;
        default: return 1;
    }
}

// il docente prof da solo rispetta la regola del sabato: e' lui che deve continuare (se devono continuare tutti e due,
// da solo non va bene)
int sabato_esaurisci_priorita_subito(const Ricerca *r, const Cella *c, const int prof) {
    int prev_prof1, prev_prof2;
    switch (sabato_chi_deve_continuare(r, c, &prev_prof1, &prev_prof2)) {
        case 3: return 0;
        case 1: return prof == prev_prof1;
        case 2: return prof == prev_prof2;
        default: return 1;
    }
}

// I controlli del docente p1 in questa cella, prima di provarlo in coppia o da solo.
int docente_puo_insegnare(const Ricerca *r, const Cella *c, const int p1) {
    return prof_è_disponibile(r, c, p1)
           && deve_ancora_svolgere_ore_in_questa_classe(r, c, p1)
           && prof_fa_al_max_n_ore(r, c, p1)
           && prof_fa_ore_contigue(r, c, p1)
           && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(r, c, p1)
           && verifica_ora_ingresso(r, p1, c->ora);
}

// I controlli del secondo docente p2 della coppia, insieme a p1. Le coppie si provano nell'ordine della coda: p1 e'
// quello che viene prima, p2 uno di quelli dopo. Quindi p2 non e' sempre il codocente della lezione: puo' essere il
// docente, e p1 il suo codocente. (Che p2 abbia ancora ore nella classe lo dicono le ore insieme rimaste.)
int secondo_della_coppia_puo_insegnare(const Ricerca *r, const Cella *c, const int p1, const int p2) {
    return prof_è_disponibile(r, c, p2)
           && docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(r, c, p1, p2)
           && prof_fa_al_max_n_ore(r, c, p2)
           && prof_fa_ore_contigue(r, c, p2)
           && sabato_esaurisci_priorita_subito_cod(r, c, p1, p2)
           && ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(r, c, p2)
           && verifica_ora_ingresso(r, p2, c->ora);
}

// I controlli per mettere p1 da solo.
int docente_puo_insegnare_da_solo(const Ricerca *r, const Cella *c, const int p1) {
    return deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(r, c, p1)
           && sabato_esaurisci_priorita_subito(r, c, p1);
}

// ---- mettere e togliere un docente ----

// Un'ora del docente p in questa cella (delta = 1) o tolta (delta = -1): priorita' nella coda, impegnato a quest'ora,
// ore fatte nella classe, ore fatte oggi nella classe e classi di oggi.
static void segna_ora(Ricerca *r, const Cella *c, const int p, const int delta) {
    r->queue_items[p].priority -= delta;
    r->prof_gia_impegnato_giorno_ora[c->base3d_goi + p] = delta > 0;
    r->ore_classi_docenti_svolte[c->base2d_ci + p] += delta;
    // le ore di oggi in questa classe; la prima che entra e l'ultima che esce cambiano anche le classi di oggi
    int *ore_oggi = &r->ore_fatte_da_un_prof_in_una_classe_in_giorno[c->base3d_gci + p];
    if (*ore_oggi == 0) r->classi_fatte_da_un_prof_in_giorno[c->giorno * INSEGNANTE_SZ + p]++;
    *ore_oggi += delta;
    if (*ore_oggi == 0) r->classi_fatte_da_un_prof_in_giorno[c->giorno * INSEGNANTE_SZ + p]--;
}

void segna_da_solo(Ricerca *r, const Cella *c, const int p, const int delta) {
    segna_ora(r, c, p, delta);
    r->ore_classi_docenti_alone_svolte[c->base2d_ci + p] += delta;
}

void segna_coppia(Ricerca *r, const Cella *c, const int p1, const int p2, const int delta) {
    segna_ora(r, c, p1, delta);
    segna_ora(r, c, p2, delta);
    *ore_codocenza_svolte(r, c->cla, p1, p2) += delta;
}

// ---- la ricerca ----

// Prova tutte le lezioni possibili in questa cella, nell'ordine della coda (i docenti della classe, da chi ha piu' ore
// da fare in tutto): ogni docente (ii) prima in coppia con
// ognuno di quelli dopo di lui nella coda (jj) con cui ha ore insieme qui, poi da solo. In una coppia il docente della
// lezione puo' essere tanto ii quanto jj: la coppia si prova una volta sola, quando si arriva al primo dei due.
// Ritorna 1 se la ricerca e' finita (vedi cerca_orario): allora non si prova altro e non si toglie niente.
static int prova_lezioni(Ricerca *r, const Cella *c) {
    r->ore_classe_svolte[c->cla]++;  // la lezione di questa cella conta gia' tra le ore della classe
    for (int ii = 0; ii < c->n_coda; ii++) {
        const int p1 = c->coda[ii]->teacher;
        if (!docente_puo_insegnare(r, c, p1)) continue;
        for (int jj = ii + 1; jj < c->n_coda; jj++) {
            const int p2 = c->coda[jj]->teacher;
            if (!secondo_della_coppia_puo_insegnare(r, c, p1, p2)) continue;
            segna_coppia(r, c, p1, p2, 1);
            r->orario[c->pos] = (teaching_slot){p1, p2};
            if (cerca_orario(r, c->pos + 1)) return 1;
            segna_coppia(r, c, p1, p2, -1);
        }
        if (docente_puo_insegnare_da_solo(r, c, p1)) {
            segna_da_solo(r, c, p1, 1);
            r->orario[c->pos] = (teaching_slot){p1, -1};
            if (cerca_orario(r, c->pos + 1)) return 1;
            segna_da_solo(r, c, p1, -1);
        }
    }
    r->ore_classe_svolte[c->cla]--;
    return 0;
}

// L'ora buca: da li' la classe non ha piu' lezione oggi. Ritorna 1 se la ricerca e' finita.
static int prova_ora_buca(Ricerca *r, const Cella *c) {
    r->orario[c->pos] = (teaching_slot){-1, -1};
    r->n_ore_buche[c->base2d_gc]++;
    if (cerca_orario(r, c->pos + 1)) return 1;
    r->n_ore_buche[c->base2d_gc]--;
    return 0;
}

// Decide la cella pos e scende alle celle dopo. Ritorna 1 se la ricerca e' finita: l'orario e' completo (trovato) o
// si e' arrivati al limite di nodi (fermato). Ritorna 0 se in questa cella non c'e' piu' niente da provare: chi l'ha
// chiamata toglie la sua scelta e prova la successiva (si torna indietro).
int cerca_orario(Ricerca *r, const int pos) {
    if (r->limite_nodi && r->nodi >= r->limite_nodi) {  // il limite di nodi: la ricerca si ferma qui
        r->fermato = 1;
        return 1;
    }
    r->nodi++;
    if (pos > r->pos_max) r->pos_max = pos;
    if (r->ogni_nodo) r->ogni_nodo(r, pos);
    if (pos == r->orario_len) {  // tutte le celle decise: l'orario e' completo
        r->trovato = 1;
        return 1;
    }
    const Cella c = cella(r, pos);
    sort_queue(c.coda, c.n_coda);
    switch (stato_della_classe(r, &c)) {
        case DEVE_FARE_ATTIVITA: return prova_lezioni(r, &c);
        case PUO_FARE_ATTIVITA: return prova_lezioni(r, &c) || prova_ora_buca(r, &c);
        case NON_PUO_FARE_ATTIVITA: return prova_ora_buca(r, &c);
    }
    return 0;
}
