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

void scrivi_risultato(const char *percorso, const char *esito, const char *errore, long long int nodi, double secondi,
                      long long int orari, const teaching_slot *orario, int celle);

#endif //UNTITLED_RISULTATO_H
