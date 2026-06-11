//
// Created by Alberto Tiraboschi on 09/06/2026.
//

#ifndef UNTITLED_PRINTING_H
#define UNTITLED_PRINTING_H

#include "database_initializers.h"

void print_orario_form(const elem orario[], const int tot) {
    time_t currentTime;
    time(&currentTime); // Get the current time

    printf("Current time: %s", ctime(&currentTime));
    printf("Giorno:     ");
    for (int i = 0; i < tot; i++)
        printf(" %c%c",
               giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][0],
               giorni_settimana[i / actual_ore_giornaliere / actual_classi_sezione % actual_giorni_settimana][1]);
    printf("\n");
    printf("Ora:        ");
    for (int i = 0; i < tot; i++)
        printf(" %s", ora_giornaliera[i / actual_classi_sezione % actual_ore_giornaliere]);
    printf("\n");

    printf("Classe:     ");
    for (int i = 0; i < tot; i++)
        printf(" %s", classe_sezione[i % actual_classi_sezione]);
    printf("\n");

    printf("Aula:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].aula == -1)
            printf("  -");
        else
            printf("  %d", orario[i].aula);
    }
    printf("\n");

    printf("Materia:    ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].materia == -1)
            printf("  -");
        else
            printf("%c%c%c", materie[orario[i].materia][0], materie[orario[i].materia][1], materie[orario[i].materia][2]);
    }
    printf("\n");

    printf("Prof:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].insegnante1 == -1)
            printf("  -");
        else
            printf("%c%c%c", insegnanti[orario[i].insegnante1][0],insegnanti[orario[i].insegnante1][1],insegnanti[orario[i].insegnante1][2]);
    }
    printf("\n");
    printf("Prof:       ");
    for (int i = 0; i < tot; i++) {
        if (orario[i].insegnante2 == -1)
            printf("  -");
        else
            printf("%c%c%c", insegnanti[orario[i].insegnante2][0],insegnanti[orario[i].insegnante2][1],insegnanti[orario[i].insegnante2][2]);
    }
    printf("\n            ");
    for (int i = 0; i < tot; i++) {
        printf("---");
    }
    printf("\n");
}

#endif //UNTITLED_PRINTING_H
