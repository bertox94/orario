# Il sito: test in tempo reale

Due pagine, senza login e senza database:
- **Test e storico** (`/`): il modulo per avviare un test (seme, classi, ritardi, ordine delle classi, limite in milioni di nodi, da 0,1
  a 20) e lo storico di tutti i test fatti (esito, nodi, cella più avanti, tempo), con la × per eliminarne uno.
- **Un test** (`/test/<n>`): prima i dati del caso (un riepilogo; le classi a schede, con un quadretto per ogni ora
  di lezione, da solo o in coppia; i docenti con ore, ingresso, classi e codocenti; le codocenze con il grafo), poi la ricerca passo per passo: uno slider su tutti i nodi, dal primo all'orario
  completo o al limite, con «Prec.» e «Succ.» (o ← e →); per ogni passo l'orario fin lì, con la cella che la ricerca
  sta per decidere (gialla), la scelta appena messa (azzurra) e quando torna indietro. Mentre il test gira lo slider
  si allunga e, se è in fondo, segue la ricerca; c'è il pulsante «Ferma». Le celle hanno tutte la stessa misura.
  Sopra l'orario per classi, lo stesso passo visto dai docenti: una riga per docente, con la classe dove insegna a ogni ora, con chi è
  in coppia e, tratteggiate, le ore prima del suo ingresso.
  Sotto l'orario, per la cella del passo: il controllo della classe (può avere lezione, può avere un'ora buca) con i
  suoi numeri, la coda di priorità in ordine, e per ogni docente della classe il valore di tutti i controlli, da solo
  e in coppia (calcolati dal solver con le stesse funzioni della ricerca).
  Quando l'orario è trovato, in cima c'è «L'orario trovato»: l'esito della verifica indipendente del solver
  (`verifica_orario.h`), controllo per controllo, e un pulsante che porta lo slider all'orario completo.
  Se i dati non passano la verifica del solver (`verifica_dati.h`) l'esito è «errore nei dati»: la ricerca non parte e
  la pagina mostra il motivo, sopra i dati del caso.

Un test alla volta, con un limite di nodi da 0,1 a 20 milioni (ogni passo si registra: 6 byte per nodo).

Il sito mostra soltanto: tutto il calcolo lo fa il solver, che il sito lancia in tre modi (vedi `main.c`):
- `./untitled caso SEME CLASSI RITARDI ORDINE LIMITE FILE PASSI` (`caso.h`): la ricerca; scrive lo stato in `FILE` ogni mezzo
  secondo e ogni nodo in `PASSI` (`passi.h`);
- `./untitled dati SEME CLASSI RITARDI ORDINE`: i dati del caso (lezioni e ore di ingresso);
- `./untitled passo SEME CLASSI RITARDI ORDINE PASSI K` (`passo.h`): il nodo K, con l'orario fin lì, il controllo della
  classe, la coda e i controlli dei docenti.

Ogni test è una cartella in `sito/lavoro/test/<n>` (o in `SITO_LAVORO`) con `info.json`, `stato.json` e i passi.

## Avvio

Serve Python 3 con Flask (e, se c'è, waitress) e gcc: il solver si compila da solo al primo test e quando cambiano i
sorgenti.

```bash
python3 -m pip install -r sito/requirements.txt
python3 -m sito avvia --porta 8000                  # sul Mac: http://127.0.0.1:8000
python3 -m sito avvia --host 0.0.0.0 --porta 8000   # sul Raspberry: visibile da tutta la rete di casa
```
