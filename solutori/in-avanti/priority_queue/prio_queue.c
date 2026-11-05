//
// Created by Alberto Tiraboschi on 20/06/2026.
//


#include "prio_queue.h"
#include "../data_initialization/database_initializers.h"

#include <stdlib.h>


QueueItem **queue_mul;
int queue_sz;


void add_to_queue(int prof, int priority) {
    for (int i = 0; i < queue_sz; i++) {
        if (queue_mul[i]->teacher == prof) {
            queue_mul[i]->priority += priority;
            return;
        }
    }
    QueueItem *new = malloc(sizeof(QueueItem));
    new->teacher = prof;
    new->priority = priority;
    queue_mul[queue_sz++] = new;
    for (int i = 1; i < GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ; i++) {
        queue_mul[i * INSEGNANTE_SZ + queue_sz - 1] = queue_mul[queue_sz - 1];
    }
}
