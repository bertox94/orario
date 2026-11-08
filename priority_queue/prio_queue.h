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
    double priority;
} QueueItem;

// queue_mul: per ogni cella (giorno, ora, classe) un blocco di INSEGNANTE_SZ puntatori agli stessi elementi. Ogni cella
// ordina il suo blocco quando la ricerca ci arriva, e l'ordine resta li' fino alla volta dopo (a parita' di priorita'
// conta l'ordine di prima): per questo ogni cella ha la sua copia dei puntatori. queue_sz: quanti docenti in coda.
extern QueueItem **queue_mul;
extern int queue_sz;

// Il docente prof ha `priority` ore in piu': lo aggiunge in coda (in tutti i blocchi) se non c'e', se no aumenta la
// sua priorita'.
void add_to_queue(int prof, int priority);

#endif //UNTITLED_PRIO_QUEUE_H
