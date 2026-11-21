#include "scuola.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// l'indice di un nome in un elenco, aggiungendolo in fondo se manca (-1: l'elenco e' pieno o il nome troppo lungo)
static int indice(char nomi[][MAX_NOME], int *quanti, const int massimo, const char *nome, const int aggiungi) {
    for (int i = 0; i < *quanti; i++)
        if (!strcmp(nomi[i], nome)) return i;
    if (!aggiungi || *quanti == massimo || strlen(nome) >= MAX_NOME) return -1;
    strcpy(nomi[*quanti], nome);
    return (*quanti)++;
}

// Aggiunge ore alla voce (classe, docenti); se non c'e' la crea. I due docenti di una coppia stanno in ordine di
// indice, cosi' la stessa coppia e' sempre la stessa voce.
static int aggiungi_voce(Scuola *s, const int classe, int a, int b, const int ore) {
    if (b >= 0 && b < a) {
        const int t = a;
        a = b, b = t;
    }
    for (int v = 0; v < s->voci; v++)
        if (s->voce[v].classe == classe && s->voce[v].docente[0] == a && s->voce[v].docente[1] == b) {
            s->voce[v].ore += ore;
            return 1;
        }
    if (s->voci == MAX_VOCI) return 0;
    s->voce[s->voci++] = (Voce){classe, {a, b}, ore};
    return 1;
}

// l'ordine delle voci: per classe, poi per docente, poi per codocente (per qsort)
static int per_classe(const void *a, const void *b) {
    const Voce *x = a, *y = b;
    if (x->classe != y->classe) return x->classe - y->classe;
    if (x->docente[0] != y->docente[0]) return x->docente[0] - y->docente[0];
    return x->docente[1] - y->docente[1];
}

// Legge un caso riga per riga (il formato e' in LEGGIMI.md), poi controlla che ci sia tutto e mette le voci in
// ordine di classe.
Scuola *leggi_scuola(const char *percorso, char *errore, const int lunga) {
    FILE *f = fopen(percorso, "r");
    Scuola *s = calloc(1, sizeof(Scuola));
    if (!f || !s) {
        snprintf(errore, lunga, "non riesco a leggere %s", percorso);
        free(s);
        return NULL;
    }
    char riga[1024];
    int numero = 0;  // il numero della riga, per dire dove e' l'errore
    int ok = 1;      // diventa 0 alla prima riga che non va, e li' ci si ferma
    while (ok && fgets(riga, sizeof riga, f)) {
        numero++;
        // la riga divisa nei suoi campi, separati da TAB (strtok mette uno zero al posto di ogni TAB)
        char *campo[64];
        int n = 0;
        for (char *c = strtok(riga, "\t\r\n"); c && n < 64; c = strtok(NULL, "\t\r\n")) campo[n++] = c;
        if (!n || campo[0][0] == '#') continue;  // riga vuota, o commento
        // il primo campo dice che cosa c'e' nella riga
        if (!strcmp(campo[0], "giorni")) {
            for (int i = 1; i < n && ok; i++) ok = indice(s->nome_giorno, &s->giorni, MAX_GIORNI, campo[i], 1) >= 0;
        } else if (!strcmp(campo[0], "ore")) {
            for (int i = 1; i < n && ok; i++) ok = indice(s->nome_ora, &s->ore, MAX_ORE, campo[i], 1) >= 0;
        } else if (!strcmp(campo[0], "obbligatorie") && n == 2) {
            s->obbligatorie = atoi(campo[1]);
        } else if (!strcmp(campo[0], "docente") && n == 3) {
            const int t = indice(s->nome_docente, &s->docenti, MAX_DOCENTI, campo[1], 1);
            if ((ok = t >= 0)) s->ingresso[t] = atoi(campo[2]);
        } else if (!strcmp(campo[0], "lezione") && (n == 4 || n == 5)) {
            // classe, ore, docente e forse codocente. La classe nasce qui la prima volta che compare; i docenti
            // devono esserci gia' (una riga «docente» prima), e i due di una coppia devono essere diversi.
            const int k = indice(s->nome_classe, &s->classi, MAX_CLASSI, campo[1], 1);
            const int a = indice(s->nome_docente, &s->docenti, MAX_DOCENTI, campo[3], 0);
            const int b = n == 5 ? indice(s->nome_docente, &s->docenti, MAX_DOCENTI, campo[4], 0) : -1;
            ok = k >= 0 && a >= 0 && (n == 4 || (b >= 0 && b != a)) && atoi(campo[2]) > 0
                 && aggiungi_voce(s, k, a, b, atoi(campo[2]));
        } else ok = 0;
    }
    fclose(f);
    if (ok && (!s->giorni || !s->ore || !s->classi || !s->voci)) {
        snprintf(errore, lunga, "%s: mancano giorni, ore o lezioni", percorso);
        ok = 0;
    } else if (!ok)
        snprintf(errore, lunga, "%s, riga %d: non la capisco, o supera i limiti di scuola.h", percorso, numero);
    for (int t = 0; ok && t < s->docenti; t++)
        if (s->ingresso[t] < 0 || s->ingresso[t] >= s->ore) {
            snprintf(errore, lunga, "%s: l'ingresso di %s non e' un'ora della giornata", percorso, s->nome_docente[t]);
            ok = 0;
        }
    if (!ok) {
        free(s);
        return NULL;
    }
    s->obbligatorie = MAX(0, MIN(s->obbligatorie, s->ore));
    // Le voci in ordine di classe, e per ogni classe dove cominciano le sue: andando dall'ultima voce alla prima,
    // in prima_voce resta la piu' bassa di ogni classe. Una in piu' in fondo, cosi' le voci della classe k sono
    // sempre quelle da prima_voce[k] a prima_voce[k + 1] escluso.
    qsort(s->voce, s->voci, sizeof(Voce), per_classe);
    for (int v = s->voci - 1; v >= 0; v--) s->prima_voce[s->voce[v].classe] = v;
    s->prima_voce[s->classi] = s->voci;
    return s;
}
