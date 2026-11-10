# Il sito: test in tempo reale

Due pagine, senza login e senza database:
- **Test e storico** (`/`): il modulo per avviare un test (seme, classi, ritardi, limite in milioni di nodi; 0 =
  senza limite) e lo storico di tutti i test fatti (esito, nodi, cella più avanti, tempo), con la × per eliminarne uno.
- **Un test** (`/test/<n>`): prima i dati del caso (le classi con le lezioni, i docenti con ore, ingresso e classi,
  le codocenze con il grafo), poi la ricerca passo per passo: uno slider su tutti i nodi, dal primo all'orario
  completo o al limite, con «Prec.» e «Succ.» (o ← e →); per ogni passo l'orario fin lì, con la cella che la ricerca
  sta per decidere (gialla), la scelta appena messa (azzurra) e quando torna indietro. Mentre il test gira lo slider
  si allunga e, se è in fondo, segue la ricerca; c'è il pulsante «Ferma». Le celle hanno tutte la stessa misura.

Un test alla volta, con un limite di nodi da 0,1 a 20 milioni (ogni passo si registra: 6 byte per nodo). Il solver lo
lancia il sito: `./untitled caso SEME CLASSI RITARDI LIMITE FILE PASSI` (vedi `caso.h` e `passi.h`), che genera i dati
dal seme come le prove (`data_initialization/generatore.c`), scrive lo stato in `FILE` ogni mezzo secondo e ogni nodo
in `PASSI`. Il sito rigenera gli stessi dati con `generatore.py` e ricostruisce un passo qualunque da `PASSI`
(`passi.py`). Ogni test è una cartella in `sito/lavoro/test/<n>` (o in `SITO_LAVORO`) con `info.json`, `stato.json` e
i passi.

## Avvio

Serve Python 3 con Flask (e, se c'è, waitress) e gcc: il solver si compila da solo al primo test e quando cambiano i
sorgenti.

```bash
python3 -m pip install -r sito/requirements.txt
python3 -m sito avvia --porta 8000                  # sul Mac: http://127.0.0.1:8000
python3 -m sito avvia --host 0.0.0.0 --porta 8000   # sul Raspberry: visibile da tutta la rete di casa
```
