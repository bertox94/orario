#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../printer/printer.h"
#include "../data_initialization/database_initializers.h"
#include "../priority_queue/prio_queue.h"
//
// Created by Alberto Tiraboschi on 20/06/2026.
//
// Copia src in dest tagliandolo o allungandolo con spazi a maxlen - 1 caratteri (piu' il terminatore):
// tutte le celle della griglia hanno la stessa larghezza.
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

// Riga di separazione tra le righe della griglia: una colonna per l'etichetta della classe e una per ogni giorno/ora.
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

        // Ogni classe occupa due righe: il primo docente dello slot e, sotto, il codocente (se c'e').
        // Gli slot da pos in poi non sono ancora decisi in questo punto della ricerca: si stampano vuoti.
        // --- Row for Insegnante 1 ---
        fprintf(f, " %-14s |", formatted_class);
        for (int g = 0; g < actual_giorni_settimana; g++) {
            for (int o = 0; o < actual_ore_giornaliere; o++) {
                int curr;
                int p = slot_di(g, o, c);
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
                int p = slot_di(g, o, c);
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

    // 4. Print Teacher workload summaries side-by-side per class
    fprintf(f, "\n--- Statistiche Insegnanti per Classe (Side-by-Side) ---\n\n");

    // Define fixed column width for each class block (e.g., 35 characters wide per class)
    const int COL_WIDTH = 35;

    // 1. Print Header Row with Class Names
    for (int c = 0; c < actual_classi_sezione; c++) {
        char header_buf[64];
        snprintf(header_buf, sizeof(header_buf), "Classe: %s", classe_sezione[c]);
        fprintf(f, "%-*s | ", COL_WIDTH, header_buf);
    }
    fprintf(f, "\n");

    // Print separator line matching the column widths
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int i = 0; i < COL_WIDTH; i++) fprintf(f, "-");
        fprintf(f, "-+-");
    }
    fprintf(f, "\n");

    // 2. Determine the maximum number of active teachers in any class
    int max_active_teachers = 0;
    for (int c = 0; c < actual_classi_sezione; c++) {
        int count = 0;
        for (int j = 0; j < actual_insegnanti; j++) {
            if (get_ore_classi_insegnante(c, j) > 0) count++;
        }
        if (count > max_active_teachers) max_active_teachers = count;
    }

    // 3. Print each teacher row side-by-side across classes
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
                        // Format: "TeacherName: 5/10 [1]" (ore svolte/ore totali nella classe, [1] = tutte svolte)
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
                // Empty padding block if this class has fewer teachers than max_active_teachers
                char empty_buf[1] = "";
                fprintf(f, "%-*s | ", COL_WIDTH, empty_buf);
            }
        }
        fprintf(f, "\n");
    }

    // Print bottom separator
    for (int c = 0; c < actual_classi_sezione; c++) {
        for (int i = 0; i < COL_WIDTH; i++) fprintf(f, "-");
        fprintf(f, "-+-");
    }
    fprintf(f, "\n");
}

/* ---- marce indietro in formato compatto (vedi printer.h) ---- */

