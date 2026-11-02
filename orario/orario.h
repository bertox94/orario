#ifndef UNTITLED_ORARIO_H
#define UNTITLED_ORARIO_H

// L'orario: un vettore di slot (giorno, ora, classe) in questo ordine, e le funzioni per muoversi al suo interno.

#include "../data_initialization/database_initializers.h"

extern teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
extern int orario_len;

// L'ordine delle classi e' lo stesso in ogni (giorno, ora): basta ricordare, per ogni classe, in che posizione
// sta dentro il blocco di un'ora. Per questo indice_slot ha dimensione CLASSE_SEZIONE_SZ e non una per giorno e ora.
extern int indice_slot[CLASSE_SEZIONE_SZ];

// Dalla tripla (giorno, ora, classe) all'indice dello slot in orario[].
// La classe di uno slot NON si ricava dalla sua posizione ma si legge dallo slot (orario[i].classe):
// l'ordine delle classi puo' essere cambiato da randomizza_ordine_classi.
static inline int slot_di(const int giorno, const int ora, const int classe) {
    return (giorno * actual_ore_giornaliere + ora) * actual_classi_sezione + indice_slot[classe];
}

// Slot dell'ora precedente (ora - 1) della stessa classe. Richiede ora > 0.
static inline const teaching_slot *slot_precedente(const int giorno, const int ora, const int classe) {
    return &orario[slot_di(giorno, ora - 1, classe)];
}

static inline int docente_nello_slot(const teaching_slot *slot, const int prof) {
    return slot->insegnante1 == prof || slot->insegnante2 == prof;
}

// Ogni slot conosce la propria posizione (giorno, ora, classe) fin dall'inizio; i docenti sono -1 = non assegnato.
// Va chiamata dopo che i dati sono stati inizializzati.
void inizializza_orario(void);

// OPZIONALE. Mescola l'ordine delle classi (unico per ogni giorno e ora). Stesso seed = stesso ordine.
// Da chiamare dopo inizializza_orario().
void randomizza_ordine_classi(unsigned int seed);

#endif //UNTITLED_ORARIO_H
