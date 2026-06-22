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

void draw_horizontal_separator(FILE *f) {
    f = f ? f : stdout;
    fprintf(f, "---+");
    for (int i = 0; i < actual_giorni_settimana; i++) {
        fprintf(f, "---------------------------+");
    }
    fprintf(f, "\n");
}

// Helper 1: Prints the header block containing the current time and attempt number
void print_header(unsigned long long attempt, FILE *f) {
    f = f ? f : stdout;
    time_t current_time;
    time(&current_time);
    fprintf(f, "Current time: %s", ctime(&current_time));
    fprintf(f, "Orario n. %llu\n", attempt);
}

// Helper 2: Prints the days of the week header row for a specific class
void print_days_header(const int pos, int class_index, FILE *f) {
    f = f ? f : stdout;
    fprintf(f, "\nClasse: %s, ore: %d\n", classe_sezione[class_index], ore_classi[class_index]);
    fprintf(f, "   |");

    for (int i = 0; i < actual_giorni_settimana; i++) {
        fprintf(f, "            %s            |", giorni_settimana[i]);
    }
    fprintf(f, "\n");
}

void print_row_prof(const int orario[], int pos, char ora[], int c, int o, FILE *f) {
    f = f ? f : stdout;
    fprintf(f, " %s|", ora);
    char formatted_str[MAX_STR_SZ_IN_DISPLAY];
    for (int g = 0; g < actual_giorni_settimana; g++) {
        int curr;
        int p = g * actual_ore_giornaliere * actual_classi_sezione + o * actual_classi_sezione + c;
        if (p >= pos) curr = -1;
        else curr = orario[p];
        copy_and_pad(formatted_str, curr == -1 ? "       --        " : insegnanti[curr],
                     MAX_STR_SZ_IN_DISPLAY);
        fprintf(f, "     %s     |", formatted_str);
    }
    fprintf(f, "\n");
}

// Main Function: Clean, descriptive, and easy to read
void print(const int orario[], const int pos, unsigned long long attempt, FILE *f) {
    f = f ? f : stdout;
    print_header(attempt, f);

    for (int c = 0; c < actual_classi_sezione; c++) {
        print_days_header(pos, c, f);
        draw_horizontal_separator(f);

        for (int o = 0; o < actual_ore_giornaliere; o++) {
            // Print the row label (the specific hour slot)
            print_row_prof(orario, pos, ore_giornaliere[o], c, o, f);

            draw_horizontal_separator(f);
        }
    }
    fprintf(f, "\n");
}

void print_priority_queue(QueueItem queue[]) {
    for (int i = 0; i < actual_insegnanti; i++) {
        if (i == 0) printf("Head: ");
        else if (i == actual_insegnanti - 1) printf("Tail: ");
        else printf("       ");
        char str[MAX_STR_SZ_IN_DISPLAY];
        copy_and_pad(str, insegnanti[queue[i].teacher], MAX_STR_SZ_IN_DISPLAY);
        printf("%d)\t%s\t: %.2f\n", i, str, queue[i].priority);
    }
    printf("------------\n");
}

void stampa_dati() {
    printf("Giorni settimana: ");
    for (int i = 0; i < actual_giorni_settimana - 1; i++)
        printf("%s, ", giorni_settimana[i]);
    if (actual_giorni_settimana - 1 > 0)
        printf("%s\n", giorni_settimana[actual_giorni_settimana - 1]);
    printf("Ore giornaliere: ");
    for (int i = 0; i < actual_ore_giornaliere - 1; i++)
        printf("%s, ", ore_giornaliere[i]);
    if (actual_ore_giornaliere - 1 > 0)
        printf("%s\n", ore_giornaliere[actual_ore_giornaliere - 1]);
    printf("Ore classe:\n");
    for (int i = 0; i < actual_classi_sezione; i++)
        printf("Classe %s:\t%d ore\n", classe_sezione[i], ore_classi[i]);
    printf("-----------\n");
    printf("Ore insegnanti:\n");
    for (int i = 0; i < actual_insegnanti; i++) {
        char str[MAX_STR_SZ_IN_DISPLAY];
        copy_and_pad(str, insegnanti[i], MAX_STR_SZ_IN_DISPLAY);
        printf("Ins. %s: %d ore\n", str, ore_docenti[i]);
    }
    printf("-----------\n");
    printf("Ore insegnanti in classe:\n");
    for (int i = 0; i < actual_classi_sezione; i++) {
        for (int j = 0; j < actual_insegnanti; j++) {
            char str[MAX_STR_SZ_IN_DISPLAY];
            copy_and_pad(str, insegnanti[j], MAX_STR_SZ_IN_DISPLAY);
            if (get_ore_classi_insegnante(i, j) > 0)
                printf("In %s\t%s\tha %d ore\n", classe_sezione[i], str, get_ore_classi_insegnante(i, j));
        }
        printf("\n");
    }
    printf("-----------\n");
}
