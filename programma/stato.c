#include "stato.h"

#include "../verifica/verifica.h"

// I nomi degli esiti e dei motivi, nell'ordine dei numeri di ricerca.h (Esito, TipoMotivo): nello stato si scrivono
// in parole, e il sito li usa cosi'.
static const char *const ESITI[] = {"in corso", "trovato", "impossibile", "limite di nodi"};
static const char *const MOTIVI[] = {"", "ore_docente", "ore_docente_in_classe", "ore_classe", "prime_ore",
                                     "docente_senza_posto", "classe_senza_scelte", "orario_della_classe"};

// un testo tra virgolette, con la barra davanti alle virgolette e alle barre che contiene
static void json_testo(FILE *f, const char *testo) {
    fputc('"', f);
    for (; *testo; testo++) {
        if (*testo == '"' || *testo == '\\') fputc('\\', f);
        fputc(*testo, f);
    }
    fputc('"', f);
}

// "chiave": ["nome", "nome", ...], con la virgola in fondo
static void json_nomi(FILE *f, const char *chiave, const char nomi[][MAX_NOME], const int n) {
    fprintf(f, "\"%s\":[", chiave);
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', f);
        json_testo(f, nomi[i]);
    }
    fputs("],", f);
}

// "scuola": {...}: i nomi, le due regole della giornata (servono ai testi del sito), i docenti con l'ingresso, le voci
void scrivi_la_scuola(FILE *f, const Scuola *s) {
    fputs("\"scuola\":{", f);
    json_nomi(f, "giorni", s->nome_giorno, s->giorni);
    json_nomi(f, "ore", s->nome_ora, s->ore);
    json_nomi(f, "classi", s->nome_classe, s->classi);
    fprintf(f, "\"obbligatorie\":%d,\"ore_di_seguito\":%d,\"classi_al_giorno\":%d,\"docenti\":[", s->obbligatorie,
            MAX_ORE_DI_SEGUITO, MAX_CLASSI_AL_GIORNO);
    for (int t = 0; t < s->docenti; t++) {
        fputs(t ? ",[" : "[", f);
        json_testo(f, s->nome_docente[t]);
        fprintf(f, ",%d]", s->ingresso[t]);
    }
    fputs("],\"voci\":[", f);
    for (int v = 0; v < s->voci; v++)
        fprintf(f, "%s[%d,%d,%d,%d]", v ? "," : "", s->voce[v].classe, s->voce[v].docente[0], s->voce[v].docente[1],
                s->voce[v].ore);
    fputs("]}", f);
}

// "orario": [...], "verifica": {...}: un orario completo e la sua verifica, fatta adesso, regola per regola
void scrivi_orario_e_verifica(FILE *f, const Scuola *s, const int *orario) {
    Controllo controllo[CONTROLLI];
    const int ok = verifica_orario(s, orario, controllo), celle = s->giorni * s->ore * s->classi;
    fputs("\"orario\":[", f);
    for (int i = 0; i < celle; i++) fprintf(f, "%s%d", i ? "," : "", orario[i]);
    fprintf(f, "],\"verifica\":{\"ok\":%d,\"controlli\":[", ok);
    for (int i = 0; i < CONTROLLI; i++) {
        fputs(i ? ",[" : "[", f);
        json_testo(f, controllo[i].regola);
        fprintf(f, ",%d,", controllo[i].rispettata);
        json_testo(f, controllo[i].dove);
        fputc(']', f);
    }
    fputs("]}", f);
}

// Lo stato della ricerca (com'e' fatto e' scritto in stato.h): sempre i numeri e la scuola; a ricerca finita
// anche il motivo, se un orario non c'era gia' in partenza, oppure l'orario trovato con la sua verifica.
void scrivi_stato(FILE *f, const Ricerca *r, const double secondi, const int a_mano) {
    const Scuola *s = r->s;
    // i numeri della ricerca
    fprintf(f, "{\"a_mano\":%d,", a_mano);
    fprintf(f, "\"in_corso\":%d,\"esito\":\"%s\",\"nodi\":%lld,\"secondi\":%.2f,\"profondita_massima\":%d,"
            "\"celle\":%d,\"ritorni\":%lld,\"ritorni_per_cella\":[", r->esito == IN_CORSO, ESITI[r->esito], r->nodi,
            secondi, r->profondita_massima, r->celle, r->ritorni);
    // i ritorni: per cella, per cella dall'ultimo passo avanti, e per motivo (qui solo i posti che ne hanno)
    for (int i = 0; i < r->celle; i++) fprintf(f, "%s%lld", i ? "," : "", r->ritorni_in[i]);
    fputs("],\"ritorni_recenti_per_cella\":[", f);
    for (int i = 0; i < r->celle; i++) fprintf(f, "%s%lld", i ? "," : "", r->ritorni_recenti_in[i]);
    fputs("],\"ritorni_per_motivo\":[", f);
    for (int i = 0, primo = 1; i < posti_dei_motivi(s); i++) {
        if (!r->ritorni_per[i]) continue;
        const int chi = i % (s->docenti + s->classi);  // prima i docenti, poi le classi
        fprintf(f, "%s[\"%s\",%d,%d,%lld,%lld]", primo ? "" : ",", MOTIVI[i / (s->docenti + s->classi)],
                chi < s->docenti ? chi : -1, chi < s->docenti ? -1 : chi - s->docenti, r->ritorni_per[i],
                r->ritorni_recenti_per[i]);
        primo = 0;
    }
    fputs("],", f);
    scrivi_la_scuola(f, s);
    // impossibile gia' al primo nodo: il conto che non torna
    if (r->esito == IMPOSSIBILE && r->motivo_radice.tipo != NESSUNO) {
        const Motivo *m = &r->motivo_radice;
        fprintf(f, ",\"motivo\":{\"tipo\":\"%s\",\"docente\":%d,\"classe\":%d,\"ora\":%d,\"servono\":%d,"
                "\"possibili\":%d}", MOTIVI[m->tipo], m->docente, m->classe, m->ora, m->servono, m->possibili);
    }
    // trovato: l'orario, e la sua verifica fatta adesso, regola per regola
    if (r->esito == TROVATO) {
        fputc(',', f);
        scrivi_orario_e_verifica(f, s, r->orario);
    }
    fputs("}\n", f);
}