// Gli slot dell'ultima riga B scritta: la riga successiva riscrive solo da dove cambiano.
static int ultimi_1[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
static int ultimi_2[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
static int n_ultimi = 0;

void stampa_intestazione_marce_compatte(FILE *f) {
    fprintf(f, "# Marce indietro in formato compatto (vedi printer.h)\n");
    fprintf(f, "# Riga \"B pos da s_da ... s_(pos-1)\": marcia indietro nello slot pos; gli slot da `da` a pos-1 come "
               "scritto, quelli prima come nella riga B precedente. Slot: \"i\" docente, \"i+j\" coppia, \"-\" vuoto.\n");
    fprintf(f, "# Con --motivi, dopo ogni riga B c'e' una riga \"M d:m ...\": il motivo m per cui il docente d (con ore da fare nella "
               "classe) non si e' potuto mettere nello slot (i nomi dei motivi sono in \"nomi_motivi\" del JSON).\n");
    fprintf(f, "# Gli orari completi sono scritti per intero, come senza -b.\n");
    fprintf(f, "G %d %d %d\n", actual_giorni_settimana, actual_ore_giornaliere, actual_classi_sezione);
    // ordine delle classi dentro ogni ora (lo stesso per tutte le ore)
    fprintf(f, "K");
    for (int k = 0; k < actual_classi_sezione; k++) fprintf(f, " %s", classe_sezione[orario[k].classe]);
    fprintf(f, "\n");
    // indice -> nome dei docenti (un docente per riga: i nomi possono contenere spazi)
    for (int p = 0; p < actual_insegnanti; p++) fprintf(f, "D %d %s\n", p, insegnanti[p]);
}

void stampa_marcia_compatta(FILE *f, const teaching_slot orario[], const int pos) {
    // primo slot diverso dall'ultima riga scritta (la ricerca cambia solo la coda del percorso)
    int da = 0;
    while (da < pos && da < n_ultimi && orario[da].insegnante1 == ultimi_1[da] && orario[da].insegnante2 == ultimi_2[da])
        da++;
    fprintf(f, "B %d %d", pos, da);
    for (int i = da; i < pos; i++) {
        const int a = orario[i].insegnante1, b = orario[i].insegnante2;
        if (a < 0) fprintf(f, " -");
        else if (b < 0) fprintf(f, " %d", a);
        else fprintf(f, " %d+%d", a, b);
        ultimi_1[i] = a;
        ultimi_2[i] = b;
    }
    fprintf(f, "\n");
    n_ultimi = pos;
}

// Riepilogo dei dati su stdout: giorni, ore, ore di ogni classe, ore di ogni docente e ore di ogni docente per classe.
// tests/casuali.py legge le righe "Classe ...: N ore" e "Ins. ...: N ore" per controllare i dati generati.
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
        printf("Classe %s:\t%d ore\n", classe_sezione[i], classe_info[i].ore);
    printf("-----------\n");
    printf("Ore insegnanti:\n");
    for (int i = 0; i < actual_insegnanti; i++) {
        char str[MAX_STR_SZ_IN_DISPLAY];
        copy_and_pad(str, insegnanti[i], MAX_STR_SZ_IN_DISPLAY);
        printf("Ins. %s: %d ore\n", str, docente_info[i].ore);
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

// Descrizione completa dei dati (classi, docenti, lezioni da soli e in codocenza) in forma leggibile.
void stampa_dati_su_file(FILE *fp) {
    fprintf(fp, "CLASSI (%d)\n", actual_classi_sezione);
    for (int c = 0; c < actual_classi_sezione; c++)
        fprintf(fp, "  %-6s %2d ore\n", classe_sezione[c], classe_info[c].ore);

    fprintf(fp, "\nDOCENTI (%d)\n", actual_insegnanti);
    for (int p = 0; p < actual_insegnanti; p++) {
        int classi = 0;
        for (int c = 0; c < actual_classi_sezione; c++)
            if (classe_docente[c * INSEGNANTE_SZ + p].ore > 0) classi++;
        fprintf(fp, "  %-8s %2d ore in %d classi", insegnanti[p], docente_info[p].ore, classi);
        if (docente_info[p].ora_entrata > 0) fprintf(fp, ", puo' iniziare dall'ora di indice %d", docente_info[p].ora_entrata);
        fprintf(fp, "\n");
    }

    // per ogni docente di una classe: ore da solo e ore con ogni codocente
    fprintf(fp, "\nLEZIONI PER CLASSE (ore totali del docente nella classe = da solo + in codocenza)\n");
    for (int c = 0; c < actual_classi_sezione; c++) {
        fprintf(fp, "%s:\n", classe_sezione[c]);
        for (int p = 0; p < actual_insegnanti; p++) {
            const classe_docente_t *cd = &classe_docente[c * INSEGNANTE_SZ + p];
            if (cd->ore <= 0) continue;
            fprintf(fp, "  %-8s %2d ore: %2d da solo", insegnanti[p], cd->ore, cd->alone);
            for (int q = 0; q < actual_insegnanti; q++) {
                if (q == p) continue;
                const int lo = p < q ? p : q, hi = p < q ? q : p;
                const int ore = codocenza[c * INSEGNANTE_SZ * INSEGNANTE_SZ + lo * INSEGNANTE_SZ + hi].ore;
                if (ore > 0) fprintf(fp, ", %d con %s", ore, insegnanti[q]);
            }
            fprintf(fp, "\n");
        }
    }

    // ogni coppia una volta sola (q > p)
    fprintf(fp, "\nCODOCENZE (classe: docente + docente = ore insieme)\n");
    int n_cod = 0;
    for (int c = 0; c < actual_classi_sezione; c++)
        for (int p = 0; p < actual_insegnanti; p++)
            for (int q = p + 1; q < actual_insegnanti; q++) {
                const int ore = codocenza[c * INSEGNANTE_SZ * INSEGNANTE_SZ + p * INSEGNANTE_SZ + q].ore;
                if (ore > 0) {
                    fprintf(fp, "  %-6s %s + %s = %d ore\n", classe_sezione[c], insegnanti[p], insegnanti[q], ore);
                    n_cod++;
                }
            }
    fprintf(fp, "(%d coppie)\n", n_cod);
}
