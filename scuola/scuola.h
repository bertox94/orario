#ifndef SCUOLA_H
#define SCUOLA_H

// La scuola: giorni, ore, classi, docenti e le lezioni da mettere in orario.
//
// Una VOCE e' una lezione di una classe: un docente da solo, oppure una coppia (docente e codocente insieme), con le
// ore che devono fare in una settimana. In una cella dell'orario va una voce, oppure un'ora buca.
//
// Le regole dell'orario (le controlla verifica.c, e la ricerca le rispetta):
// - ogni voce fa esattamente le sue ore, nella sua classe;
// - un docente e' in una sola classe per volta, e non prima della sua ora di ingresso;
// - in una classe, in un giorno, un docente fa al massimo MAX_ORE_DI_SEGUITO ore, e di seguito;
// - in un giorno un docente insegna in al massimo MAX_CLASSI_AL_GIORNO classi;
// - ogni classe ha lezione nelle prime `obbligatorie` ore di ogni giorno, e dopo un'ora buca quel giorno ha finito.

// Quanto puo' essere grande una scuola: le tabelle della Scuola hanno queste misure, fisse. Un caso che le supera
// viene rifiutato da leggi_scuola.
#define MAX_GIORNI 7
#define MAX_ORE 10
#define MAX_CLASSI 120
#define MAX_DOCENTI 250
#define MAX_VOCI 4000
#define MAX_NOME 48

// Le due regole della giornata di un docente (vedi sopra). Cambiarle cambia la ricerca e i risultati di riferimento.
#define MAX_ORE_DI_SEGUITO 3
#define MAX_CLASSI_AL_GIORNO 3

// Il piu' piccolo e il piu' grande di due numeri. Sono macro: ogni argomento e' scritto due volte, quindi dentro
// non ci va mai una chiamata che fa qualcosa (per esempio che estrae un numero casuale).
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    // la classe della lezione (indice)
    int classe;
    int docente[2];  // docente[1] < 0: da solo
    // le ore in una settimana
    int ore;
} Voce;

typedef struct {
    int giorni, ore, classi, docenti, voci;
    int obbligatorie;  // le prime ore di ogni giorno, che hanno sempre lezione
    char nome_giorno[MAX_GIORNI][MAX_NOME], nome_ora[MAX_ORE][MAX_NOME];
    char nome_classe[MAX_CLASSI][MAX_NOME], nome_docente[MAX_DOCENTI][MAX_NOME];
    int ingresso[MAX_DOCENTI];        // l'ora (indice) da cui il docente e' a scuola
    Voce voce[MAX_VOCI];              // in ordine di classe
    int prima_voce[MAX_CLASSI + 1];   // le voci della classe k vanno da prima_voce[k] a prima_voce[k + 1] - 1
} Scuola;

// Legge un caso (il formato e' in LEGGIMI.md: una riga per giorni, ore, docenti e lezioni, campi separati da TAB).
// NULL, con il motivo in errore, se il file non si legge o non e' scritto bene. La scuola si libera con free.
Scuola *leggi_scuola(const char *percorso, char *errore, int lunga);

// quanti docenti ha la voce (1 o 2), e se il docente t e' uno dei suoi
static inline int docenti_della_voce(const Voce *v) { return v->docente[1] < 0 ? 1 : 2; }
static inline int nella_voce(const Voce *v, const int t) { return v->docente[0] == t || v->docente[1] == t; }

#endif
