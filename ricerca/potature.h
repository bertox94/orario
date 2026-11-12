#ifndef UNTITLED_POTATURE_H
#define UNTITLED_POTATURE_H

// Le potature: i sei controlli della verifica dei dati (verifiche/verifica_dati.h) rifatti durante la ricerca su
// quello che resta. La verifica dei dati li fa una volta, sulle ore di tutta la settimana; qui si rifanno a ogni nodo
// sulle ore ancora da mettere e sul tempo ancora libero, da oggi alla fine. Sono condizioni necessarie: se una non
// vale, da questo nodo un orario non puo' piu' uscire e la ricerca torna indietro subito, invece di scoprirlo molte
// celle dopo. Scartano solo rami senza orario: l'orario trovato e' lo stesso, cambia quanti nodi servono.
//
// Quando: all'ingresso di cerca_orario(pos), con le celle fino a pos - 1 decise e prima di provare qualcosa in pos.
// «Oggi» e' il giorno di pos; in ogni classe la prima ora non ancora decisa e' quella di pos per le classi da quella
// di pos in poi, e l'ora dopo per le classi prima (che a quell'ora sono gia' decise).
//
// Quello che un docente puo' ancora fare oggi in una classe (lo usano la 1, la 3 e la 5): niente se la classe oggi ha
// gia' un'ora buca; se oggi ci ha gia' insegnato, solo continuando subito (le sue ore sono di seguito) fino a 3 ore;
// se no, dal suo ingresso (e non in un'ora in cui e' gia' in un'altra classe), al massimo 3 ore, e solo se oggi non e'
// gia' stato in 3 classi.

#include "ricerca.h"

// Le potature, nell'ordine della verifica dei dati. r->potature_attive dice quali si fanno: il bit i (1 << i) accende
// la potatura i.
enum {
    POTATURA_ORE_DOCENTE_IN_CLASSE,  // 1. le ore che restano al docente in una classe stanno nel tempo che ha li'
    POTATURA_ORE_DOCENTE,            // 2. le ore che restano al docente in tutto stanno nelle sue ore libere
    POTATURA_CLASSI_AL_GIORNO,       // 3. le classi che gli restano stanno in tre al giorno nei giorni dopo
    POTATURA_ORE_CLASSE,             // 4. le ore che restano alla classe stanno nelle lezioni che puo' ancora avere
    POTATURA_LEZIONI_OBBLIGATORIE,   // 5. le lezioni ancora possibili bastano per le ore obbligatorie che restano
    POTATURA_DOCENTI_PER_ORA,        // 6. ora per ora, i docenti liberi bastano per le classi
    N_POTATURE
};
#define TUTTE_LE_POTATURE ((1 << N_POTATURE) - 1)

// Prepara quello che serve alle potature (per ogni classe, i suoi docenti e le sue lezioni): una volta, dopo
// nuova_ricerca. libera_potature quando non serve piu'.
void prepara_potature(Ricerca *r);
void libera_potature(Ricerca *r);

// Con le celle fino a pos - 1 decise: -1 se tutte le potature accese passano, se no quale non passa (la prima).
int potatura(const Ricerca *r, int pos);

#endif //UNTITLED_POTATURE_H
