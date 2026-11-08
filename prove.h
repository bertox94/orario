#ifndef UNTITLED_PROVE_H
#define UNTITLED_PROVE_H

// Le prove con i dati casuali (./untitled test N, oppure ./untitled test e il numero lo chiede).
//
// Con N: un test per ogni seme da 0 a N, per ogni numero di classi da PROVE_CLASSI_MIN a PROVE_CLASSI_MIN + N, prima
// senza e poi con i ritardi: (N + 1) * (N + 1) * 2 test. Per esempio con 2: seme 0 con 8 classi senza e con ritardi,
// seme 0 con 9 classi, seme 0 con 10 classi, poi seme 1... fino al seme 2 con 10 classi.
//
// Ogni test cerca un orario e si ferma a PROVE_LIMITE_NODI nodi (100.000: con di piu' l'ultimo backtrack di un test
// fermo al limite e' quasi tutta la ricerca, e ultimo-backtrack.txt arriva a GB). I file, nella cartella corrente:
// - report.txt: una riga per test (seme, classi, ritardi, esito, nodi, cella piu' avanti raggiunta, secondi);
// - passi.txt: per i test che arrivano al limite di nodi, i primi PROVE_PASSI nodi, una riga per nodo (la cella appena
//   decisa e la scelta: dalla riga 1 a una riga k si ricostruisce l'orario al nodo k);
// - ultimo-backtrack.txt: per ogni test l'ultimo backtrack: dal nodo dove la ricerca si e' bloccata l'ultima volta,
//   ogni nodo (una riga come in passi.txt) fino a quando va oltre quella cella, o fino alla fine del test.
// Ogni test gira in un processo a parte (fork): i dati di un test non restano nel test dopo.

#define PROVE_CLASSI_MIN 8
#define PROVE_LIMITE_NODI 100000LL
#define PROVE_PASSI 1000000LL

// Ritorna 0 se tutte le prove sono state fatte.
int esegui_prove(int n);

#endif //UNTITLED_PROVE_H
