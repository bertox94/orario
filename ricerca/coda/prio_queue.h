//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRIO_QUEUE_H
#define UNTITLED_PRIO_QUEUE_H

// La coda dei docenti: un elemento per docente, con la sua priorita' (le ore che gli restano da mettere: cala di uno
// per ogni ora messa). L'ordine in cui la ricerca prova i docenti in una cella e' quello della coda, dalla priorita'
// piu' alta.
typedef struct el {
    int teacher;
    int priority;
} QueueItem;

// Ordina n puntatori a elementi della coda: dalla priorita' piu' alta (piu' ore da fare) alla piu' bassa; a parita'
// prima il docente con l'indice minore, cosi' l'ordine e' uno solo e non dipende da come si ordina.
void sort_queue(QueueItem **coda, int n);

#endif //UNTITLED_PRIO_QUEUE_H
