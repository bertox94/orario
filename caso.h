#ifndef UNTITLED_CASO_H
#define UNTITLED_CASO_H

// Un caso casuale seguito dal sito: ./untitled caso SEME CLASSI RITARDI ORDINE LIMITE FILE PASSI
//
// Genera i dati come le prove (data_initialization/generatore.c: stesso seme, stesse classi, stessi ritardi e stesso
// ordine delle classi: stessi dati), cerca un orario fermandosi
// a LIMITE nodi (0: senza limite) e scrive lo stato della ricerca in FILE, un oggetto JSON, ogni mezzo secondo e alla
// fine (scritto in FILE.tmp e poi rinominato: chi legge trova sempre un JSON intero):
// {
//   "in_corso": 1 | 0,
//   "esito": "in corso" | "trovato" | "limite di nodi" | "nessuna soluzione" | "errore nei dati" |
//            "dati non generati",
//   "errore": "..."   solo con «errore nei dati»: il controllo che non vale (verifica_dati.h); la ricerca non parte
//   "verifica": {"ok": 1 | 0, "controlli": [[nome, 1 | 0, dettaglio], ...]}   solo con «trovato»: la verifica
//            indipendente dell'orario (verifica_orario.h); il dettaglio e' la prima cosa che non va
//   "nodi": N, "secondi": S, "pos": P, "pos_max": M, "celle": C,
//   "giorni": [...], "ore": [...], "classi": [...],
//   "orario": [[docente, codocente], ...]   le prime P celle, quelle gia' decise (null: nessuno)
// }
// La cella k e' giorno = k / (ore * classi), ora = (k / classi) % ore, classe = k % classi. A ricerca finita l'orario
// e' quello trovato (tutte le celle), o quello del nodo dove si e' fermata al limite.
// In PASSI scrive ogni nodo della ricerca, per lo slider del sito (vedi passi.h).

int esegui_caso(unsigned long long int seme, int classi, int ritardi, int ordine, long long int limite,
                const char *file, const char *file_passi);

#endif //UNTITLED_CASO_H
