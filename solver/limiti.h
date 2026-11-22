#ifndef LIMITI_H
#define LIMITI_H

#include "ricerca.h"

// I limiti: i conti che devono tornare perche' da un nodo possa ancora uscire un orario (limiti.c e, per la classe da
// sola, classe_sola.c). Qui c'e' quello che ne usa la ricerca, e i conti piu' piccoli che servono anche a chi spiega
// un limite al sito (spiegazioni/).

// I due momenti in cui la ricerca li guarda: 1 se i conti tornano; se no 0, con il perche' in *motivo.
// All'inizio di un'ora, per tutti i docenti e tutte le classi; prepara anche r->deve e r->margine per quell'ora.
int limiti_a_inizio_ora(Ricerca *r, int giorno, int ora, Motivo *motivo);
// Dopo una scelta, per quello che la scelta ha cambiato: la classe della cella e le altre classi dei docenti messi.
int limiti_dopo_la_scelta(const Ricerca *r, int cella, Motivo *motivo);

// La classe k, guardata da sola, ha ancora un orario per le ore che le restano (classe_sola.c): 1 se c'e' o non si
// riesce a dirlo in poco tempo, 0 se non c'e'.
int la_classe_ha_un_orario(const Ricerca *r, int giorno, int ora, int k);

// ---- i conti piu' piccoli ----

// Quante ore il docente t puo' ancora fare oggi nella classe k tra l'ora `da` (la prima della classe non ancora
// decisa) e l'ora `fino_a`, comprese. Niente se la classe ha gia' avuto un'ora buca. Se oggi ci ha gia' insegnato,
// solo continuando subito (c'era nell'ora prima, e adesso non e' altrove) fino al massimo di ore di seguito. Se no,
// dal suo ingresso (dall'ora dopo, se adesso e' in un'altra classe), e solo se oggi non e' gia' in troppe classi.
int ancora_oggi(const Ricerca *r, int giorno, int da, int fino_a, int k, int t);

// le ore in cui il docente t e' a scuola dall'ora `da` di quel giorno alla fine della settimana
int ore_libere_da(const Ricerca *r, int giorno, int da, int t);

// La prima ora di oggi che la classe k deve ancora decidere: l'ora che si sta decidendo, o quella dopo se la classe in
// quest'ora ha gia' la sua scelta.
int prima_ora_libera(const Ricerca *r, int giorno, int ora, int k);

// Le lezioni che la voce v puo' fare nella fascia fino all'ora h (compresa), con la prima ora libera della sua classe
// in `da`: quelle di oggi vanno in *oggi, quelle di ognuno dei giorni dopo in *al_giorno; ritorna il totale, che non
// e' mai piu' delle ore che restano alla voce.
int lezioni_nella_fascia(const Ricerca *r, int giorno, int da, int h, int v, int *oggi, int *al_giorno);

#endif
