//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRIO_QUEUE_H
#define UNTITLED_PRIO_QUEUE_H
#include "data_initialization/database_initializers.h"

typedef struct el {
    int teacher;
    double priority;
    struct el *prev;
    struct el *next;
} QueueItem, *elp;

typedef struct {
    elp head;
    elp tail;
} head_tail_queue;

extern QueueItem queue[INSEGNANTE_SZ];

int pop(int pos, elp *priority_queue, elp *tail);

elp peek(elp priority_queue, int pos);

void sort_queue_descending(head_tail_queue *q);

void mergeSort(int l, int r);

head_tail_queue initialize_priority_queue(const int class);

void print_prioritt_queue(elp priority_queue, elp tail);

void print_priority_queue();

head_tail_queue copy_list(const elp curr);

void set_priority(elp *head, elp *tail, int giorno, int docente);
#endif //UNTITLED_PRIO_QUEUE_H
