#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
//
// Created by Alberto Tiraboschi on 02/06/2026.
//
void test(int max) {
    struct timeval t1, t2;
    char *s = string_allocation(max + 1);

    for (int tot = 75; tot < max; tot++) {
        // Start timer
        gettimeofday(&t1, NULL);
        struct param parameters = {0, -1, 0};
        gen(s, 0, tot, parameters);

        // Stop timer
        gettimeofday(&t2, NULL);
        const unsigned long elapsedTime = t2.tv_sec - t1.tv_sec;
        printf("len: %d, %lu s.\n", tot, elapsedTime);
    }
    free(s);
}