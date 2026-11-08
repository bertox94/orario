#ifndef UNTITLED_POTATURE_H
#define UNTITLED_POTATURE_H

// Le potature: condizioni necessarie sul resto della settimana, oltre ai controlli sul singolo docente (quelli di
// print_orario, che guardano solo la cella che si sta decidendo). Vengono dal solver precedente (archivio/2026-11-04,
// ricerca/vincoli.c). Se una non vale, il ramo non ha soluzioni e la ricerca torna indietro subito, invece di scoprirlo
// molte celle dopo con milioni di backtrack.
//
// Perche' sono necessarie: ogni controllo confronta quello che resta da fare con un massimo che nessun orario puo'
// superare (le ore dal proprio ingresso, al massimo 3 ore al giorno in una classe, ore contigue, una classe per ora).
// Quindi scartano solo rami senza soluzioni: gli orari trovati, e il loro ordine, sono gli stessi di prima; cambia solo
// quanto presto la ricerca torna indietro (verificato: stesso primo orario su tutti i casi risolti).
//
// Quando avvengono: subito dopo aver messo una scelta nella cella del passo s - 1, nello stesso nodo, prima di scendere
// alla cella dopo (dall'8 novembre 2026, richiesta di Alb: i controlli generali della scelta stanno nella sua cella; prima
// si facevano entrando nella cella dopo, che contava un nodo in piu'). Si chiamano con potatura_dopo(s):
//
//   A OGNI CELLA, per la classe della cella appena decisa (dipendono solo da quella classe):
//     L. lezioni della classe: le lezioni che le restano (un docente da solo o una coppia) bastano a riempire le sue
//        ore obbligatorie, oggi e in ogni giorno che resta, e fino all'ultima ora tutte le ore che le restano;
//     C. ore dei docenti nella classe: a ogni suo docente le ore che restano li' stanno in quelle che puo' ancora fare
//        (oggi: dal suo ingresso, al massimo 3, solo se il suo blocco di oggi non si e' chiuso; poi 3 al giorno).
//
//   A OGNI ORA CHIUSA (decisa in tutte le classi) E DOPO OGNI ORA BUCA, per tutte le classi e tutti i docenti
//   (il tempo passa per tutti; un'ora buca chiude la giornata della classe e toglie ore ai suoi docenti):
//     C. come sopra, per ogni classe;
//     T. ore dei docenti in tutto: le ore che restano a ogni docente stanno nelle ore libere che gli restano (oggi:
//        dal suo ingresso, dove non insegna gia' e c'e' ancora una classe da decidere; poi tutte le ore dal suo
//        ingresso dei giorni dopo).
//
//   A OGNI CELLA, alla fine (se le altre passano):
//     A. il controllo in avanti (controllo_in_avanti/controllo_in_avanti.h): le celle obbligatorie di oggi hanno ancora
//        un docente possibile, le lezioni delle classi bastano, i docenti hanno abbastanza tempo.
//
// (La verifica sui soli dati, prima della ricerca, e' in controllo_in_avanti/: verifica_fattibilita.)
//
// Si scrivono sulle celle gia' decise, quindi valgono per qualunque ordine della ricerca (main: un'ora alla volta;
// per-classe: una classe alla volta dentro il giorno): per ogni classe conta la sua prima cella non ancora decisa.

// Da chiamare una volta, dopo aver letto i dati. cella_di[s] e' la cella decisa al passo s della ricerca (NULL: la
// cella s, come in main).
void prepara_potature(const int *cella_di);

// Per un solver che riempie i giorni in un altro ordine (giorni-liberi): giorno_di_posto[r] e' il giorno che si
// riempie per r-esimo, posto_di_giorno[g] il suo posto. I "giorni dopo" sono quelli che si riempiono dopo. Senza
// chiamarla (o con NULL) e' l'ordine del calendario, come in main.
void potature_ordine_giorni(const int *giorno_di_posto, const int *posto_di_giorno);

// All'ingresso del passo s (s > 0): ritorna 0 se il resto e' ancora possibile, altrimenti quale controllo non vale
// (POTATURA_ORE_DOCENTI per C e T, POTATURA_LEZIONI_CLASSE per L), con i numeri in `potatura`.
enum { POTATURA_ORE_DOCENTI = 1, POTATURA_LEZIONI_CLASSE = 2, POTATURA_IN_AVANTI = 3 };
int potatura_dopo(int s);

// I numeri dell'ultima potatura scattata (per la traccia).
typedef struct {
    int controllo;              // POTATURA_ORE_DOCENTI, POTATURA_LEZIONI_CLASSE o POTATURA_IN_AVANTI
    int docente, classe;        // ore dei docenti: il docente, la classe (-1: in tutto); lezioni: la classe
    int restano, possibili;     // ore che restano al docente / ore che puo' ancora fare; lezioni: da riempire / possibili
    int fino_ora;               // lezioni: l'ora (fino a questa, ogni giorno) per cui le lezioni non bastano
    int giorno, ora;            // la prima cella non decisa della classe (L, C); T: il giorno, ora -1; A: la cella
    int cella, motivo;          // A: la cella che non si potra' riempire e il motivo (vedi controllo_in_avanti.h)
} Potatura;
extern Potatura potatura;

#endif //UNTITLED_POTATURE_H
