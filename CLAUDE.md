# Orario scolastico: solver e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice e messaggi di commit sono in italiano.

## Cosa c'è (tutto in `main`, dal 10 novembre 2026)

- **Il solver** (C): ricerca con backtracking dell'orario, un orario solo. `ricerca.c` è la ricerca (la cella, i
  vincoli, la coda, `cerca_orario`); `vincoli.h` dichiara i vincoli per chi li valuta uno per uno; `main.c` lancia:
  - `./untitled test N`: le prove con i dati casuali (`prove.c`), su stdout solo i test che arrivano al limite;
  - `./untitled caso|dati|passo|ultima ...`: per il sito (`caso.c`, `data_initialization/generatore.c`, `passo.c`,
    `ultima.c`, `passi.c`).
  Niente variabili globali né `static` (richiesta di Alb dell'11 novembre 2026): i dati stanno in una `Scuola`
  (`data_initialization/database_initializers.h`), lo stato della ricerca in una `Ricerca` (`ricerca.h`), e ogni
  funzione le riceve come argomento. Restano `static` solo le tabelle di nomi costanti negli header.
  Prima della ricerca, `verifica_dati.c` (richiesta di Alb del 10 novembre 2026): sei controlli sui dati, condizioni
  necessarie; se uno non vale la ricerca non parte («errore nei dati», con il motivo); le prove saltano i casi che
  non la passano.
  Dopo la ricerca, `verifica_orario.c` (richiesta di Alb dell'11 novembre 2026): ricontrolla l'orario trovato senza
  usare niente della ricerca (otto controlli); l'esito va nello stato del test (`verifica`).
  Il solver scrive solo quello che serve al sito: niente `orario.txt`, niente stampe in più.
- **Il sito** (`sito/`, Flask, senza login né database): test in tempo reale, storico, la ricerca passo per passo con
  i controlli di ogni cella. Mostra soltanto: dati, passi e controlli li calcola il solver. Vedi `sito/LEGGIMI.md`.
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

## Regole

- **Non cambiare come il solver calcola gli orari** (ricerca, ordine della coda, controlli) se Alb non lo chiede
  esplicitamente. Uscite e comandi per il sito si possono toccare: non devono cambiare il percorso della ricerca.
- Prima di ogni commit che tocca il solver: `python3 tests/non_regressione.py` deve finire con «tutto come atteso»
  (confronta esito, nodi e l'impronta di ogni passo su 14 casi, alcuni fermati dalla verifica dei dati). Dopo una
  modifica voluta alla ricerca:
  `python3 tests/non_regressione.py --aggiorna`, e dirlo nel commit.
- Come si lavora (richieste di Alb del 10 e dell'11 novembre 2026):
  - si lavora direttamente in `main`: niente rami e niente PR;
  - prima di ogni commit si spiegano ad Alb le modifiche (il codice, cosa cambia, come è stato provato) e si
    aspetta il suo «ok» esplicito, dato dopo averle viste. Senza «ok» niente commit: le modifiche restano in locale;
  - per un lavoro nuovo, prima una proposta a parole; poi le modifiche in locale, la spiegazione e l'«ok»;
  - un commit per ogni modifica (per esempio un commit per ogni controllo aggiunto), e ogni funzione deve dire il
    vero già nel commit in cui nasce: due passi che da soli lascerebbero qualcosa di sbagliato vanno insieme.

## Raspberry

`raspberry@192.168.1.5`, servizio `orario-sito`, repository `~/orario`, ambiente Python `~/orario-venv`. Da qui non si
raggiunge: i comandi li lancia Alb. Aggiornare:
`ssh -t raspberry@192.168.1.5 'cd ~/orario && git pull && sudo systemctl restart orario-sito'`, poi Ctrl+F5.

## Rami vecchi

`sito/terza-versione` (il sito di prima, con Postgres, i test notturni e il solver con potature) e
`archivio/2026-11-04`: non si toccano.
