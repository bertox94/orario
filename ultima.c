#include "ultima.h"
#include "passi.h"
#include "ricerca.h"
#include "vincoli.h"
#include "data_initialization/generatore.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>

#define CONTROLLI_DOCENTE 6
#define CONTROLLI_DA_SOLO 2
#define CONTROLLI_COPPIA 2

// In quante visite ogni controllo era vero: per un docente (i suoi controlli, poi da solo) e per una coppia.
typedef struct {
    int controlli[CONTROLLI_DOCENTE], puo;
    int da_solo[CONTROLLI_DA_SOLO], puo_da_solo;
} ContiDocente;

typedef struct {
    int p1, p2;
    int controlli[CONTROLLI_COPPIA], puo;
} ContiCoppia;

// La cella piu' avanti nel file dei passi (-1 se il file non c'e' o e' vuoto).
static int cella_piu_avanti(const char *file_passi) {
    FILE *f = fopen(file_passi, "rb");
    if (!f) return -1;
    int pos, massima = -1;
    teaching_slot prima;
    while (passi_prossimo(f, &pos, &prima)) if (pos > massima) massima = pos;
    fclose(f);
    return massima;
}

// Porta lo stato della ricerca da «decise le prime `decise` celle» al nodo che sta per decidere la cella pos dopo
// aver messo `prima` nella cella pos - 1: si tolgono le scelte dalla cella pos - 1 in poi, poi si mette quella nuova.
static void vai_al_nodo(Ricerca *r, const int decise, const int pos, const teaching_slot prima) {
    for (int i = decise - 1; i >= pos - 1 && i >= 0; i--) {
        const Cella c = cella(r, i);
        segna_cella(r, &c, r->orario[i], -1);
        r->orario[i] = (teaching_slot){-1, -1};
    }
    if (pos == 0) return;
    const Cella c = cella(r, pos - 1);
    r->orario[pos - 1] = prima;
    segna_cella(r, &c, prima, 1);
}

// Una visita: i controlli di ogni docente della classe e di ogni coppia, contati dove sono veri.
static void conta_visita(const Ricerca *r, const Cella *c, ContiDocente *docenti, ContiCoppia *coppie,
                         const int n_coppie) {
    const Scuola *s = r->scuola;
    for (int p = 0; p < s->actual_insegnanti; p++) {
        if (!get_ore_insegnante_in_una_classe(s, c->cla, p)) continue;
        ContiDocente *d = &docenti[p];
        d->controlli[0] += prof_è_disponibile(r, c, p);
        d->controlli[1] += deve_ancora_svolgere_ore_in_questa_classe(r, c, p);
        d->controlli[2] += prof_fa_al_max_n_ore(r, c, p);
        d->controlli[3] += prof_fa_ore_contigue(r, c, p);
        d->controlli[4] += ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno(r, c, p);
        d->controlli[5] += verifica_ora_ingresso(r, p, c->ora);
        d->puo += docente_puo_insegnare(r, c, p);
        d->da_solo[0] += deve_svolgere_ancora_ora_in_questa_classe_senza_codocente(r, c, p);
        d->da_solo[1] += sabato_esaurisci_priorita_subito(r, c, p);
        d->puo_da_solo += docente_puo_insegnare(r, c, p) && docente_puo_insegnare_da_solo(r, c, p);
    }
    for (int i = 0; i < n_coppie; i++) {
        ContiCoppia *k = &coppie[i];
        k->controlli[0] += docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe(r, c, k->p1, k->p2);
        k->controlli[1] += sabato_esaurisci_priorita_subito_cod(r, c, k->p1, k->p2);
        k->puo += docente_puo_insegnare(r, c, k->p1) && secondo_della_coppia_puo_insegnare(r, c, k->p1, k->p2);
    }
}

// un elenco di controlli: [["nome", n], ...]
static void stampa_conti(const char *nomi[], const int *conti, const int n) {
    putchar('[');
    for (int i = 0; i < n; i++) printf("%s[\"%s\",%d]", i ? "," : "", nomi[i], conti[i]);
    putchar(']');
}

