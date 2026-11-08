#ifndef UNTITLED_RISULTATO_H
#define UNTITLED_RISULTATO_H

// Il risultato della ricerca per il sito (opzione --risultato FILE), un oggetto JSON:
// {
//   "esito": "trovato" | "limite di nodi" | "nessuna soluzione" | "errore nei dati",
//   "errore": "...",                     (solo con "errore nei dati")
//   "nodi": N, "secondi": S, "orari": quanti orari trovati,
//   "giorni": [...], "ore": [...], "classi": [...],
//   "orario": [[docente, codocente], ...] (il primo orario trovato, una cella per elemento nell'ordine della ricerca:
//              cella = (giorno * ore + ora) * classi + classe; null dove non c'e' docente; senza orario: null)
// }

#include "../data_initialization/database_initializers.h"

// Lo stato della ricerca mentre gira (opzione --stato FILE), per il sito: {"nodi": N, "pos": P, "pos_max": M,
// "celle": totale, "orario": [[docente, codocente], ...]} con le prime P celle, quelle gia' decise (la ricerca sta
// decidendo la cella P). Si scrive in un file provvisorio e poi si rinomina: chi legge trova sempre un JSON intero.
void scrivi_stato(const char *percorso, long long int nodi, int pos, int pos_max, const teaching_slot *orario,
                  int celle);

void scrivi_risultato(const char *percorso, const char *esito, const char *errore, long long int nodi, double secondi,
                      long long int orari, const teaching_slot *orario, int celle);

#endif //UNTITLED_RISULTATO_H
