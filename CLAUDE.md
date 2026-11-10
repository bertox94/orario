# Orario scolastico: solver e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice e messaggi di commit sono in italiano.

## Cosa c'è (tutto in `main`, dal 10 novembre 2026)

- **Il solver** (C): ricerca con backtracking dell'orario, un orario solo. `ricerca.c` è la ricerca (la cella, i
  vincoli, la coda, `print_orario`); `vincoli.h` dichiara i vincoli per chi li valuta uno per uno; `main.c` lancia:
  - `./untitled`: i dati di prova (`data_initialization/dati_di_prova.c`), su console solo il tempo;
  - `./untitled test N`: le prove con i dati casuali (`prove.c`), su stdout solo i test che arrivano al limite;
  - `./untitled caso|dati|passo ...`: per il sito (`caso.c`, `data_initialization/generatore.c`, `passo.c`,
    `passi.c`).
  Prima della ricerca, `verifica_dati.c` (richiesta di Alb del 10 novembre 2026): cinque controlli sui dati, condizioni
  necessarie; se uno non vale la ricerca non parte («errore nei dati», con il motivo). Non la usano le prove.
  Il solver scrive solo quello che serve al sito: niente `orario.txt`, niente stampe in più.
- **Il sito** (`sito/`, Flask, senza login né database): test in tempo reale, storico, la ricerca passo per passo con
  i controlli di ogni cella. Mostra soltanto: dati, passi e controlli li calcola il solver. Vedi `sito/LEGGIMI.md`.
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

## Regole

- **Non cambiare come il solver calcola gli orari** (ricerca, ordine della coda, controlli) se Alb non lo chiede
  esplicitamente. Uscite e comandi per il sito si possono toccare: non devono cambiare il percorso della ricerca.
- Prima di ogni commit che tocca il solver: `python3 tests/non_regressione.py` deve finire con «tutto come atteso»
  (confronta esito, nodi e l'impronta di ogni passo su 14 casi, alcuni fermati dalla verifica dei dati). Dopo una modifica voluta alla ricerca:
  `python3 tests/non_regressione.py --aggiorna`, e dirlo nel commit.
- Le modifiche si fanno in un ramo e si portano in `main` con una PR; unire solo quando Alb lo dice.

## Raspberry

`raspberry@192.168.1.5`, servizio `orario-sito`, repository `~/orario`, ambiente Python `~/orario-venv`. Da qui non si
raggiunge: i comandi li lancia Alb. Aggiornare:
`ssh -t raspberry@192.168.1.5 'cd ~/orario && git pull && sudo systemctl restart orario-sito'`, poi Ctrl+F5.

## Rami vecchi

`sito/terza-versione` (il sito di prima, con Postgres, i test notturni e il solver con potature) e
`archivio/2026-11-04`: non si toccano.
