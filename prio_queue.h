//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRIO_QUEUE_H
#define UNTITLED_PRIO_QUEUE_H
#include "data_initialization/database_initializers.h"

typedef struct el {
    int teacher;
    double priority;
} QueueItem, *elp;

extern QueueItem **queue_mul;

void initialize_queue_arr(QueueItem *queue[]);
#endif //UNTITLED_PRIO_QUEUE_H
