#ifndef UNTITLED_PROVE_H
#define UNTITLED_PROVE_H

// Le prove con i dati casuali: ./untitled test N.
//
// Un test per ogni seme da 0 a N, per ogni numero di classi da PROVE_CLASSI_MIN a PROVE_CLASSI_MIN + N, senza e con
// ritardi: (N + 1) * (N + 1) * 2 test. Ogni test cerca un orario e si ferma a PROVE_LIMITE_NODI nodi. Su stdout solo
// i test che arrivano al limite, uno per riga; nessun file.

#define PROVE_CLASSI_MIN 3
#define PROVE_LIMITE_NODI 10000000LL

void esegui_prove(int n);

#endif //UNTITLED_PROVE_H
