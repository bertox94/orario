//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#include "prio_queue.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "data_initialization/database_initializers.h"

int pop(int pos, elp *priority_queue, elp *tail) {
    assert(pos < actual_insegnanti);

    elp curr = *priority_queue;
    while (pos > 0)
        curr = curr->next, pos--;

    if (curr == *tail) return curr->teacher;

    if (curr == *priority_queue) {
        *priority_queue = (*priority_queue)->next;
    } else {
        curr->prev->next = curr->next;
    }
    curr->next->prev = curr->prev; // OK also if curr was the head, it just copies NULL
    curr->next = NULL;
    curr->prev = *tail;
    (*tail)->next = curr;
    *tail = curr;

    return curr->teacher;
}

int peek(elp priority_queue, int pos) {
    assert(pos < actual_insegnanti);
    assert(pos>=0);
    elp curr = priority_queue;
    while (pos > 0 && curr && curr->next)
        curr = curr->next, pos--;
    return curr->teacher;
}

// Helper: Splits a doubly linked list into two halves
static elp split_list(elp head) {
    elp fast = head;
    elp slow = head;

    while (fast->next && fast->next->next) {
        fast = fast->next->next;
        slow = slow->next;
    }

    elp temp = slow->next;
    slow->next = NULL;
    if (temp) {
        temp->prev = NULL;
    }
    return temp;
}

// Helper: Merges two sorted doubly linked lists in descending order
static elp merge_descending(elp first, elp second) {
    // Base cases
    if (!first) return second;
    if (!second) return first;

    // Pick the larger priority for descending order
    if (first->priority >= second->priority) {
        first->next = merge_descending(first->next, second);
        if (first->next) {
            first->next->prev = first;
        }
        first->prev = NULL;
        return first;
    } else {
        second->next = merge_descending(first, second->next);
        if (second->next) {
            second->next->prev = second;
        }
        second->prev = NULL;
        return second;
    }
}

// Helper: Recursive merge sort wrapper for node pointers
static elp merge_sort_rec(elp head) {
    if (!head || !head->next) {
        return head;
    }

    elp second = split_list(head);

    // Recur for left and right halves
    head = merge_sort_rec(head);
    second = merge_sort_rec(second);

    // Merge the two sorted halves
    return merge_descending(head, second);
}

// PUBLIC FUNCTION: Sorts the head_tail_queue in-place (Descending)
void sort_queue_descending(head_tail_queue *q) {
    if (q->head == NULL || q->head == q->tail) {
        return; // Empty or single element list is already sorted
    }

    // 1. Perform the merge sort on the nodes
    q->head = merge_sort_rec(q->head);

    // 2. Fix the tail pointer by walking to the end of the sorted list
    elp curr = q->head;
    while (curr && curr->next) {
        curr = curr->next;
    }
    q->tail = curr;
}

void set_priority(elp curr, int giorno, int cla, int docente) {
    while (curr && curr->next) {
        if (curr->teacher == docente && ore_classi[cla] - ore_classe_svolte[cla] > 0) {
            curr->priority =
                    (double) (ore_classi_docenti[cla * INSEGNANTE_SZ + docente] - ore_classi_docenti_svolte[
                                  cla * INSEGNANTE_SZ + docente]) / (ore_classi[cla] - ore_classe_svolte[cla]);
            break;
        }
        curr = curr->next;
    }
}

head_tail_queue initialize_queue(int prof, const int class) {
    while (prof < actual_insegnanti && ore_classi_docenti[class * INSEGNANTE_SZ + prof] == 0) prof++;
    if (prof == actual_insegnanti) return (head_tail_queue){NULL, NULL};

    const elp curr = malloc(sizeof(el));
    curr->prev = NULL;
    curr->teacher = prof;
    head_tail_queue ret = initialize_queue(prof + 1, class);
    if (ret.head != NULL)
        ret.head->prev = curr;
    curr->next = ret.head;
    curr->priority = (double) ore_classi_docenti[class * INSEGNANTE_SZ + prof] / ore_classi[class];
    return (head_tail_queue){curr, ret.tail == NULL ? curr : ret.tail};
}

head_tail_queue copy_list(const elp curr) {
    if (curr != NULL) {
        elp copy = malloc(sizeof(el));
        copy->prev = NULL;
        copy->teacher = curr->teacher;
        const head_tail_queue ret = copy_list(curr->next);
        if (ret.head != NULL)
            ret.head->prev = copy;
        copy->next = ret.head;
        return (head_tail_queue){copy, ret.tail == NULL ? copy : ret.tail};
    }

    if (curr == NULL) {
        return (head_tail_queue){NULL, NULL};
    }
    exit(EXIT_FAILURE);
}

void print_prioritt_queue(elp priority_queue, elp tail) {
    for (elp i = priority_queue; i != NULL; i = i->next) {
        if (i == priority_queue) printf("Head: ");
        else if (i == tail) printf("Tail: ");
        else printf("       ");
        printf("%p: %s\t priority: %.2f\n", i, insegnanti[i->teacher], i->priority);
    }
    printf("------------\n");
}

head_tail_queue initialize_priority_queue(const int cla) {
    const head_tail_queue ret = initialize_queue(0, cla);
    return (head_tail_queue){ret.head, ret.tail};
}
