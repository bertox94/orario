//
// Created by Alberto Tiraboschi on 20/06/2026.
//

#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H

#define MAX_STR_SZ_IN_DISPLAY 18
#include <_stdio.h>

#include "../prio_queue.h"

void print(const int orario[], const int pos, unsigned long long attempt, FILE *f);
void print_priority_queue(QueueItem *queue[]);
#endif //UNTITLED_PRINTER_H
