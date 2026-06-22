//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#include "prio_queue.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "data_initialization/database_initializers.h"
/*
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

elp peek(elp priority_queue, int pos) {
    assert(pos < actual_insegnanti);
    assert(pos>=0);
    elp curr = priority_queue;
    while (pos > 0 && curr && curr->next)
        curr = curr->next, pos--;
    return curr;
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
*/
/*
void set_priority(elp *head, elp *tail, int giorno, int docente) {
    elp curr = *head;

    // 1. Find and update the node's priority
    while (curr) {
        assert(ore_docenti[docente]>0);
        if (curr->teacher == docente) {
            curr->priority = ore_docenti[docente] - ore_docenti_svolte[docente];
            break; // Target found and updated
        }
        curr = curr->next;
    }

    // If node wasn't found, nothing to do
    if (!curr) {
        return;
    }

    // 2. Detach curr from its current position
    if (curr->prev) {
        curr->prev->next = curr->next;
    } else {
        *head = curr->next; // curr was the head
    }

    if (curr->next) {
        curr->next->prev = curr->prev;
    } else {
        *tail = curr->prev; // curr was the tail
    }

    // 3. Find the correct insertion spot from the beginning
    // We assume the list is sorted in descending order (highest priority first)
    elp scan = *head;
    elp prev_scan = NULL;

    while (scan && scan->priority >= curr->priority) {
        prev_scan = scan;
        scan = scan->next;
    }

    // 4. Insert curr into its new position (between prev_scan and scan)
    curr->next = scan;
    curr->prev = prev_scan;

    if (prev_scan) {
        prev_scan->next = curr;
    } else {
        *head = curr; // New highest priority element (new head)
    }

    if (scan) {
        scan->prev = curr;
    } else {
        *tail = curr; // New lowest priority element (new tail)
    }
}
*/
/*
// Merges two subarrays of queue[].
// First subarray is queue[l..m]
// Second subarray is queue[m+1..r]
void merge(QueueItem queue[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Create temporary arrays
    QueueItem *L = (QueueItem *) malloc(n1 * sizeof(QueueItem));
    QueueItem *R = (QueueItem *) malloc(n2 * sizeof(QueueItem));

    // Copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++)
        L[i] = queue[l + i];
    for (j = 0; j < n2; j++)
        R[j] = queue[m + 1 + j];

    // Merge the temp arrays back into queue[l..r]
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = l; // Initial index of merged subarray

    while (i < n1 && j < n2) {
        // Crucial Change: Use >= for descending order
        if (L[i].priority >= R[j].priority) {
            queue[k] = L[i];
            i++;
        } else {
            queue[k] = R[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of L[], if there are any
    while (i < n1) {
        queue[k] = L[i];
        i++;
        k++;
    }

    // Copy the remaining elements of R[], if there are any
    while (j < n2) {
        queue[k] = R[j];
        j++;
        k++;
    }

    // Free allocated memory
    free(L);
    free(R);
}

// l is for left index and r is right index of the sub-array of queue to be sorted
void mergeSort(int l, int r) {
    if (l < r) {
        // Same as (l+r)/2, but avoids overflow for large l and h
        int m = l + (r - l) / 2;

        // Sort first and second halves
        mergeSort(l, m);
        mergeSort(m + 1, r);

        merge(queue, l, m, r);
    }
}

// Helper function to swap two QueueItem structures
void swap(QueueItem *a, QueueItem *b) {
    QueueItem t = *a;
    *a = *b;
    *b = t;
}

// Partition function based on QueueItem.priority
int partition(QueueItem arr[], int l, int r) {
    int pivot = arr[r].priority;
    int i = l;
    for (int j = l; j <= r - 1; j++) {
        if (arr[j].priority <= pivot) {
            swap(&arr[i], &arr[j]);
            i++;
        }
    }
    swap(&arr[i], &arr[r]);
    return i;
}

// QuickSelect for QueueItem array (0-indexed)
// 'n' is 0 for 1st min, 1 for 2nd min, etc.
QueueItem quickSelect(int l, int r, int n, int *success) {
    // Safety check: ensure n falls within the current subarray bounds
    if (n >= 0 && n <= r - l) {
        int index = partition(queue, l, r);

        // Convert the pivot's absolute index to a relative 0-based position
        int relative_position = index - l;

        // Found the target element
        if (relative_position == n) {
            *success = 1;
            return queue[index];
        }

        // Target is in the left partition
        if (relative_position > n) {
            return quickSelect(l, index - 1, n, success);
        }

        // Target is in the right partition (adjust n relative to the new subarray)
        return quickSelect(index + 1, r, n - (relative_position + 1), success);
    }

    *success = 0;
    QueueItem empty = {0, 0};
    return empty;
}

head_tail_queue initialize_queue(const int num, const int maxelems) {
    if (num < maxelems) {
        // S -> aS
        const elp curr = malloc(sizeof(QueueItem));
        curr->prev = NULL;
        curr->teacher = num;
        head_tail_queue ret = initialize_queue(num + 1, maxelems);
        if (ret.head != NULL)
            ret.head->prev = curr;
        curr->next = ret.head;
        curr->priority = (double) ore_docenti[num];
        return (head_tail_queue){curr, ret.tail == NULL ? curr : ret.tail};
    }
    // S -> eps
    return (head_tail_queue){NULL, NULL};
}

head_tail_queue copy_list(const elp curr) {
    if (curr != NULL) {
        elp copy = malloc(sizeof(QueueItem));
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
head_tail_queue initialize_priority_queue(const int profs) {
    const head_tail_queue ret = initialize_queue(0, profs);
    return (head_tail_queue){ret.head, ret.tail};
}
*/

void initialize_queue_arr(QueueItem queue[]) {
    for (int i = 0; i < actual_insegnanti; i++) {
        QueueItem newItem = {.teacher = i, .priority = ore_docenti[i]};

        // Find the correct position for newItem in the already initialized part of the array
        int j = i - 1;

        // Shift elements with smaller priorities to the right to make room
        while (j >= 0 && queue[j].priority < newItem.priority) {
            queue[j + 1] = queue[j];
            j--;
        }

        // Place the item into its correct descending position
        queue[j + 1] = newItem;
    }
}