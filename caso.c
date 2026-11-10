#include "caso.h"
#include "ricerca.h"
#include "data_initialization/generatore.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *file_stato;
static clock_t inizio, ultima_scrittura;
static const char *esito = "in corso";
// la foto dell'orario all'ultimo nodo scritto: durante la ricerca orario[] cambia, la foto no
static teaching_slot foto[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
static int foto_pos = 0;

static void stringa(FILE *r, const char *s) {
    fputc('"', r);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') fputc('\\', r);
        fputc(*s, r);
    }
    fputc('"', r);
}

static void elenco(FILE *r, const char *campo, char nomi[][MAX_LEN_WORDS], const int n) {
    fprintf(r, ",\"%s\":[", campo);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', r);
        stringa(r, nomi[i]);
    }
    fputc(']', r);
}

static void docente(FILE *r, const int p) {
    if (p < 0) fputs("null", r);
    else stringa(r, insegnanti[p]);
}

static void scrivi_stato(const int in_corso) {
    char provvisorio[1024];
    snprintf(provvisorio, sizeof provvisorio, "%s.tmp", file_stato);
    FILE *r = fopen(provvisorio, "w");
    if (!r) return;
    fprintf(r, "{\"in_corso\":%d,\"esito\":", in_corso);
    stringa(r, esito);
    fprintf(r, ",\"nodi\":%lld,\"secondi\":%.2f,\"pos\":%d,\"pos_max\":%d,\"celle\":%d", nodi,
            (clock() - inizio) / (double) CLOCKS_PER_SEC, foto_pos, pos_max, orario_len);
    elenco(r, "giorni", giorni_settimana, actual_giorni_settimana);
    elenco(r, "ore", ore_giornaliere, actual_ore_giornaliere);
    elenco(r, "classi", classe_sezione, actual_classi_sezione);
    fputs(",\"orario\":[", r);
    for (int i = 0; i < foto_pos; i++) {
        fputs(i ? ",[" : "[", r);
        docente(r, foto[i].insegnante1);
        fputc(',', r);
        docente(r, foto[i].insegnante2);
        fputc(']', r);
    }
    fputs("]}\n", r);
    fclose(r);
    rename(provvisorio, file_stato);
}

static void fotografa(const int pos) {
    memcpy(foto, orario, sizeof(teaching_slot) * pos);
    foto_pos = pos;
}

// a ogni nodo: la foto quando la ricerca finisce qui (orario completo, ultimo nodo prima del limite) e ogni mezzo
// secondo (guardato ogni 4096 nodi) anche lo stato su file
static void segui(const int pos) {
    if (pos == orario_len || (limite_nodi && nodi == limite_nodi)) {
        fotografa(pos);
        return;
    }
    if (nodi & 4095) return;
    const clock_t adesso = clock();
    if (adesso - ultima_scrittura < CLOCKS_PER_SEC / 2) return;
    ultima_scrittura = adesso;
    fotografa(pos);
    scrivi_stato(1);
}

int esegui_caso(const unsigned long long int seme, const int classi, const int ritardi, const long long int limite,
                const char *file) {
    file_stato = file;
    inizio = ultima_scrittura = clock();
    if (!genera_dati_casuali(seme, classi, ritardi)) {
        esito = "dati non generati";
        scrivi_stato(0);
        return 1;
    }
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    limite_nodi = limite;
    ogni_nodo = segui;
    scrivi_stato(1);
    print_orario(0);
    esito = trovato ? "trovato" : fermato ? "limite di nodi" : "nessuna soluzione";
    if (!trovato && !fermato) foto_pos = 0;  // nessuna soluzione: non resta niente da mostrare
    scrivi_stato(0);
    return 0;
}