int stampa_ultima_cella(const unsigned long long int seme, const int classi, const int ritardi,
                        const char *file_passi, const char *file_visite) {
    const char *nomi_docente[CONTROLLI_DOCENTE] = {"libero", "ore_nella_classe", "max_ore_oggi", "ore_contigue",
                                                   "max_classi_oggi", "entrato"};
    const char *nomi_da_solo[CONTROLLI_DA_SOLO] = {"ore_da_solo", "sabato"};
    const char *nomi_coppia[CONTROLLI_COPPIA] = {"ore_insieme", "sabato"};
    Scuola *s = nuova_scuola();
    if (!genera_dati_casuali(s, seme, classi, ritardi)) return 1;
    Ricerca *r = nuova_ricerca(s);
    const int pos_max = cella_piu_avanti(file_passi);
    if (pos_max < 0) return 1;
    if (pos_max >= r->orario_len) {
        printf("{\"completo\":1}\n");
        return 0;
    }
    const Cella ultima = cella(r, pos_max);

    // i conti: uno per docente, e uno per ogni coppia con ore insieme nella classe
    ContiDocente *docenti = calloc(s->actual_insegnanti, sizeof(ContiDocente));
    ContiCoppia *coppie = calloc(s->actual_insegnanti * s->actual_insegnanti, sizeof(ContiCoppia));
    if (!docenti || !coppie) return 1;
    int n_coppie = 0;
    for (int p = 0; p < s->actual_insegnanti; p++)
        for (int q = p + 1; q < s->actual_insegnanti; q++)
            if (get_ore_insegnanti_codocenza_in_una_classe(s, ultima.cla, p, q) > 0)
                coppie[n_coppie++] = (ContiCoppia){.p1 = p, .p2 = q};

    // tutti i nodi, in ordine: lo stato si aggiorna a ogni nodo, i conti a ogni visita della cella piu' avanti
    FILE *f = fopen(file_passi, "rb"), *v = fopen(file_visite, "wb");
    if (!f || !v) return 1;
    int pos, decise = 0;
    teaching_slot prima;
    long long int nodo = 0, visite = 0;
    while (passi_prossimo(f, &pos, &prima)) {
        nodo++;
        vai_al_nodo(r, decise, pos, prima);
        decise = pos;
        if (pos != pos_max) continue;
        visite++;
        fwrite(&nodo, sizeof nodo, 1, v);
        conta_visita(r, &ultima, docenti, coppie, n_coppie);
    }
    fclose(f);
    fclose(v);

    printf("{\"pos\":%d,\"celle\":%d,\"visite\":%lld,\"docenti\":[", pos_max, r->orario_len, visite);
    int scritti = 0;
    for (int p = 0; p < s->actual_insegnanti; p++) {
        if (!get_ore_insegnante_in_una_classe(s, ultima.cla, p)) continue;
        printf("%s{\"docente\":", scritti++ ? "," : "");
        json_stringa(stdout, s->insegnanti[p]);
        printf(",\"controlli\":");
        stampa_conti(nomi_docente, docenti[p].controlli, CONTROLLI_DOCENTE);
        printf(",\"puo\":%d,\"da_solo\":{\"controlli\":", docenti[p].puo);
        stampa_conti(nomi_da_solo, docenti[p].da_solo, CONTROLLI_DA_SOLO);
        printf(",\"puo\":%d}}", docenti[p].puo_da_solo);
    }
    printf("],\"coppie\":[");
    for (int i = 0; i < n_coppie; i++) {
        printf("%s{\"a\":", i ? "," : "");
        json_stringa(stdout, s->insegnanti[coppie[i].p1]);
        printf(",\"b\":");
        json_stringa(stdout, s->insegnanti[coppie[i].p2]);
        printf(",\"controlli\":");
        stampa_conti(nomi_coppia, coppie[i].controlli, CONTROLLI_COPPIA);
        printf(",\"puo\":%d}", coppie[i].puo);
    }
    printf("]}\n");
    free(docenti);
    free(coppie);
    libera_ricerca(r);
    libera_scuola(s);
    return 0;
}
