#include "passo.h"
#include "passi.h"
#include "ricerca.h"
#include "vincoli.h"
#include "data_initialization/generatore.h"
#include "json.h"

#include <stdio.h>

// ---- lo stato della ricerca con le prime pos celle decise ----
static void rimetti_stato(Ricerca *r, const int pos) {
    for (int i = 0; i < pos; i++) {
        const Cella c = cella(r, i);
        const teaching_slot t = r->orario[i];
        if (t.insegnante1 < 0) {
            r->n_ore_buche[c.giorno * CLASSE_SEZIONE_SZ + c.cla]++;
            continue;
        }
        r->ore_classe_svolte[c.cla]++;
        if (t.insegnante2 < 0) segna_da_solo(r, &c, t.insegnante1, 1);
        else segna_coppia(r, &c, t.insegnante1, t.insegnante2, 1);
    }
}

// ---- il JSON ----
// Un controllo dentro un elenco: ["nome", esito], con la virgola davanti se non e' il primo. Ritorna 0: il prossimo
// non e' piu' il primo (primo = controllo(primo, ...)).
static int controllo(const int primo, const char *nome, const int esito) {
    printf("%s[\"%s\",%d]", primo ? "" : ",", nome, esito);
    return 0;
}

int stampa_passo(const unsigned long long int seme, const int classi, const int ritardi, const char *file_passi,
                 const long long int k) {
    Scuola *s = nuova_scuola();
    if (!genera_dati_casuali(s, seme, classi, ritardi)) return 1;
    Ricerca *r = nuova_ricerca(s);
    int prima;
    const int pos = passi_leggi(file_passi, k, r->orario_len, r->orario, &prima);
    if (pos < 0) return 1;
    rimetti_stato(r, pos);

    // l'orario fin qui
    printf("{\"k\":%lld,\"pos\":%d,\"prima\":", k, pos);
    if (prima < 0) printf("null");
    else printf("%d", prima);
    printf(",\"orario\":[");
    for (int i = 0; i < pos; i++) {
        printf(i ? ",[" : "[");
        json_nome(stdout, r->orario[i].insegnante1 < 0 ? NULL : s->insegnanti[r->orario[i].insegnante1]);
        putchar(',');
        json_nome(stdout, r->orario[i].insegnante2 < 0 ? NULL : s->insegnanti[r->orario[i].insegnante2]);
        putchar(']');
    }
    putchar(']');
    if (pos >= r->orario_len) {  // orario completo: nessuna cella da decidere
        printf(",\"completo\":1}\n");
        return 0;
    }
    const Cella c = cella(r, pos);
    // la coda di tutti i docenti, in ordine: quella della ricerca in questa cella ha solo i docenti della classe,
    // nello stesso ordine
    QueueItem *coda[INSEGNANTE_SZ];
    const int n_coda = s->actual_insegnanti;
    for (int p = 0; p < n_coda; p++) coda[p] = &r->queue_items[p];
    sort_queue(coda, n_coda);

    // la classe
    const char *stati[] = {"deve", "può", "non può"};  // nell'ordine di StatoClasse
    const int lezione = activities_for_this_class_and_hour(r, &c);
    printf(",\"classe\":{\"stato\":\"%s\",\"ha_attivita\":%d,\"ore_rimanenti\":%d,"
           "\"celle_obbligatorie_rimanenti\":%d,\"ore_buche_oggi\":%d,\"ora_buca_possibile\":%d,"
           "\"posti_nei_giorni_dopo\":%d}",
           stati[stato_della_classe(r, &c)], lezione,
           ore_rimaste_alla_classe(r, &c),
           PRIME_N_ORE_OBBLIGATORIE * s->actual_giorni_settimana
           - (c.giorno * PRIME_N_ORE_OBBLIGATORIE
              + (c.ora < PRIME_N_ORE_OBBLIGATORIE - 1 ? c.ora : PRIME_N_ORE_OBBLIGATORIE - 1)),
           r->n_ore_buche[c.giorno * CLASSE_SEZIONE_SZ + c.cla], la_classe_puo_avere_ora_buca(r, &c),
           posti_nei_giorni_dopo(r, &c));

    // la coda, in ordine
    printf(",\"coda\":[");
    for (int i = 0; i < n_coda; i++) {
        const int p = coda[i]->teacher;
        printf("%s{\"docente\":", i ? "," : "");
        json_stringa(stdout, s->insegnanti[p]);
        printf(",\"priorita\":%d,\"nella_classe\":%d}", coda[i]->priority,
               get_ore_insegnante_in_una_classe(s, c.cla, p) > 0);
    }

    // i docenti della classe, nell'ordine della coda, con tutti i controlli
    printf("],\"docenti\":[");
    int scritti = 0;
    for (int i = 0; i < n_coda; i++) {
        const int p1 = coda[i]->teacher;
        if (get_ore_insegnante_in_una_classe(s, c.cla, p1) == 0) continue;
        printf("%s{\"docente\":", scritti++ ? "," : "");
        json_stringa(stdout, s->insegnanti[p1]);
        printf(",\"posto\":%d,\"priorita\":%d,\"ore_rimaste\":%d,\"puo\":%d,\"controlli\":[", i + 1, coda[i]->priority,
               ore_rimaste(r, &c, p1), docente_puo_insegnare(r, &c, p1));
        int primo = 1;
        primo = controllo(primo, "libero", prof_è_disponibile(r, &c, p1));
        primo = controllo(primo, "ore_nella_classe", deve_ancora_svolgere_ore_in_questa_classe(r, &c, p1));
        primo = controllo(primo, "max_ore_oggi", prof_fa_al_max_n_ore(r, &c, p1));
        primo = controllo(primo, "ore_contigue", prof_fa_ore_contigue(r, &c, p1));
        primo = controllo(primo, "max_classi_oggi", ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(r, &c, p1));
        primo = controllo(primo, "entrato", verifica_ora_ingresso(r, p1, c.ora));
        printf("],\"da_solo\":{\"puo\":%d,\"controlli\":[", docente_puo_insegnare_da_solo(r, &c, p1));
        primo = 1;
        primo = controllo(primo, "ore_da_solo", deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(r, &c, p1));
        primo = controllo(primo, "sabato", sabato_esaurisci_priorita_subito(r, &c, p1));
        printf("]},\"coppie\":[");
        int coppie = 0;
        for (int j = i + 1; j < n_coda; j++) {  // con quelli dopo di lui nella coda, come la ricerca
            const int p2 = coda[j]->teacher;
            if (get_ore_insegnanti_codocenza_in_una_classe(s, c.cla, p1, p2) == 0) continue;
            printf("%s{\"con\":", coppie++ ? "," : "");
            json_stringa(stdout, s->insegnanti[p2]);
            printf(",\"puo\":%d,\"controlli\":[", secondo_della_coppia_puo_insegnare(r, &c, p1, p2));
            primo = 1;
            primo = controllo(primo, "libero", prof_è_disponibile(r, &c, p2));
            primo = controllo(primo, "ore_insieme",
                              docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(r, &c, p1, p2));
            primo = controllo(primo, "max_ore_oggi", prof_fa_al_max_n_ore(r, &c, p2));
            primo = controllo(primo, "ore_contigue", prof_fa_ore_contigue(r, &c, p2));
            primo = controllo(primo, "sabato", sabato_esaurisci_priorita_subito_cod(r, &c, p1, p2));
            primo = controllo(primo, "max_classi_oggi", ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(r, &c, p2));
            primo = controllo(primo, "entrato", verifica_ora_ingresso(r, p2, c.ora));
            printf("]}");
        }
        printf("]}");
    }
    printf("]}\n");
    return 0;
}
