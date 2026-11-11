#ifndef UNTITLED_VERIFICA_DATI_H
#define UNTITLED_VERIFICA_DATI_H

#include <stddef.h>

// La verifica dei dati, prima della ricerca: condizioni necessarie che si vedono dai dati senza cercare. Se una non
// vale l'orario non esiste e la ricerca non parte («errore nei dati»); se valgono tutte non e' garantito niente.
// Non tocca la ricerca: legge soltanto i dati.
//
// G giorni, O ore al giorno, M ore obbligatorie (PRIME_N_ORE_OBBLIGATORIE); un docente fa al massimo 3 ore al giorno
// in una classe e insegna in al massimo 3 classi al giorno; chi entra all'ora k ha O - k ore al giorno.
// I controlli, nell'ordine in cui si fanno (il perche' di ognuno e' in verifica_dati.c):
//  1. docente in una classe: ore <= G x min(3, O - k)
//  2. docente in tutto: ore <= G x (O - k)
//  3. docente, classi al giorno: una classe con n ore gli prende almeno n / 3 giorni (arrotondato in su); la somma
//     su tutte le sue classi <= 3 x G
//  4. classe, ore obbligatorie: per ogni ora h < M, le lezioni possibili dalla prima ora fino ad h (per ogni docente
//     da solo e per ogni coppia: dal loro ingresso, al massimo 3 al giorno, non piu' delle loro ore) sono almeno le
//     (h + 1) x G celle da riempire
//  5. ora per ora: a ogni ora obbligatoria i docenti gia' entrati sono almeno quante le classi
//
// Chi la chiama: main.c per i dati di prova (stampa «Errore nei dati: ...») e caso.c per i casi del sito (esito
// «errore nei dati», con il motivo nel campo «errore» dello stato). Le prove (./untitled test N) saltano i casi
// che non la passano.
//
// 1 se i dati passano tutti i controlli; se no 0, con il primo che non vale spiegato in errore.
// (Il motivo puo' avere piu' righe: la frase e, sotto, i numeri. VERIFICA_DATI_ERRORE e' lo spazio che serve.)
#define VERIFICA_DATI_ERRORE 4096
int verifica_dati(char *errore, size_t dimensione);

#endif //UNTITLED_VERIFICA_DATI_H
