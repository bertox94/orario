# Il sito: test in tempo reale

Due pagine, senza login e senza database:
- **Test e storico** (`/`): il modulo per avviare un test (seme, classi, ritardi, limite in milioni di nodi; 0 =
  senza limite) e lo storico di tutti i test fatti (esito, nodi, cella più avanti, tempo), con la × per eliminarne uno.
- **Un test** (`/test/<n>`): la ricerca in tempo reale. Mentre gira, ogni secondo, l'orario fino alla cella che sta
  decidendo (in giallo), uno slider che mostra dove è arrivata, i nodi e il tempo, e il pulsante «Ferma». Alla fine
  l'orario trovato, o quello dove si è fermata.

Un test alla volta. Il solver lo lancia il sito: `./untitled caso SEME CLASSI RITARDI LIMITE FILE` (vedi `caso.h`), che
genera i dati dal seme come le prove (`data_initialization/generatore.c`) e scrive lo stato in `FILE` ogni mezzo
secondo. Ogni test è una cartella in `sito/lavoro/test/<n>` (o in `SITO_LAVORO`), con `info.json` e `stato.json`.

## Avvio

Serve Python 3 con Flask (e, se c'è, waitress) e gcc: il solver si compila da solo al primo test e quando cambiano i
sorgenti.

```bash
python3 -m pip install -r sito/requirements.txt
python3 -m sito avvia --porta 8000                  # sul Mac: http://127.0.0.1:8000
python3 -m sito avvia --host 0.0.0.0 --porta 8000   # sul Raspberry: visibile da tutta la rete di casa
```
