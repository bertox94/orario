#include "controlli.h"
#include "passi.h"
#include "ricerca.h"
#include "vincoli.h"
#include "data_initialization/generatore.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// ---- l'orario al nodo k, dal file dei passi (come sito/passi.py) ----
static long dimensione(FILE *x) {
    fseek(x, 0, SEEK_END);
    return ftell(x);
}

// Riempie orario[] fino alla cella che il nodo k sta per decidere e la ritorna (-1: il nodo non c'e').
static int orario_al_nodo(const char *file_passi, const long long int k) {
    char file_ist[1024];
    snprintf(file_ist, sizeof file_ist, "%s.ist", file_passi);
    FILE *p = fopen(file_passi, "rb"), *ist = fopen(file_ist, "rb");
    if (!p || !ist) return -1;
    if (k < 1 || k > dimensione(p) / 6) return -1;
    static int16_t celle[2 * GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
    const long per_istantanea = 4L * orario_len;
    long long int j = k / PASSI_ISTANTANEA;  // l'istantanea j e' l'orario dopo il nodo j * PASSI_ISTANTANEA
    if (j > dimensione(ist) / per_istantanea) j = dimensione(ist) / per_istantanea;
    for (int i = 0; i < 2 * orario_len; i++) celle[i] = -1;
    if (j) {
        fseek(ist, (j - 1) * per_istantanea, SEEK_SET);
        if (fread(celle, 1, per_istantanea, ist) != (size_t) per_istantanea) return -1;
    }
    int pos = 0;
    fseek(p, j * PASSI_ISTANTANEA * 6, SEEK_SET);
    for (long long int n = j * PASSI_ISTANTANEA + 1; n <= k; n++) {
        int16_t r[3];
        if (fread(r, sizeof r, 1, p) != 1) return -1;
        pos = r[0];
        if (pos > 0) {
            celle[2 * (pos - 1)] = r[1];
            celle[2 * (pos - 1) + 1] = r[2];
        }
    }
    fclose(p);
    fclose(ist);
    for (int i = 0; i < pos; i++) orario[i] = (teaching_slot){celle[2 * i], celle[2 * i + 1]};
    return pos;
}

// ---- lo stato della ricerca con le prime pos celle decise ----
static QueueItem *in_coda(const int docente) {
    for (int i = 0; i < queue_sz; i++) if (queue_mul[i]->teacher == docente) return queue_mul[i];
    return NULL;
}

static void rimetti_stato(const int pos) {
    for (int i = 0; i < pos; i++) {
        const Cella c = cella(i);
        const teaching_slot t = orario[i];
        if (t.insegnante1 < 0) {
            n_ore_buche[c.giorno * CLASSE_SEZIONE_SZ + c.cla]++;
            continue;
        }
        ore_classe_svolte[c.cla]++;
        if (t.insegnante2 < 0) segna_da_solo(&c, in_coda(t.insegnante1), 1);
        else segna_coppia(&c, in_coda(t.insegnante1), in_coda(t.insegnante2), 1);
    }
}

// ---- il JSON ----
static void stringa(const char *s) {
    putchar('"');
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') putchar('\\');
        putchar(*s);
    }
    putchar('"');
}

static int primo;  // per le virgole dentro un elenco di controlli
static void controllo(const char *nome, const int esito) {
    printf("%s[\"%s\",%d]", primo ? "" : ",", nome, esito);
    primo = 0;
}

