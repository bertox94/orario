#include "passo.h"
#include "passi.h"
#include "ricerca.h"
#include "vincoli.h"
#include "data_initialization/generatore.h"
#include "json.h"

#include <stdio.h>

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
static int primo;  // per le virgole dentro un elenco di controlli
static void controllo(const char *nome, const int esito) {
    printf("%s[\"%s\",%d]", primo ? "" : ",", nome, esito);
    primo = 0;
}

int stampa_passo(const unsigned long long int seme, const int classi, const int ritardi, const char *file_passi,
                 const long long int k) {
    if (!genera_dati_casuali(seme, classi, ritardi)) return 1;
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    int prima;
    const int pos = passi_leggi(file_passi, k, orario_len, orario, &prima);
    if (pos < 0) return 1;
    rimetti_stato(pos);

    // l'orario fin qui
    printf("{\"k\":%lld,\"pos\":%d,\"prima\":", k, pos);
    if (prima < 0) printf("null");
    else printf("%d", prima);
    printf(",\"orario\":[");
    for (int i = 0; i < pos; i++) {
        printf(i ? ",[" : "[");
        json_nome(stdout, orario[i].insegnante1 < 0 ? NULL : insegnanti[orario[i].insegnante1]);
        putchar(',');
        json_nome(stdout, orario[i].insegnante2 < 0 ? NULL : insegnanti[orario[i].insegnante2]);
        putchar(']');
    }
    putchar(']');
    if (pos >= orario_len) {  // orario completo: nessuna cella da decidere
        printf(",\"completo\":1}\n");
        return 0;
    }
    const Cella c = cella(pos);
    sort_queue_mul(c.base4d_queue, queue_sz);
    QueueItem **coda = &queue_mul[c.base4d_queue];

    // la classe
    static const char *stati[] = {"deve", "può", "non può"};  // nell'ordine di StatoClasse
    const int lezione = activities_for_this_class_and_hour(&c);
    printf(",\"classe\":{\"stato\":\"%s\",\"ha_attivita\":%d,\"ore_rimanenti\":%d,"
           "\"celle_obbligatorie_rimanenti\":%d,\"ore_buche_oggi\":%d,\"ora_buca_possibile\":%d,"
           "\"posti_nei_giorni_dopo\":%d}",
           stati[stato_della_classe(&c)], lezione,
           ore_rimaste_alla_classe(&c),
           PRIME_N_ORE_OBBLIGATORIE * actual_giorni_settimana
           - (c.giorno * PRIME_N_ORE_OBBLIGATORIE
              + (c.ora < PRIME_N_ORE_OBBLIGATORIE - 1 ? c.ora : PRIME_N_ORE_OBBLIGATORIE - 1)),
           n_ore_buche[c.giorno * CLASSE_SEZIONE_SZ + c.cla], la_classe_puo_avere_ora_buca(&c),
           posti_nei_giorni_dopo(&c));

    // la coda, in ordine
    printf(",\"coda\":[");
    for (int i = 0; i < queue_sz; i++) {
        const int p = coda[i]->teacher;
        printf("%s{\"docente\":", i ? "," : "");
        json_stringa(stdout, insegnanti[p]);
        printf(",\"priorita\":%.0f,\"nella_classe\":%d}", coda[i]->priority, ore_classi_docenti[c.base2d_ci + p] > 0);
    }

    // i docenti della classe, nell'ordine della coda, con tutti i controlli
    printf("],\"docenti\":[");
    int scritti = 0;
    for (int i = 0; i < queue_sz; i++) {
        const int p1 = coda[i]->teacher;
        if (ore_classi_docenti[c.base2d_ci + p1] == 0) continue;
        printf("%s{\"docente\":", scritti++ ? "," : "");
        json_stringa(stdout, insegnanti[p1]);
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
            json_stringa(stdout, insegnanti[p2]);
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
