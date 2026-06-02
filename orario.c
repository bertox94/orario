//
// Created by Alberto Tiraboschi on 02/06/2026.
//
int ore_totali(const char *orario, unsigned int orario_len) {
    unsigned int sum = 0;
    for (int i = 0; i < orario_len; i++) {
        if (orario[i] == '1')
            sum++;
    }
    return sum;
}