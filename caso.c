#include "caso.h"
#include "ricerca.h"
#include "data_initialization/generatore.h"
#include "passi.h"
#include "verifica_dati.h"
#include "verifica_orario.h"
#include "json.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *file_stato;
static clock_t inizio, ultima_scrittura;
static const char *esito = "in corso";
// la verifica indipendente dell'orario trovato (verifica_orario.h): c'e' solo con l'esito «trovato»
static ControlloOrario verifica[VERIFICA_ORARIO_CONTROLLI];
static int verificato = -1;  // -1: non fatta, 1: l'orario e' valido, 0: no
static char errore[VERIFICA_DATI_ERRORE];  // il motivo, quando l'esito e' «errore nei dati»
// la foto dell'orario all'ultimo nodo scritto: durante la ricerca orario[] cambia, la foto no
static teaching_slot foto[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
static int foto_pos = 0;

static void elenco(FILE *r, const char *campo, char nomi[][MAX_LEN_WORDS], const int n) {
    fprintf(r, ",\"%s\":[", campo);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', r);
        json_stringa(r, nomi[i]);
    }
    fputc(']', r);
}

static void scrivi_stato(const int in_corso) {
    char provvisorio[1024];
    snprintf(provvisorio, sizeof provvisorio, "%s.tmp", file_stato);
    FILE *r = fopen(provvisorio, "w");
    if (!r) return;
    fprintf(r, "{\"in_corso\":%d,\"esito\":", in_corso);
    json_stringa(r, esito);
    if (errore[0]) {
        fputs(",\"errore\":", r);
        json_stringa(r, errore);
    }
    if (verificato >= 0) {
        fprintf(r, ",\"verifica\":{\"ok\":%d,\"controlli\":[", verificato);
        for (int i = 0; i < VERIFICA_ORARIO_CONTROLLI; i++) {
            fputs(i ? ",[" : "[", r);
            json_stringa(r, verifica[i].nome);
            fprintf(r, ",%d,", verifica[i].ok);
            json_stringa(r, verifica[i].dettaglio);
            fputc(']', r);
        }
        fputs("]}", r);
    }
    fprintf(r, ",\"nodi\":%lld,\"secondi\":%.2f,\"pos\":%d,\"pos_max\":%d,\"celle\":%d", nodi,
            (clock() - inizio) / (double) CLOCKS_PER_SEC, foto_pos, pos_max, orario_len);
    elenco(r, "giorni", giorni_settimana, actual_giorni_settimana);
    elenco(r, "ore", ore_giornaliere, actual_ore_giornaliere);
    elenco(r, "classi", classe_sezione, actual_classi_sezione);
    fputs(",\"orario\":[", r);
    for (int i = 0; i < foto_pos; i++) {
        fputs(i ? ",[" : "[", r);
        json_nome(r, foto[i].insegnante1 < 0 ? NULL : insegnanti[foto[i].insegnante1]);
        fputc(',', r);
        json_nome(r, foto[i].insegnante2 < 0 ? NULL : insegnanti[foto[i].insegnante2]);
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
    passi_nodo(nodi, pos, orario, orario_len);
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
                const char *file, const char *file_passi) {
    file_stato = file;
    if (file_passi && !passi_apri(file_passi)) return 2;
    inizio = ultima_scrittura = clock();
    if (!genera_dati_casuali(seme, classi, ritardi)) {
        esito = "dati non generati";
        scrivi_stato(0);
        return 1;
    }
    orario_len = actual_giorni_settimana * actual_ore_giornaliere * actual_classi_sezione;
    if (!verifica_dati(errore, sizeof errore)) {  // l'orario non puo' esistere: la ricerca non parte
        passi_chiudi();
        esito = "errore nei dati";
        scrivi_stato(0);
        return 0;
    }
    limite_nodi = limite;
    ogni_nodo = segui;
    scrivi_stato(1);
    print_orario(0);
    passi_chiudi();
    esito = trovato ? "trovato" : fermato ? "limite di nodi" : "nessuna soluzione";
    // l'orario trovato e' la foto (fatta quando la ricerca ha deciso l'ultima cella): finita la ricerca orario[]
    // non lo contiene piu', perche' tornando indietro le celle vengono riscritte
    if (trovato) verificato = verifica_orario(foto, verifica);
    if (!trovato && !fermato) foto_pos = 0;  // nessuna soluzione: non resta niente da mostrare
    scrivi_stato(0);
    return 0;
}
