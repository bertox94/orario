//
// Created by Alberto Tiraboschi on 13/06/2026.
//

#ifndef UNTITLED_PRINTER_H
#define UNTITLED_PRINTER_H

void print_orario_form(const elem orario[], const int tot, const long attempt) {
    time_t currentTime;
    time(&currentTime); // Get the current time

    printf("Current time: %s", ctime(&currentTime));
    printf("Orario n. %lu\n", attempt);

    for (int curr_class = 0; curr_class < actual_classi_sezione; curr_class++) {
        printf("\nClasse: %s, ore: %d\n", classe_sezione[curr_class], ore_classe_settimanali[curr_class]);
        printf("Giorno:     ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            printf(" %c%c",
                   giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][0],
                   giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][1]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                printf(" ");
        }
        printf("\n");

        printf("Ora:        ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            printf(" %s", ore_giornaliere[i / actual_classi_sezione % actual_ore_giornaliere]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                printf(" ");
        }
        printf("\n");

        printf("Aula:       ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].aula == -1)
                printf("  -");
            else
                printf("  %d", orario[i].aula);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                printf(" ");
        }
        printf("\n");

        printf("Materia:    ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].materia == -1)
                printf("  -");
            else
                printf("%c%c%c", materie[orario[i].materia][0], materie[orario[i].materia][1],
                       materie[orario[i].materia][2]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                printf(" ");
        }
        printf("\n");

        printf("Prof:       ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].insegnante1 == -1)
                printf("  -");
            else
                printf("%c%c%c", insegnanti[orario[i].insegnante1][0], insegnanti[orario[i].insegnante1][1],
                       insegnanti[orario[i].insegnante1][2]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                printf(" ");
        }
        printf("\n");
        printf("Prof:       ");
        for (int i = curr_class; i < tot; i += actual_classi_sezione) {
            if (orario[i].insegnante2 == -1)
                printf("  -");
            else
                printf("%c%c%c", insegnanti[orario[i].insegnante2][0], insegnanti[orario[i].insegnante2][1],
                       insegnanti[orario[i].insegnante2][2]);
            if (i / actual_classi_sezione % actual_ore_giornaliere == actual_ore_giornaliere - 1)
                printf(" ");
        }
        printf("\n");
    }
    printf("            ");
    for (int i = 0; i < tot/actual_classi_sezione; i++) {
        if (i %actual_ore_giornaliere == actual_ore_giornaliere-1)
            printf("-");
        printf("---");
    }
    printf("\n");
}

#endif //UNTITLED_PRINTER_H
