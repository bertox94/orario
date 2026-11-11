#ifndef UNTITLED_RICERCA_H
#define UNTITLED_RICERCA_H

// La ricerca (ricerca.c), per chi la lancia: main.c con i dati di prova, prove.c e caso.c con i dati casuali.
//
// Tutto quello che la ricerca sa e cambia sta in una Ricerca: la scuola (che legge soltanto), l'orario, le ore gia'
// messe, la coda dei docenti, il conto dei nodi. Chi la lancia la crea con nuova_ricerca, puo' mettere un limite di
// nodi e una funzione da chiamare a ogni nodo, e chiama print_orario(r, 0).

#include "data_initialization/database_initializers.h"
#include "priority_queue/prio_queue.h"

typedef struct Ricerca Ricerca;

struct Ricerca {
    const Scuola *scuola;

    // l'orario: una cella per (giorno, ora, classe), la cella (giorno * ore + ora) * classi + classe
    teaching_slot orario[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * CLASSE_SEZIONE_SZ];
    int orario_len;  // giorni * ore * classi

    // le ore gia' messe nell'orario fino alla cella che la ricerca sta decidendo. Gli array sono piatti, con gli
    // stessi indici dei dati: classe, docente -> classe * INSEGNANTE_SZ + docente; giorno, ora, docente ->
    // (giorno * ORA_GIORNALIERA_SZ + ora) * INSEGNANTE_SZ + docente; giorno, classe, docente ->
    // (giorno * CLASSE_SEZIONE_SZ + classe) * INSEGNANTE_SZ + docente.
    int ore_classi_docenti_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];        // di ogni docente in ogni classe
    int ore_classi_docenti_alone_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];  // di cui da solo
    int ore_classe_svolte[CLASSE_SEZIONE_SZ];                                // di ogni classe
    // di ogni coppia in ogni classe (classe, docente con l'indice minore, l'altro)
    int ore_classi_codocenza_svolte[CLASSE_SEZIONE_SZ * INSEGNANTE_SZ * INSEGNANTE_SZ];
    // di un docente in una classe in un giorno
    int ore_fatte_da_un_prof_in_una_classe_in_giorno[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ * INSEGNANTE_SZ];
    // in quante classi diverse un docente ha insegnato in un giorno (giorno * INSEGNANTE_SZ + docente)
    int classi_fatte_da_un_prof_in_giorno[GIORNO_SETTIMANA_SZ * INSEGNANTE_SZ];
    // il docente insegna gia' in quel giorno e a quell'ora (in un'altra classe)
    int prof_gia_impegnato_giorno_ora[GIORNO_SETTIMANA_SZ * ORA_GIORNALIERA_SZ * INSEGNANTE_SZ];
    // ore buche gia' messe in un giorno in una classe (dopo un'ora buca la classe non ha piu' lezione quel giorno)
    int n_ore_buche[GIORNO_SETTIMANA_SZ * CLASSE_SEZIONE_SZ];

    // la coda dei docenti: un elemento per docente (queue_items[p] e' quello del docente p) e, per ogni cella, un
    // blocco di puntatori agli elementi dei docenti della sua classe (solo loro possono insegnare li'), che la cella
    // ordina quando la ricerca ci arriva. Ogni cella ha il suo blocco perche' mentre scorre il suo le celle dopo
    // riordinano il loro.
    QueueItem queue_items[INSEGNANTE_SZ];
    QueueItem **queue_mul;                   // il blocco della cella pos comincia a queue_mul[pos * queue_sz]
    int queue_sz;                            // quanto e' lungo un blocco: i docenti della classe che ne ha di piu'
    int queue_sz_classe[CLASSE_SEZIONE_SZ];  // quanti docenti ha ogni classe: i puntatori usati nei suoi blocchi

    // come e' finita
    int trovato;  // l'orario e' completo: e' in orario
    int fermato;  // si e' fermata al limite di nodi

    // il conto dei nodi (le chiamate di print_orario) e i limiti: non cambiano la ricerca, la fermano soltanto
    long long int nodi;         // nodi visitati
    long long int limite_nodi;  // dopo tanti nodi la ricerca si ferma (0: nessun limite)
    int pos_max;                // la cella piu' avanti raggiunta
    // se c'e', si chiama a ogni nodo, dopo averlo contato: la ricerca sta per decidere la cella pos e l'orario e'
    // deciso fino a pos - 1 (per seguirla dal sito, vedi caso.c). Non cambia la ricerca. di_chi: quello che serve a
    // chi la segue.
    void (*ogni_nodo)(Ricerca *r, int pos);
    void *di_chi;
};

// Una ricerca pronta a partire su questa scuola: orario vuoto, nessuna ora messa, la coda con le ore di ogni docente.
// (E' grande: sta nella memoria dinamica. libera_ricerca quando non serve piu'.)
Ricerca *nuova_ricerca(const Scuola *s);
void libera_ricerca(Ricerca *r);

// Decide la cella pos e scende alle celle dopo (print_orario(r, 0): tutto l'orario). Ritorna 1 se la ricerca e'
// finita (trovato o fermato dicono come): allora r->orario e' l'orario trovato, o quello del nodo dove si e' fermata.
// Ritorna 0 se non c'e' nessun orario.
int print_orario(Ricerca *r, int pos);

#endif //UNTITLED_RICERCA_H
