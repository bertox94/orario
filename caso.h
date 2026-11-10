#ifndef UNTITLED_CASO_H
#define UNTITLED_CASO_H

// Un caso casuale seguito dal sito: ./untitled caso SEME CLASSI RITARDI LIMITE FILE
//
// Genera i dati come le prove (data_initialization/generatore.c: stesso seme, stessi dati), cerca un orario fermandosi
// a LIMITE nodi (0: senza limite) e scrive lo stato della ricerca in FILE, un oggetto JSON, ogni mezzo secondo e alla
// fine (scritto in FILE.tmp e poi rinominato: chi legge trova sempre un JSON intero):
// {
//   "in_corso": 1 | 0,
//   "esito": "in corso" | "trovato" | "limite di nodi" | "nessuna soluzione" | "dati non generati",
//   "nodi": N, "secondi": S, "pos": P, "pos_max": M, "celle": C,
//   "giorni": [...], "ore": [...], "classi": [...],
//   "orario": [[docente, codocente], ...]   le prime P celle, quelle gia' decise (null: nessuno)
// }
// La cella k e' giorno = k / (ore * classi), ora = (k / classi) % ore, classe = k % classi. A ricerca finita l'orario
// e' quello trovato (tutte le celle), o quello del nodo dove si e' fermata al limite.

int esegui_caso(unsigned long long int seme, int classi, int ritardi, long long int limite, const char *file);

#endif //UNTITLED_CASO_H
