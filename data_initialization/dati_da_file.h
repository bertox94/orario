#ifndef UNTITLED_DATI_DA_FILE_H
#define UNTITLED_DATI_DA_FILE_H

// Dati letti da un file JSON (opzione --input) invece che scritti nel programma. Il formato e' quello della sezione
// "dati" del JSON che il solver scrive con --json (vedi printer/risultati_json.h), cosi' l'uscita di un'esecuzione
// si puo' rileggere come ingresso:
// {
//   "giorni":  ["Lun", ...],                     da 1 a GIORNO_SETTIMANA_SZ
//   "ore":     ["08", ...],                      da PRIME_N_ORE_OBBLIGATORIE a ORA_GIORNALIERA_SZ
//   "classi":  [{"nome": "3A"}, ...],            "ore" si puo' omettere: si ricava dalle lezioni
//   "docenti": [{"nome": "Rossi", "ingresso": 0}, ...]   ingresso = indice della prima ora (default 0)
//   "lezioni": [{"classe": 0, "docente": 3, "codocente": null, "ore": 2}, ...],
//   "ordine_coda": [3, 0, ...]                    facoltativo, vedi sotto
// }
// La coda dei docenti parte nell'ordine di "ordine_coda" se c'e' (lo scrive --json: cosi' un caso riletto rifa'
// esattamente la stessa ricerca), altrimenti nell'ordine in cui i docenti compaiono nelle lezioni.
// Le lezioni indicano classe e docenti con l'indice negli elenchi sopra. codocente null = il docente da solo;
// altrimenti "ore" sono le ore in cui i due docenti sono insieme. Accetta anche un file con altre sezioni (come
// quello scritto da --json): legge "dati" se c'e', altrimenti l'oggetto intero.

#include <stddef.h>

// Carica i dati dal file. Ritorna 1 se tutto va bene, altrimenti 0 con il motivo in `errore`.
// Va chiamata al posto di inizializza_dati(), su dati ancora vuoti.
int carica_dati_da_file(const char *file, char *errore, size_t dimensione);

#endif //UNTITLED_DATI_DA_FILE_H
