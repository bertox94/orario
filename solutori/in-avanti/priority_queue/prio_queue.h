//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRIO_QUEUE_H
#define UNTITLED_PRIO_QUEUE_H

typedef struct el {
    int teacher;
    double priority;
} QueueItem, *elp;

extern QueueItem **queue_mul;
extern int queue_sz;

void add_to_queue(int prof, int priority);

#endif //UNTITLED_PRIO_QUEUE_H
