//
// Created by Alberto Tiraboschi on 13/06/2026.
//

#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H
#include <sys/errno.h>
#include <unistd.h>

void print_orario_form(const elem orario[], const int tot, const long attempt, FILE *f) {

    fseek(f, 0, SEEK_END);
    const long current_size = ftell(f);
    if (current_size > 9999999999) {
        fseek(f, 0, SEEK_SET);
        ftruncate(fileno(f), 0);
    }

    time_t currentTime;
    time(&currentTime); // Get the current time
    fprintf(f, "Current time: %s", ctime(&currentTime));
    fprintf(f, "Orario n. %lu, pos: %d", attempt, tot);

    for (int curr_class = 0; curr_class < actual_classi_sezione; curr_class++) {
        fprintf(f, "\nClasse: %s, ore: %d\n", classe_sezione[curr_class], ore_classe_settimanali[curr_class]);
        fprintf(f, "Giorno:     ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            fprintf(f, " %c%c",
                    giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][0],
                    giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][1]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                fprintf(f, " ");
        }
        fprintf(f, "\n");

        fprintf(f, "Ora:        ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            fprintf(f, " %s", ore_giornaliere[i / actual_classi_sezione % actual_ore_giornaliere]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                fprintf(f, " ");
        }
        fprintf(f, "\n");

        fprintf(f, "Aula:       ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].aula == -1)
                fprintf(f, "  -");
            else
                fprintf(f, "  %d", orario[i].aula);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                fprintf(f, " ");
        }
        fprintf(f, "\n");

        fprintf(f, "Materia:    ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].materia == -1)
                fprintf(f, "  -");
            else
                fprintf(f, "%c%c%c", materie[orario[i].materia][0], materie[orario[i].materia][1],
                        materie[orario[i].materia][2]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                fprintf(f, " ");
        }
        fprintf(f, "\n");

        fprintf(f, "Prof:       ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].insegnante1 == -1)
                fprintf(f, "  -");
            else
                fprintf(f, "%c%c%c", insegnanti[orario[i].insegnante1][0], insegnanti[orario[i].insegnante1][1],
                        insegnanti[orario[i].insegnante1][2]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                fprintf(f, " ");
        }
        fprintf(f, "\n");
        fprintf(f, "Prof:       ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].insegnante2 == -1)
                fprintf(f, "  -");
            else
                fprintf(f, "%c%c%c", insegnanti[orario[i].insegnante2][0], insegnanti[orario[i].insegnante2][1],
                        insegnanti[orario[i].insegnante2][2]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                fprintf(f, " ");
        }
        fprintf(f, "\n");
    }
    fprintf(f, "            ");
    for (int i = 0; i < tot / actual_classi_sezione; i++) {
        if (i % actual_ore_giornaliere == actual_ore_giornaliere - 1)
            fprintf(f, "-");
        fprintf(f, "---");
    }
    fprintf(f, "\n");
    fflush(f);
}

#endif //UNTITLED_PRINTER_H