int stampa_controlli(const unsigned long long int seme, const int classi, const int ritardi, const char *file_passi,
                     const long long int k) {
    if (!genera_dati_casuali(seme, classi, ritardi)) return 1;
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    const int pos = orario_al_nodo(file_passi, k);
    if (pos < 0) return 1;
    rimetti_stato(pos);
    printf("{\"k\":%lld,\"pos\":%d", k, pos);
    if (pos >= orario_len) {  // orario completo: nessuna cella da decidere
        printf(",\"completo\":1}\n");
        return 0;
    }
    const Cella c = cella(pos);
    sort_queue_mul(c.base4d_queue, queue_sz);
    QueueItem **coda = &queue_mul[c.base4d_queue];

    // la classe
    const int lezione = activities_for_this_class_and_hour(&c);
    printf(",\"classe\":{\"ha_attivita\":%d,\"ore_rimanenti\":%d,\"celle_obbligatorie_rimanenti\":%d,"
           "\"ore_buche_oggi\":%d,\"ora_buca_possibile\":%d}", lezione, ore_classi[c.cla] - ore_classe_svolte[c.cla],
           PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana
           - (c.giorno * PRIME_N_ORE_OBBLIGATORIE
              + (c.ora < PRIME_N_ORE_OBBLIGATORIE - 1 ? c.ora : PRIME_N_ORE_OBBLIGATORIE - 1)),
           n_ore_buche[c.giorno * CLASSE_SEZIONE_SZ + c.cla], c.ora >= PRIME_N_ORE_OBBLIGATORIE);

    // la coda, in ordine
    printf(",\"coda\":[");
    for (int i = 0; i < queue_sz; i++) {
        const int p = coda[i]->teacher;
        printf("%s{\"docente\":", i ? "," : "");
        stringa(insegnanti[p]);
        printf(",\"priorita\":%.0f,\"nella_classe\":%d}", coda[i]->priority, ore_classi_docenti[c.base2d_ci + p] > 0);
    }

    // i docenti della classe, nell'ordine della coda, con tutti i controlli
    printf("],\"docenti\":[");
    int scritti = 0;
    for (int i = 0; i < queue_sz; i++) {
        const int p1 = coda[i]->teacher;
        if (ore_classi_docenti[c.base2d_ci + p1] == 0) continue;
        printf("%s{\"docente\":", scritti++ ? "," : "");
        stringa(insegnanti[p1]);
        printf(",\"posto\":%d,\"priorita\":%.0f,\"ore_rimaste\":%d,\"puo\":%d,\"controlli\":[", i + 1, coda[i]->priority,
               ore_rimaste(&c, p1), docente_puo_insegnare(&c, p1));
        primo = 1;
        controllo("libero", prof_è_disponibile(&c, p1));
        controllo("ore_nella_classe", deve_ancora_svolgere_ore_in_questa_classe(&c, p1));
        controllo("max_ore_oggi", prof_fa_al_max_n_ore(&c, p1));
        controllo("ore_contigue", prof_fa_ore_contigue(&c, p1));
        controllo("max_classi_oggi", ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(&c, p1));
        controllo("entrato", verifica_ora_ingresso(p1, c.ora));
        printf("],\"da_solo\":{\"puo\":%d,\"controlli\":[", docente_puo_insegnare_da_solo(&c, p1));
        primo = 1;
        controllo("ore_da_solo", deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(&c, p1));
        controllo("sabato", sabato_esaurisci_priorita_subito(&c, p1));
        printf("]},\"coppie\":[");
        int coppie = 0;
        for (int j = i + 1; j < queue_sz; j++) {  // con quelli dopo di lui nella coda, come la ricerca
            const int p2 = coda[j]->teacher;
            if (get_ore_insegnanti_codocenza_in_una_classe(c.cla, p1, p2) == 0) continue;
            printf("%s{\"con\":", coppie++ ? "," : "");
            stringa(insegnanti[p2]);
            printf(",\"puo\":%d,\"controlli\":[", secondo_della_coppia_puo_insegnare(&c, p1, p2));
            primo = 1;
            controllo("libero", prof_è_disponibile(&c, p2));
            controllo("ore_insieme", docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(&c, p1, p2));
            controllo("max_ore_oggi", prof_fa_al_max_n_ore(&c, p2));
            controllo("ore_contigue", prof_fa_ore_contigue(&c, p2));
            controllo("sabato", sabato_esaurisci_priorita_subito_cod(&c, p1, p2));
            controllo("max_classi_oggi", ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(&c, p2));
            controllo("entrato", verifica_ora_ingresso(p2, c.ora));
            printf("]}");
        }
        printf("]}");
    }
    printf("]}\n");
    return 0;
}
