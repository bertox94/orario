# Orario scolastico: solver e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice e messaggi di commit sono in italiano.

## Cosa c'è (tutto in `main`, dal 10 novembre 2026)

- **Le cartelle del solver** (richiesta di Alb del 12 novembre 2026; nella radice resta solo `main.c`):
  - `ricerca/`: la ricerca e i vincoli (`ricerca.c`, `ricerca.h`, `vincoli.h`); `ricerca/coda/`: la coda di priorità;
  - `dati/`: la scuola e i casi casuali (`database_initializers`, `generatore`, `nomi_docenti.h`);
  - `verifiche/`: la verifica dei dati prima della ricerca e quella dell'orario dopo;
  - `comandi/`: i comandi per il sito e per le prove (`caso`, `passo`, `prove`), il file dei passi (`passi`) e
    l'uscita in JSON (`json.h`).
  Chi compila elenca queste cartelle: `CMakeLists.txt`, `sito/solver.py`, `tests/non_regressione.py`.
- **Le potature** (richiesta di Alb del 12 novembre 2026, in `main` dal 13; `ricerca/potature.c`): i sei controlli
  della verifica dei dati rifatti a ogni nodo su quello che resta, più una settima (idea di Alb del 13 novembre):
  una classe guardata da sola, come se i suoi docenti fossero sempre liberi, deve avere ancora un orario suo per il
  resto della settimana (una ricerca piccola dentro la ricerca, con le risposte ricordate per stato della classe).
  Sono condizioni necessarie: scartano solo rami senza orario, quindi l'orario trovato è lo stesso e cambiano solo i
  nodi. A ogni nodo rifanno solo i conti che la scelta appena messa ha cambiato. Accese la 1, la 2, la 5 e la 7
  (`POTATURE_ACCESE` in `potature.h`); la 3, la 4 e la 6 sono nel codice ma spente, perché non ripagano il loro
  costo (la 4 la copre la 7). Misura del 13 novembre su 341 casi a 2 milioni di nodi: 175 orari trovati senza
  potature (180 con 14 milioni di nodi), 297 con 1, 2, 4 e 5, 306 con 1, 2, 5 e 7 come con tutte e sette, nessuno
  perso. La parità nella coda («a parità, chi ha più ore nella classe») è stata provata e lasciata fuori: cambia la
  strada e perde dei casi.
- **Il solver** (C): ricerca con backtracking dell'orario, un orario solo. `ricerca.c` è la ricerca (la cella, i
  vincoli, la coda, `cerca_orario`); `vincoli.h` dichiara i vincoli per chi li valuta uno per uno; `main.c` lancia:
  - `./untitled test N [ORDINI LIMITE]`: le prove con i dati casuali (`prove.c`), su stdout solo i test che arrivano
    al limite;
  - `./untitled caso|dati|passo ...`: per il sito (`caso.c`, `dati/generatore.c`, `passo.c`, `passi.c`).
  I casi casuali hanno anche un ordine delle classi (`ORDINE`, 0 quello generato: stessi dati, classi in un altro
  ordine, quindi un'altra strada della ricerca), uguale a quello del sito vecchio.
  Niente variabili globali né `static` (richiesta di Alb dell'11 novembre 2026): i dati stanno in una `Scuola`
  (`dati/database_initializers.h`), lo stato della ricerca in una `Ricerca` (`ricerca.h`), e ogni
  funzione le riceve come argomento. Restano `static` solo le tabelle di nomi costanti negli header.
  Prima della ricerca, `verifica_dati.c` (richiesta di Alb del 10 novembre 2026): sei controlli sui dati, condizioni
  necessarie; se uno non vale la ricerca non parte («errore nei dati», con il motivo); le prove saltano i casi che
  non la passano.
  Dopo la ricerca, `verifica_orario.c` (richiesta di Alb dell'11 novembre 2026): ricontrolla l'orario trovato senza
  usare niente della ricerca (otto controlli); l'esito va nello stato del test (`verifica`).
  Il solver scrive solo quello che serve al sito: niente `orario.txt`, niente stampe in più.
- **Il sito** (`sito/`, Flask, senza login né database): test in tempo reale, storico, la ricerca passo per passo con
  i controlli di ogni cella. Mostra soltanto: dati, passi e controlli li calcola il solver. Vedi `sito/LEGGIMI.md`.
  Anche i test notturni (`sito/notturni.py`): un elenco di casi rieseguiti in un giro, con un limite di nodi unico e
  il confronto con il giro prima; il giro della notte parte da solo e ogni sera aggiunge i casi di base (semi e
  classi scelti nella pagina) con un ordine delle classi nuovo.
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

## Regole

- **Non cambiare come il solver calcola gli orari** (ricerca, ordine della coda, controlli) se Alb non lo chiede
  esplicitamente. Uscite e comandi per il sito si possono toccare: non devono cambiare il percorso della ricerca.
- Prima di ogni commit che tocca il solver: `python3 tests/non_regressione.py` deve finire con «tutto come atteso»
  (confronta esito, nodi e l'impronta di ogni passo su 17 casi, alcuni fermati dalla verifica dei dati). Dopo una
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

## Rami

- `main`: tutto il lavoro. Il 13 novembre 2026 (richiesta di Alb) è stato portato al ramo `potature`, con un commit di
  unione che ne tiene tutti i file: i due coincidono, e `potature` non serve più.
- `cella-piu-avanti`: il `main` di prima (923ec81), con la sezione «La cella più avanti» e il comando `ultima`, che
  nel `main` di adesso non ci sono. Da lì si riprendono se servono.
- `stabile/2026-11-11` e `stabile/2026-11-12`: punti fermi da cui ripartire; non si aggiornano.

## Rami vecchi

`sito/terza-versione` (il sito di prima, con Postgres, i test notturni e il solver con potature) e
`archivio/2026-11-04`: non si toccano.
