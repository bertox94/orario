//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRIO_QUEUE_H
#define UNTITLED_PRIO_QUEUE_H

typedef struct el {
    int teacher;
    double priority;
    struct el *prev;
    struct el *next;
} el, *elp;

typedef struct {
    elp head;
    elp tail;
} head_tail_queue;

int pop(int pos, elp *priority_queue, elp *tail);

int peek(elp priority_queue, int pos);

void sort_queue_descending(head_tail_queue *q);

head_tail_queue initialize_priority_queue(const int class);

void print_prioritt_queue(elp priority_queue, elp tail);

head_tail_queue copy_list(const elp curr);

void set_priority(elp priority_queue, int docente, int ore_docenti_svolte[], int giorno);
#endif //UNTITLED_PRIO_QUEUE_H
