//
// Created by Alberto Tiraboschi on 20/06/2026.
//


#include "prio_queue.h"
#include "../data_initialization/database_initializers.h"

#include <stdlib.h>


QueueItem **queue_mul;
int queue_sz;


// vedi prio_queue.h: il blocco della cella 0 e' l'elenco dei docenti; un docente nuovo si copia in tutti i blocchi
void add_to_queue(int prof, int priority) {
    for (int i = 0; i < queue_sz; i++) {
        if (queue_mul[i]->teacher == prof) {
            queue_mul[i]->priority += priority;
            return;
        }
    }
    if (!queue_mul)  // la prima volta: un blocco di INSEGNANTE_SZ puntatori per ogni cella
        queue_mul = malloc(sizeof(QueueItem *) * GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ *
                           INSEGNANTE_SZ);
    QueueItem *new = malloc(sizeof(QueueItem));
    new->teacher = prof;
    new->priority = priority;
    queue_mul[queue_sz++] = new;
    for (int i = 1; i < GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ; i++) {
        queue_mul[i * INSEGNANTE_SZ + queue_sz - 1] = queue_mul[queue_sz - 1];
    }
}
