#ifndef UNTITLED_GENERATORE_H
#define UNTITLED_GENERATORE_H

#include "database_initializers.h"

// Dati di prova casuali, riproducibili da un seme: gli stessi di sito/generatore.py (stesso algoritmo e stessi numeri
// casuali, xorshift64*), quindi gli stessi casi del sito per ogni seme.
//
// Ogni classe ha da 30 a 36 ore, divise in lezioni da 1 a 6 ore; una parte delle lezioni (tra il 15% e il 60%, a ogni
// tentativo) ha anche un codocente per alcune di quelle ore. Il numero di docenti si sceglie perche' in media abbiano
// tra 10 e 20 ore; ogni lezione va al meno carico fra tre docenti presi a caso. Se un docente resta con meno di 5 ore
// o ne avrebbe piu' di 24, o se mancano lezioni da soli o in coppia, si riprova da capo.
//
// Con i ritardi, il seme decide quanti docenti entrano tardi (da uno a tutti) e quali; ognuno entra alla seconda,
// terza o quarta ora, abbassata finche' le sue ore stanno ancora nelle ore che gli restano nella settimana. Con e senza
// ritardi i dati di un seme sono gli stessi, tranne le ore di ingresso. Un caso con i ritardi puo' essere impossibile.
//
// L'ordine delle classi (ordine): le stesse classi e le stesse lezioni, con le classi in un altro ordine nei dati. La
// ricerca numera le classi in quell'ordine e dentro ogni ora le decide in quell'ordine (anche i docenti si numerano
// in ordine di comparsa, quindi cambia anche il loro ordine a parita' di ore): la strada della ricerca cambia. Ordine
// 0: come generato; gli altri sono rimescolamenti riproducibili dal loro numero, gli stessi di sito/generatore.py.
//
// Le classi si chiamano C01, C02...; i docenti con i nomi di nomi_docenti.h (oltre l'elenco D346, D347...).
// Registra nella scuola s giorni, ore, lezioni e ore di ingresso. Ritorna 0 se non riesce a generare i dati.
int genera_dati_casuali(Scuola *s, unsigned long long int seme, int n_classi, int con_ritardi, int ordine);

// I dati del caso in JSON su stdout (./untitled dati, per il sito): giorni, ore, lezioni e ore di ingresso. Ritorna 0
// se non riesce a generare i dati.
int stampa_dati_casuali(unsigned long long int seme, int n_classi, int con_ritardi, int ordine);

#endif //UNTITLED_GENERATORE_H
