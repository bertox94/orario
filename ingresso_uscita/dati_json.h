#ifndef UNTITLED_DATI_JSON_H
#define UNTITLED_DATI_JSON_H

// Lettura dei dati da un file JSON (opzione --input), al posto dei dati scritti in inizializza_dati().
//
// Fa quello che fa inizializza_dati() in data_initialization/database_initializers.c: registra giorni e ore, prepara
// la coda, poi per ogni lezione, nello stesso ordine del file, registra classe e docenti (alla loro prima comparsa) e
// chiama set_classe_insegnante_ore / set_classe_insegnante_ore_cod, poi le ore di ingresso, poi gli stessi controlli
// finali sulle ore delle classi. Cambia solo da dove arrivano i valori. Il calcolo dell'orario non e' toccato.
//
// Formato:
// {
//   "giorni": ["Lun", "Mar", "Mer", "Gio", "Ven", "Sab"],
//   "ore": ["08", "09", "10", "11", "12", "13"],
//   "lezioni": [
//     {"classe": "3A", "docente": "D. Malfoy", "ore": 2},
//     {"classe": "3A", "docente": "Lupin", "ore": 5, "codocente": "G. Vermilinguo", "ore_codocenza": 3}
//   ],
//   "ingressi": [{"docente": "Naruto", "ora": 1}]
// }
// Una lezione con codocente e' come set_classe_insegnante_ore_cod(classe, docente, ore, codocente, ore_codocenza):
// il docente ha `ore` ore nella classe, di cui `ore_codocenza` insieme al codocente. "ora" e' l'indice dell'ora di
// ingresso (0 = prima ora).

#include <stddef.h>

// Ritorna 1 se i dati sono stati letti; 0 con il motivo in `errore`.
int inizializza_dati_da_json(const char *percorso, char *errore, size_t dimensione);

#endif
