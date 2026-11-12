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
// Quelle accese in una ricerca nuova: la 1, la 2, la 4 e la 5. La 3 e la 6 sono giuste ma non ripagano il loro costo:
// misurate il 12 novembre 2026 su 341 casi casuali a 2 milioni di nodi, queste quattro trovano gli stessi 297 orari di
// tutte e sei (senza potature: 175) in tre quarti del tempo.
#define POTATURE_ACCESE (1 << POTATURA_ORE_DOCENTE_IN_CLASSE | 1 << POTATURA_ORE_DOCENTE | 1 << POTATURA_ORE_CLASSE \
                         | 1 << POTATURA_LEZIONI_OBBLIGATORIE)

// Prepara quello che serve alle potature (per ogni classe, i suoi docenti e le sue lezioni): una volta, dopo
// nuova_ricerca. libera_potature quando non serve piu'.
void prepara_potature(Ricerca *r);
void libera_potature(Ricerca *r);

// Con le celle fino a pos - 1 decise: -1 se tutte le potature accese passano, se no quale non passa (la prima).
int potatura(const Ricerca *r, int pos);

// Perche' una potatura non passa: i numeri del primo conto che non torna. Quello che non c'entra vale -1.
//   1: al docente restano `servono` ore nella classe, ne puo' fare ancora `possibili`;
//   2: al docente restano `servono` ore in tutto, ha ancora `possibili` ore libere;
//   3: le classi del docente gli prendono ancora `servono` giorni in una classe, dopo oggi ne ha `possibili`;
//   4: alla classe restano `servono` ore, puo' avere ancora `possibili` lezioni;
//   5: la classe ha ancora `servono` celle da riempire fino all'ora `ora` di ogni giorno, le lezioni possibili in
//      quelle ore sono `possibili`;
//   6: all'ora `ora` servono `servono` docenti (uno per classe), ce ne sono `possibili`.
typedef struct {
    int quale;            // la potatura (POTATURA_...)
    int docente, classe;  // di chi si parla
    int ora;
    int servono, possibili;
} Scatto;

// Come potatura, per chi vuole mostrare il perche' (passo.c): rifa' tutti i conti, non solo quelli cambiati (il
// risultato e' lo stesso), e se una potatura non passa mette i suoi numeri in perche.
int potatura_spiegata(const Ricerca *r, int pos, Scatto *perche);

#endif //UNTITLED_POTATURE_H
