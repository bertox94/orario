#include "../printer/printer.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../data_initialization/database_initializers.h"
#include "../prio_queue.h"
//
// Created by Alberto Tiraboschi on 20/06/2026.
//
void copy_and_pad(char *dest, const char *src, int maxlen) {
    int src_len = strlen(src);
    int copy_len = src_len;

    // 1. Determine how much to copy (leave 1 slot for the null terminator)
    if (copy_len > maxlen - 1) {
        copy_len = maxlen - 1;
    }

    // 2. Copy the actual string characters
    memcpy(dest, src, copy_len);

    // 3. Fill the remaining space with blank spaces ' '
    for (int i = copy_len; i < maxlen - 1; i++) {
        dest[i] = ' ';
    }

    // 4. Always force a null terminator at the very end
    dest[maxlen - 1] = '\0';
}

void draw_horizontal_separator(int pos) {
    printf("---+");
    for (int i = 0; i < actual_giorni_settimana; i++) {
        printf("---------------------------+");
    }
    printf("\n");
}

// Helper 1: Prints the header block containing the current time and attempt number
void print_header(unsigned long long attempt) {
    time_t current_time;
    time(&current_time);

    printf("Current time: %s", ctime(&current_time));
    printf("Orario n. %llu\n", attempt);
}

// Helper 2: Prints the days of the week header row for a specific class
void print_days_header(const int pos, int class_index) {
    printf("\nClasse: %s, ore: %d\n", classe_sezione[class_index], ore_classi[class_index]);
    printf("   |");

    for (int i = 0; i < actual_giorni_settimana; i++) {
        printf("            %s            |", giorni_settimana[i]);
    }
    printf("\n");
}

void print_row_prof(const int orario[], int pos, char ora[], int c, int o) {
    printf(" %s|", ora);
    char formatted_str[MAX_STR_SZ_IN_DISPLAY];
    for (int g = 0; g < actual_giorni_settimana; g++) {
        int curr;
        int p = g * actual_ore_giornaliere * actual_classi_sezione + o * actual_classi_sezione + c;
        if (p >= pos) curr = -1;
        else curr = orario[p];
        copy_and_pad(formatted_str, curr == -1 ? "       --        " : insegnanti[curr],
                     MAX_STR_SZ_IN_DISPLAY);
        printf("     %s     |", formatted_str);
    }
    printf("\n");
}

// Main Function: Clean, descriptive, and easy to read
void print(const int orario[], const int pos, unsigned long long attempt) {
    print_header(attempt);

    for (int c = 0; c < actual_classi_sezione; c++) {
        print_days_header(pos, c);
        draw_horizontal_separator(pos);

        for (int o = 0; o < actual_ore_giornaliere; o++) {
            // Print the row label (the specific hour slot)
            print_row_prof(orario, pos, ore_giornaliere[o], c, o);

            draw_horizontal_separator(pos);
        }
    }
    printf("\n");
}
/*
void print_prioritt_queue(elp priority_queue, elp tail) {
    for (elp i = priority_queue; i != NULL; i = i->next) {
        if (i == priority_queue) printf("Head: ");
        else if (i == tail) printf("Tail: ");
        else printf("       ");
        printf("%p: %s\t priority: %.2f\n", i, insegnanti[i->teacher], i->priority);
    }
    printf("------------\n");
}
*/
void print_priority_queue() {
    for (int i = 0; i < actual_insegnanti; i++) {
        if (i == 0) printf("Head: ");
        else if (i == actual_insegnanti - 1) printf("Tail: ");
        else printf("       ");
        printf("%d)\t%s\t priority: %.2f\n", i, insegnanti[queue[i].teacher], queue[i].priority);
    }
    printf("------------\n");
}
