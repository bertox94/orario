//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#include "prio_queue.h"

// a viene prima di b: ha piu' ore da fare, o le stesse e l'indice minore
static int viene_prima(const QueueItem *a, const QueueItem *b) {
    if (a->priority != b->priority) return a->priority > b->priority;
    return a->teacher < b->teacher;
}

// Ordinamento per inserimento: ogni elemento scende verso l'inizio finche' trova uno che viene prima di lui. La coda
// di una cella e' gia' quasi in ordine (dall'ultima volta che la ricerca e' passata di li' sono cambiate le ore di
// pochi docenti), quindi quasi tutti restano dove sono: un confronto per elemento. L'ordine che esce e' lo stesso
// di qualunque altro ordinamento, perche' due docenti diversi non sono mai «uguali».
void sort_queue(QueueItem **coda, const int n) {
    for (int i = 1; i < n; i++) {
        QueueItem *x = coda[i];
        int j = i - 1;
        for (; j >= 0 && viene_prima(x, coda[j]); j--) coda[j + 1] = coda[j];
        coda[j + 1] = x;
    }
}
