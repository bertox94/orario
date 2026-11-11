//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#include "prio_queue.h"

#include <stdlib.h>

static int compare_queue_items(const void *a, const void *b) {
    const QueueItem *itemA = *(const QueueItem **) a;
    const QueueItem *itemB = *(const QueueItem **) b;
    if (itemA->priority != itemB->priority) return itemA->priority < itemB->priority ? 1 : -1;
    return (itemA->teacher > itemB->teacher) - (itemA->teacher < itemB->teacher);
}

void sort_queue(QueueItem **coda, const int n) {
    qsort(coda, n, sizeof(QueueItem *), compare_queue_items);
}
