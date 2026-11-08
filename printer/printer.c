#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../printer/printer.h"
#include "../data_initialization/database_initializers.h"
//
// Created by Alberto Tiraboschi on 20/06/2026.
//

// La stampa dell'orario in orario.txt (print) e dei dati letti su stdout (stampa_dati).

// src in dest, tagliato o completato con spazi fino a maxlen - 1 caratteri
static void copy_and_pad(char *dest, const char *src, int maxlen) {
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

static void draw_grid_separator(FILE *f) {
    fprintf(f, "----------------+");
    for (int g = 0; g < actual_giorni_settimana; g++) {
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            fprintf(f, "---------------------------+");
        }
    }
    fprintf(f, "\n");
}

// l'intestazione: l'ora e quanti orari completi trovati finora
static void print_header(unsigned long long attempt, FILE *f) {
    time_t current_time;
    time(&current_time);
    fprintf(f, "Current time: %s", ctime(&current_time));
    fprintf(f, "Orari corretti: %llu\n\n", attempt);
}

// Una riga della classe c: il docente (quale = 1) o il codocente (quale = 2) di ogni cella fino a pos (esclusa), con
// in testa l'etichetta (la classe nella prima riga, vuota nella seconda).
static void riga_docenti(FILE *f, const teaching_slot orario[], const int pos, const int c, const char *etichetta,
                         const int quale) {
    if (quale == 1) fprintf(f, " %-14s |", etichetta);
    else fprintf(f, "                |");
    for (int g = 0; g < actual_giorni_settimana; g++) {
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            const int p = g * actual_ore_giornaliere * actual_classi_sezione + o * actual_classi_sezione + c;
            const int curr = p >= pos ? -1 : quale == 1 ? orario[p].insegnante1 : orario[p].insegnante2;
            char formatted_str[MAX_STR_SZ_IN_DISPLAY];
            copy_and_pad(formatted_str, curr == -1 ? "       --        " : insegnanti[curr], MAX_STR_SZ_IN_DISPLAY);
            fprintf(f, "     %s     |", formatted_str);
        }
    }
    fprintf(f, "\n");
}

// le classi in riga, giorni e ore in colonna; poi le ore di ogni docente in ogni classe, le classi una accanto all'altra
void print(const teaching_slot orario[], const int pos, unsigned long long attempt, FILE *f) {
    print_header(attempt, f);

    // 1. i giorni
    fprintf(f, "                |");
    for (int g = 0; g < actual_giorni_settimana; g++) {
        for (int o = 0; o < actual_ore_giornaliere; o++) {
            char day_str[MAX_STR_SZ_IN_DISPLAY];
            copy_and_pad(day_str, giorni_settimana[g], MAX_STR_SZ_IN_DISPLAY);
            fprintf(f, "     %s     |", day_str);
        }
    }
    fprintf(f, "\n");

    // 2. le ore
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

    // 3. una riga per classe (docente e codocente)
    for (int c = 0; c < actual_classi_sezione; c++) {
        char class_label[MAX_STR_SZ_IN_DISPLAY];
        snprintf(class_label, sizeof(class_label), "%s", classe_sezione[c]);
        char formatted_class[15];
        copy_and_pad(formatted_class, class_label, 15);

        riga_docenti(f, orario, pos, c, formatted_class, 1);
        riga_docenti(f, orario, pos, c, "", 2);

        draw_grid_separator(f);
    }

    // 4. per ogni classe, una accanto all'altra, le ore messe / da fare di ogni suo docente ([1]: tutte messe)
    fprintf(f, "\n--- Statistiche Insegnanti per Classe (Side-by-Side) ---\n\n");

    const int COL_WIDTH = 35;

    // i nomi delle classi
    for (int c = 0; c < actual_classi_sezione; c++) {
        char header_buf[64];
        snprintf(header_buf, sizeof(header_buf), "Classe: %s", classe_sezione[c]);
        fprintf(f, "%-*s | ", COL_WIDTH, header_buf);
    }
    fprintf(f, "\n");

    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int i = 0; i < COL_WIDTH; i++) fprintf(f, "-");
        fprintf(f, "-+-");
    }
    fprintf(f, "\n");

    // quante righe: i docenti della classe che ne ha di piu'
    int max_active_teachers = 0;
    for (int c = 0; c < actual_classi_sezione; c++) {
        int count = 0;
        for (int j = 0; j < actual_insegnanti; j++) {
            if (get_ore_classi_insegnante(c, j) > 0) count++;
        }
        if (count > max_active_teachers) max_active_teachers = count;
    }

    // la riga r: l'r-esimo docente di ogni classe
    for (int r = 0; r < max_active_teachers; r++) {
        for (int c = 0; c < actual_classi_sezione; c++) {
            int current_match = 0;
            int printed = 0;

            for (int j = 0; j < actual_insegnanti; j++) {
                if (get_ore_classi_insegnante(c, j) > 0) {
                    if (current_match == r) {
                        int ore_i_s = get_ore_classi_insegnante_svolte(c, j);
                        int ore_i = get_ore_classi_insegnante(c, j);

                        char line_buf[64];
                        snprintf(line_buf, sizeof(line_buf), "%-18s %2d/%2d [%d]",
                                 insegnanti[j], ore_i_s, ore_i, ore_i_s == ore_i);

                        fprintf(f, "%-*s | ", COL_WIDTH, line_buf);
                        printed = 1;
                        break;
                    }
                    current_match++;
                }
            }
            if (!printed) {
                char empty_buf[1] = "";
                fprintf(f, "%-*s | ", COL_WIDTH, empty_buf);
            }
        }
        fprintf(f, "\n");
    }

    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int i = 0; i < COL_WIDTH; i++) fprintf(f, "-");
        fprintf(f, "-+-");
    }
    fprintf(f, "\n");
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
