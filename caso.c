#include "caso.h"
#include "ricerca.h"
#include "data_initialization/generatore.h"
#include "passi.h"
#include "verifica_dati.h"
#include "verifica_orario.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Quello che serve per seguire un caso e scriverne lo stato. La ricerca lo porta con se' (r->di_chi) e lo passa alla
// funzione che si chiama a ogni nodo.
typedef struct {
    const char *file_stato;
    clock_t inizio, ultima_scrittura;
    const char *esito;
    char errore[VERIFICA_DATI_ERRORE];                  // il motivo, quando l'esito e' «errore nei dati»
    ControlloOrario verifica[VERIFICA_ORARIO_CONTROLLI];  // la verifica indipendente dell'orario trovato
    int verificato;                                     // -1: non fatta, 1: l'orario e' valido, 0: no
    // la foto dell'orario all'ultimo nodo scritto: durante la ricerca l'orario cambia, la foto no
    teaching_slot foto[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
    int foto_pos;
    Passi *passi;  // il file dei passi (NULL: non chiesto)
} Caso;

static void elenco(FILE *f, const char *campo, const char nomi[][MAX_LEN_WORDS], const int n) {
    fprintf(f, ",\"%s\":[", campo);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', f);
        json_stringa(f, nomi[i]);
    }
    fputc(']', f);
}

// Lo stato del caso su file (vedi caso.h). r e' la ricerca, o NULL se non e' partita.
static void scrivi_stato(const Caso *k, const Scuola *s, const Ricerca *r, const int in_corso) {
    char provvisorio[1024];
    snprintf(provvisorio, sizeof provvisorio, "%s.tmp", k->file_stato);
    FILE *f = fopen(provvisorio, "w");
    if (!f) return;
    fprintf(f, "{\"in_corso\":%d,\"esito\":", in_corso);
    json_stringa(f, k->esito);
    if (k->errore[0]) {
        fputs(",\"errore\":", f);
        json_stringa(f, k->errore);
    }
    if (k->verificato >= 0) {
        fprintf(f, ",\"verifica\":{\"ok\":%d,\"controlli\":[", k->verificato);
        for (int i = 0; i < VERIFICA_ORARIO_CONTROLLI; i++) {
            fputs(i ? ",[" : "[", f);
            json_stringa(f, k->verifica[i].nome);
            fprintf(f, ",%d,", k->verifica[i].ok);
            json_stringa(f, k->verifica[i].dettaglio);
            fputc(']', f);
        }
        fputs("]}", f);
    }
    fprintf(f, ",\"nodi\":%lld,\"secondi\":%.2f,\"pos\":%d,\"pos_max\":%d,\"celle\":%d", r ? r->nodi : 0,
            (clock() - k->inizio) / (double) CLOCKS_PER_SEC, k->foto_pos, r ? r->pos_max : 0,
            s->actual_giorni_settimana * s->actual_ore_giornaliere * s->actual_classi_sezione);
    elenco(f, "giorni", s->giorni_settimana, s->actual_giorni_settimana);
    elenco(f, "ore", s->ore_giornaliere, s->actual_ore_giornaliere);
    elenco(f, "classi", s->classe_sezione, s->actual_classi_sezione);
    fputs(",\"orario\":[", f);
    for (int i = 0; i < k->foto_pos; i++) {
        fputs(i ? ",[" : "[", f);
        json_nome(f, k->foto[i].insegnante1 < 0 ? NULL : s->insegnanti[k->foto[i].insegnante1]);
        fputc(',', f);
        json_nome(f, k->foto[i].insegnante2 < 0 ? NULL : s->insegnanti[k->foto[i].insegnante2]);
        fputc(']', f);
    }
    fputs("]}\n", f);
    fclose(f);
    rename(provvisorio, k->file_stato);
}

static void fotografa(Caso *k, const Ricerca *r, const int pos) {
    memcpy(k->foto, r->orario, sizeof(teaching_slot) * pos);
    k->foto_pos = pos;
}

// a ogni nodo: il passo su file; la foto quando la ricerca finisce qui (orario completo, ultimo nodo prima del
// limite) e ogni mezzo secondo (guardato ogni 4096 nodi), con lo stato su file
static void segui(Ricerca *r, const int pos) {
    Caso *k = r->di_chi;
    passi_nodo(k->passi, r->nodi, pos, r->orario, r->orario_len);
    if (pos == r->orario_len || (r->limite_nodi && r->nodi == r->limite_nodi)) {
        fotografa(k, r, pos);
        return;
    }
    if (r->nodi & 4095) return;
    const clock_t adesso = clock();
    if (adesso - k->ultima_scrittura < CLOCKS_PER_SEC / 2) return;
    k->ultima_scrittura = adesso;
    fotografa(k, r, pos);
    scrivi_stato(k, r->scuola, r, 1);
}

int esegui_caso(const unsigned long long int seme, const int classi, const int ritardi, const long long int limite,
                const char *file, const char *file_passi) {
    Caso *k = calloc(1, sizeof(Caso));
    Scuola *s = nuova_scuola();
    if (!k) return 2;
    k->file_stato = file;
    k->esito = "in corso";
    k->verificato = -1;
    k->inizio = k->ultima_scrittura = clock();
    if (file_passi && !(k->passi = passi_apri(file_passi))) return 2;
    if (!genera_dati_casuali(s, seme, classi, ritardi)) {
        k->esito = "dati non generati";
        scrivi_stato(k, s, NULL, 0);
        return 1;
    }
    if (!verifica_dati(s, k->errore, sizeof k->errore)) {  // l'orario non puo' esistere: la ricerca non parte
        passi_chiudi(k->passi);
        k->esito = "errore nei dati";
        scrivi_stato(k, s, NULL, 0);
        return 0;
    }
    Ricerca *r = nuova_ricerca(s);
    r->limite_nodi = limite;
    r->ogni_nodo = segui;
    r->di_chi = k;
    scrivi_stato(k, s, r, 1);
    print_orario(r, 0);
    passi_chiudi(k->passi);
    k->esito = r->trovato ? "trovato" : r->fermato ? "limite di nodi" : "nessuna soluzione";
    if (r->trovato) k->verificato = verifica_orario(s, r->orario, k->verifica);
    if (!r->trovato && !r->fermato) k->foto_pos = 0;  // nessuna soluzione: non resta niente da mostrare
    scrivi_stato(k, s, r, 0);
    libera_ricerca(r);
    libera_scuola(s);
    free(k);
    return 0;
}
