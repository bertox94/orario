#ifndef UNTITLED_PROVE_H
#define UNTITLED_PROVE_H

// Le prove con i dati casuali: ./untitled test N [ORDINI LIMITE]
//
// Un test per ogni seme da 0 a N, per ogni numero di classi da PROVE_CLASSI_MIN a PROVE_CLASSI_MIN + N, senza e con
// ritardi, e per ogni ordine delle classi da 0 a ORDINI - 1 (generatore.h): (N + 1) * (N + 1) * 2 * ORDINI test. Ogni
// test cerca un orario e si ferma a LIMITE nodi. Su stdout solo i test che arrivano al limite, uno per riga, per
// esempio «seme 0, 5 classi, senza ritardi: ...» (con «, ordine K» se l'ordine non e' 0); nessun file. I test con i
// dati che non passano la verifica (verifica_dati.h) si saltano: la ricerca non parte.
// Senza ORDINI e LIMITE: un ordine solo (quello generato) e PROVE_LIMITE_NODI nodi.

#define PROVE_CLASSI_MIN 3
#define PROVE_LIMITE_NODI 10000000LL

void esegui_prove(int n, int ordini, long long int limite);

#endif //UNTITLED_PROVE_H
