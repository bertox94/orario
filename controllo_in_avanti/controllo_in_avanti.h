#ifndef UNTITLED_CONTROLLO_IN_AVANTI_H
#define UNTITLED_CONTROLLO_IN_AVANTI_H

#include <stddef.h>

// Il controllo in avanti e la verifica dei dati (venivano dal solver in-avanti, tolto; li hanno tutti i solver).
//
// CONTROLLO IN AVANTI, dopo ogni scelta (una coppia, un docente da solo o un'ora buca): il resto e' ancora
// risolvibile? Sono condizioni necessarie: se dicono "no" l'orario non si puo' completare e la scelta si scarta; se
// dicono "si'" non e' garantito niente. In ordine:
//  1. le celle di oggi, una per una: ogni cella obbligatoria che resta oggi ha ancora almeno un docente possibile (ha
//     ancora ore nella classe, e' gia' entrato e libero a quell'ora, puo' ancora dare ore oggi nella classe);
//  2. le lezioni delle classi, classe per classe (per voce: un docente da solo o una coppia):
//     a. fino a ogni ora obbligatoria bastano per le celle da riempire, oggi e in ognuno dei giorni che restano;
//     b. fino all'ultima ora di ogni giorno bastano per tutte le ore che restano alla classe;
//  3. le ore dei docenti: in ogni classe le ore che restano al docente stanno in quelle che puo' ancora fare (oggi dopo
//     l'ultima ora decisa, se il suo blocco non si e' chiuso; poi al massimo 3 al giorno dal suo ingresso), e in tutto
//     stanno nelle ore libere che gli restano.
// Quando: all'ingresso della cella dopo la scelta, con le potature (potature/potature.h, POTATURA_IN_AVANTI).
//
// Vale per qualunque ordine delle celle (main: un'ora alla volta; per-classe: dentro il giorno una
// classe alla volta) e dei giorni (giorni-liberi): i solver si decidono un giorno alla volta, e dentro il giorno ogni
// classe le sue ore in ordine di tempo, quindi per ogni classe le celle decise oggi sono le prime ore.
//
// VERIFICA DEI DATI, prima della ricerca: condizioni necessarie che si vedono senza cercare (ore di un docente in una
// classe e in tutto, classi al giorno, lezioni possibili fino a ogni ora obbligatoria, docenti entrati a ogni ora
// obbligatoria). Se una non vale, l'orario non esiste e la ricerca non parte («errore nei dati»).

// Da chiamare una volta, dopo aver letto i dati. cella_di[s]: la cella decisa al passo s, indice_di[cella] il passo
// (NULL tutti e due: la cella s al passo s, come in main).
void prepara_controllo_in_avanti(const int *cella_di, const int *indice_di);
// Per un solver che riempie i giorni in un altro ordine (giorni-liberi): giorno_di_posto[r] il giorno che si riempie
// per r-esimo, posto_di_giorno[g] il suo posto (NULL: calendario). Si puo' cambiare durante la ricerca.
void controllo_in_avanti_ordine_giorni(const int *giorno_di_posto, const int *posto_di_giorno);

// Dopo le prime s celle della ricerca (l'ultima decisa e' la cella del passo s - 1, s > 0): -1 se il resto e' ancora
// possibile, altrimenti una cella che non si potra' riempire, con il motivo in motivo_in_avanti e i numeri in
// numeri_in_avanti.
int controllo_in_avanti(int s);
// 0 una cella di oggi senza docenti, 1 non bastano le lezioni fino a un'ora obbligatoria, 2 non bastano per tutte le
// ore della classe, 3 a un docente non basta il tempo
extern int motivo_in_avanti;
const char *nome_motivo_in_avanti(int motivo);
// i numeri dei motivi 2 e 3: il docente (-1 per 2), la classe (-1: in tutto), quante ore restano, quante possibili
void numeri_in_avanti(int *docente, int *classe, int *restano, int *possibili);

// La verifica dei dati: 1 se tutto va, se no 0 con il motivo in errore.
int verifica_fattibilita(char *errore, size_t dimensione);

#endif
