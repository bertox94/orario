# Orario scolastico: solver e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice del sito e messaggi di commit sono in italiano.

## Cosa c'è

- **Il solver** (C, `main.c`): ricerca con backtracking dell'orario. È il solver del 1° novembre 2026 (correzione del
  sabato, PR #1), rimesso in `main` l'8 novembre su richiesta di Alb, più l'elenco dei nomi di fantasia
  (`data_initialization/nomi_docenti.h`, non ancora usato) e l'ingresso e l'uscita per il sito (`ingresso_uscita/`):
  `--input` (dati da JSON), `--risultato` (esito, nodi, tempo e primo orario in JSON), `-n`, `--txt`, `--ferma-nodi`.
  Senza opzioni il solver fa esattamente quello che faceva prima. I dati di prova sono in
  `data_initialization/database_initializers.c` (`inizializza_dati()`).
- **Il sito** (ramo `sito/quarta-versione`): solo il test in tempo reale, con questo solver. Vedi il suo LEGGIMI.
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

## Branch

- `main`: il solver.
- `sito/quarta-versione`: il sito semplice (dall'8 novembre 2026). Il Raspberry segue ancora `sito/terza-versione`
  finché Alb non decide di cambiarlo.
- `sito/terza-versione`: il sito di prima, con il solver di prima (potature, controllo in avanti, solver
  alternativi, test notturni). Lì c'è anche il `main` com'era prima dell'8 novembre.
- `archivio/2026-11-04`: il lavoro precedente. Non si tocca.

## Regole per il solver

- **Non cambiare come il solver calcola gli orari** (ricerca, ordine della coda, controlli) se Alb non lo chiede
  esplicitamente. Ingresso e uscita si possono toccare: non devono cambiare il percorso della ricerca.
- Ogni modifica al solver: branch separato da `main`, PR su GitHub, merge in `main`, poi `git merge origin/main` nel
  ramo del sito e push.

### Verificare che il calcolo non cambia

```bash
gcc -O2 -std=gnu11 -w main.c data_initialization/database_initializers.c printer/printer.c \
    priority_queue/prio_queue.c ingresso_uscita/*.c -o /tmp/solver -lm
cd /tmp && rm -f orario.txt && /tmp/solver > /dev/null && grep -v '^Current time:' orario.txt | md5sum
```

Riferimento su Linux (dati di `inizializza_dati()`, 11.000 orari): `1f200e3068a438d982c5b40507cdeac2`. Su macOS può
essere diverso: a parità di priorità l'ordine della coda dipende da `qsort`. Una modifica che non deve cambiare la
ricerca deve dare lo stesso valore. `orario.txt` diventa di centinaia di MB (la griglia a ogni orario trovato e a ogni
backtrack): cancellarlo dopo l'uso.
