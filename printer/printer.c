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

    if (copy_len > maxlen - 1) {
        copy_len = maxlen - 1;
    }

    memcpy(dest, src, copy_len);

    for (int i = copy_len; i < maxlen - 1; i++) {
        dest[i] = ' ';
    }

    dest[maxlen - 1] = '\0';
}

void draw_grid_separator(FILE *f) {
    fprintf(f, "----------------+");
    for (int g = 0; g < actual_giorni_settimana; g++) {
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            fprintf(f, "---------------------------+");
        }
    }
    fprintf(f, "\n");
}

// Helper 1: Prints the header block containing the current time and attempt number
void print_header(unsigned long long attempt, FILE *f) {
    time_t current_time;
    time(&current_time);
    fprintf(f, "Current time: %s", ctime(&current_time));
    fprintf(f, "Orari corretti: %llu\n\n", attempt);
}

// Main Function: Prints classes as rows, and days/hours as columns
void print(const teaching_slot orario[], const int pos, unsigned long long attempt, FILE *f) {
    print_header(attempt, f);

    // 1. Print Days header row
    fprintf(f, "                |");
    for (int g = 0; g < actual_giorni_settimana; g++) {
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            char day_str[MAX_STR_SZ_IN_DISPLAY];
            copy_and_pad(day_str, giorni_settimana[g], MAX_STR_SZ_IN_DISPLAY);
            fprintf(f, "     %s     |", day_str);
        }
    }
    fprintf(f, "\n");

    // 2. Print Hours header row (sub-columns)
    fprintf(f, "                |");
    for (int g = 0; g < actual_giorni_settimana; g++) {
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            char hour_str[MAX_STR_SZ_IN_DISPLAY];
            copy_and_pad(hour_str, ore_giornaliere[o], MAX_STR_SZ_IN_DISPLAY);
            fprintf(f, "     %s     |", hour_str);
        }
    }
    fprintf(f, "\n");

    draw_grid_separator(f);

    // 3. Print each class as a row
    for (int c = 0; c < actual_classi_sezione; c++) {
        // Prepare class label (e.g., "Classe: 1A")
        char class_label[MAX_STR_SZ_IN_DISPLAY];
        snprintf(class_label, sizeof(class_label), "%s", classe_sezione[c]);
        char formatted_class[15];
        copy_and_pad(formatted_class, class_label, 15);

        // --- Row for Insegnante 1 ---
        fprintf(f, " %-14s |", formatted_class);
        for (int g = 0; g < actual_giorni_settimana; g++) {
            for (int o = 0; o < actual_ore_giornaliere; o++) {
                int curr;
                int p = g * actual_ore_giornaliere * actual_classi_sezione + o * actual_classi_sezione + c;
                if (p >= pos) curr = -1;
                else curr = orario[p].insegnante1;

                char formatted_str[MAX_STR_SZ_IN_DISPLAY];
                copy_and_pad(formatted_str, curr == -1 ? "       --        " : insegnanti[curr],
                             MAX_STR_SZ_IN_DISPLAY);
                fprintf(f, "     %s     |", formatted_str);
            }
        }
        fprintf(f, "\n");

        // --- Row for Insegnante 2 (Code/Codocenza) ---
        fprintf(f, "                |");
        for (int g = 0; g < actual_giorni_settimana; g++) {
            for (int o = 0; o < actual_ore_giornaliere; o++) {
                int curr;
                int p = g * actual_ore_giornaliere * actual_classi_sezione + o * actual_classi_sezione + c;
                if (p >= pos) curr = -1;
                else curr = orario[p].insegnante2;

                char formatted_str[MAX_STR_SZ_IN_DISPLAY];
                copy_and_pad(formatted_str, curr == -1 ? "       --        " : insegnanti[curr],
                             MAX_STR_SZ_IN_DISPLAY);
                fprintf(f, "     %s     |", formatted_str);
            }
        }
        fprintf(f, "\n");

        draw_grid_separator(f);
    }

    // 4. Print Teacher workload summaries below the grid
    fprintf(f, "\n--- Statistiche Insegnanti per Classe ---\n");
    for (int c = 0; c < actual_classi_sezione; c++) {
        fprintf(f, "\nClasse: %s\n", classe_sezione[c]);
        for (int j = 0; j < actual_insegnanti; j++) {
            if (get_ore_classi_insegnante(c, j) > 0) {
                char str[MAX_STR_SZ_IN_DISPLAY];
                copy_and_pad(str, insegnanti[j], MAX_STR_SZ_IN_DISPLAY);
                int ore_i_s = get_ore_classi_insegnante_svolte(c, j);
                int ore_i = get_ore_classi_insegnante(c, j);
                fprintf(f, "%s\tha svolto %d ore su %d [%d]\n", str, ore_i_s, ore_i, ore_i_s == ore_i);
            }
        }
    }

    fprintf(f, "\n");
    fflush(f);
}
void print_priority_queue(FILE *f, int start, int end) {
    for (int i = start; i < end; i++) {
        if (i == start) fprintf(f,"Head: ");
        else if (i == end - 1) fprintf(f,"Tail: ");
        else fprintf(f,"       ");
        char str[MAX_STR_SZ_IN_DISPLAY];
        copy_and_pad(str, insegnanti[queue_mul[i]->teacher], MAX_STR_SZ_IN_DISPLAY);
        fprintf(f, "%d)\t%s : %.2f\n", i, str,queue_mul[i]->priority);
    }
    fprintf(f,"------------\n");
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
        printf("In %s:\n", classe_sezione[i]);
        for (int j = 0; j < actual_insegnanti; j++) {
            char str[MAX_STR_SZ_IN_DISPLAY];
            copy_and_pad(str, insegnanti[j], MAX_STR_SZ_IN_DISPLAY);
            if (get_ore_classi_insegnante(i, j) > 0)
                printf("%s\tha %d ore\n", str, get_ore_classi_insegnante(i, j));
        }
        printf("\n");
    }
    printf("-----------\n");
}
