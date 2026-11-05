//
// Created by Alberto Tiraboschi on 30/10/2026.
//

#ifndef UNTITLED_UTILS_H
#define UNTITLED_UTILS_H

static void swap_min_first(int *p1, int *p2) {
    if (*p1 > *p2) {
        const int temp = *p1;
        *p1 = *p2;
        *p2 = temp;
    }
}


#endif //UNTITLED_UTILS_H
