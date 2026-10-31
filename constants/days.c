#include "days.h"

#include "../data_initialization/database_initializers.h"
const char giorni_settimana[GIORNO_SETTIMANA_SZ][MAX_LEN_WORDS] = {
    "Lun",
    "Mar",
    "Mer",
    "Gio",
    "Ven",
    "Sab"
};

const int giorni_settimana_COUNT = sizeof(giorni_settimana) / sizeof(giorni_settimana[0]);
