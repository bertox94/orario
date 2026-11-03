#ifndef UNTITLED_RISULTATI_JSON_H
#define UNTITLED_RISULTATI_JSON_H

// Risultati di un'esecuzione in JSON (opzione --json), per il sito che lancia le prove e ne mostra i risultati.
//
// Struttura (le tabelle per slot sono sempre [giorno][ora][classe], con la classe come indice in dati.classi,
// indipendentemente dall'ordine in cui la ricerca le visita):
// {
//   "versione": 1,
//   "argomenti": [...],                         argomenti della riga di comando
//   "dati": {
//     "giorni": [...], "ore": [...],
//     "classi":  [{"nome", "ore"}],
//     "docenti": [{"nome", "ore", "ingresso"}],   ingresso = indice della prima ora in cui puo' insegnare
//     "lezioni": [{"classe", "docente", "codocente", "ore"}]   codocente null = da solo
//     "ordine_coda": [...]                       ordine iniziale della coda dei docenti (per --input)
//   },
//   "ordine_classi": [...],                     in che ordine la ricerca visita le classi dentro ogni ora
//   "impossibile": null | "motivo",              motivo della verifica preliminare (allora non si e' cercato)
//   "esito": {"orari_trovati", "interrotto_per_tempo", "interrotto_per_spazio", "errore_di_scrittura",
//             "interrotto_su_richiesta", "secondi",
//             "nodi", "marce_indietro",
//             "posizione_massima", "slot"},
//   "orario": null | [g][o][c] = [docente, codocente]   primo orario trovato; -1 = nessuno (slot vuoto o da solo)
//   "diagnosi": {
//     "marce": [g][o][c],                        marce indietro in ogni slot
//     "scartati": {"<motivo>": [g][o][c]},       rami scartati dalla potatura dopo ogni slot, per motivo
//     "nomi_motivi": [...],
//     "motivi": null | [g][o][c][m],             con --motivi: perche' i docenti sono stati scartati nello slot
//     "motivi_docenti": null | [d][m]            con --motivi: perche' ogni docente e' stato scartato
//   }
// }

// In piu', dopo "diagnosi":
//   "dettagli_slot": [{"giorno", "ora", "classe", "visite", "ore_classe", "marce",
//                      "ultima_marcia": {"ore_classe_fatte", "docenti_con_ore"},          se ci sono marce indietro
//                      "scarti": [{"motivo", "volte", "ore_classe_fatte",
//                                  "fino_ora", "da_riempire", "possibili"                 lezioni della classe
//                                  | "docente", "classe_docente", "ore_rimaste", "ore_possibili"}]}]   ore dei docenti
//   uno per ogni slot con blocchi: quante volte la ricerca ci e' arrivata e lo stato all'ultimo blocco (ore della classe
//   gia' in orario, e per i rami scartati i numeri della verifica che li ha scartati; classe_docente -1 = ore in tutto).

// Scrive i risultati in `file` (se NULL non fa nulla). impossibile = motivo della verifica preliminare, oppure NULL
// se si e' cercato; secondi = durata (tempo di CPU).
void scrivi_json(const char *file, int argc, char **argv, const char *impossibile, double secondi);

#endif //UNTITLED_RISULTATI_JSON_H
