#ifndef UNTITLED_OSTACOLI_H
#define UNTITLED_OSTACOLI_H

// I backtrack di una ricerca, per il sito: ./untitled ostacoli PASSI FASCE QUANTI
//
// Rilegge il file dei passi (passi.h) dall'inizio alla fine, senza rifare la ricerca: di ogni nodo guarda solo la
// cella che sta per decidere. Solo lettura: la ricerca non c'entra.
//
// La ricerca torna indietro da un nodo quando il nodo dopo decide una cella che non viene dopo la sua: li' si e'
// incagliata (nessuna scelta, o una potatura), ed e' tornata a cambiare la cella prima di quella del nodo dopo.
// Un OSTACOLO e' una cella da cui la ricerca torna indietro, dal primo ritorno finche' non riesce ad andare oltre
// quella cella: tutti i ritorni dalla stessa cella nel frattempo sono lo stesso ostacolo. Di un ostacolo si dice:
// - la cella (da 0) e la profondita': di quante celle la ricerca e' dovuta tornare indietro, al massimo, prima di
//   superarla (la cella meno la piu' bassa che ha cambiato);
// - da quale nodo a quale, quanti nodi ci sono voluti e quanti ritorni da quella cella;
// - se alla fine del file non e' ancora superato.
// Gli ostacoli stanno uno dentro l'altro: mentre risale verso una cella la ricerca puo' incagliarsi in una prima, e
// quello e' un ostacolo dentro l'altro, che riguarda una cella piu' bassa e si chiude prima.
//
// Stampa su stdout, in JSON:
// - "passi": quanti nodi ha il file; "ritorni": quante volte la ricerca e' tornata indietro; "ostacoli": quanti
//   ostacoli in tutto; "cella_massima": la cella piu' avanti;
// - "fasce": il file diviso in FASCE parti uguali (meno, se i nodi sono meno): per ognuna [la cella piu' bassa, la
//   piu' alta] decise in quei nodi: e' il profilo della ricerca, quanto avanti arriva e quanto torna indietro;
// - "lunghi": i QUANTI ostacoli che hanno richiesto piu' nodi, dal piu' lungo: {"cella", "profondita", "dal", "al"
//   (null: non ancora superato), "passi", "ritorni"}; i nodi si contano da 1, come nel comando passo.
// Ritorna 0, oppure 1 se il file non si legge.
int stampa_ostacoli(const char *file_passi, int fasce, int quanti);

#endif //UNTITLED_OSTACOLI_H
