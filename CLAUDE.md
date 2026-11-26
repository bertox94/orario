# Orario scolastico: programma e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice e messaggi di commit sono in italiano.

## Cosa c'è

Dal 26 novembre 2026 `main` è tutto in Python (richiesta di Alb: «lasciare la ricerca con il solver nuovo e il resto
tutto in Python», e la ricerca in C «toglila da main»). Vedi `LEGGIMI.md` e, per il dettaglio, `DESCRIZIONE.md`.

- `orario/`: il programma. `scuola.py` (i dati, le regole, la lettura di un caso), `genera.py` e `nomi.py` (i casi
  casuali: gli stessi, carattere per carattere, del generatore in C di prima), `esatto.py` (il metodo: il caso come
  programma lineare intero, risolto con scipy/HiGHS; dà l'orario o dimostra che non esiste), `verifica.py` (l'orario
  ricontrollato da capo, senza usare il metodo), `__main__.py` (i comandi `genera`, `risolvi`, `verifica`).
- `sito/` (Flask, senza login né database): le prove (`prove.py`) e i test notturni (`notturni.py`). Mostra
  soltanto: genera i casi con `orario.genera` e lancia `python3 -m orario risolvi` in un processo a parte
  (`programma.py`). Una cosa alla volta: una prova o un giro.
- `prove/prova.py`: la prova automatica (caso generato, esito e verifica su 54 casi).
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

La ricerca in C (un'ora alla volta, i controlli A–G, i passi su file, i ritorni, «Continua», «Cambia questa
scelta…») non è più in `main`: l'ultima versione è in `stabile/2026-11-26`, con il suo `DESCRIZIONE.md`.

## Regole

- Si lavora in `main`. Prima di ogni commit: spiegare ad Alb le modifiche (cosa cambia, come è stato provato) e
  aspettare il suo «ok». Gli stabili si toccano solo quando lo chiede.
- Prima di ogni commit che tocca `orario/`: `python3 prove/prova.py` deve finire con «tutto come atteso»; dopo una
  modifica voluta, `--aggiorna`, e dirlo nel commit.
- `genera.py` non si cambia senza chiederlo: cambierebbe i casi di tutti i semi, e i risultati dei test notturni
  non sarebbero più confrontabili. Le impronte dei casi sono in `prove/attesi.json`.
- Dopo una modifica a `esatto.py`: ogni orario trovato deve passare la verifica, e nessun caso deve passare da
  «trovato» a «impossibile» o il contrario (sono risposte certe). Va rifatto su tanti casi, non solo sui 54.
- `verifica.py` non deve usare niente di `esatto.py`: è il controllo indipendente.
- Una regola nuova dell'orario va in tre posti: l'elenco in `scuola.py`, i conti in `esatto.py`, il controllo in
  `verifica.py` (e in `DESCRIZIONE.md`). Dal 26 novembre 2026 (richiesta di Alb) c'è la regola 9: un docente ha al
  massimo 2 ore buche al giorno, tra la sua prima e la sua ultima lezione. Con lei dimostrare un caso impossibile
  diventava lentissimo: per questo `esatto.risolvi` chiede prima senza la regola 9 e poi, se serve, con tutte.
- Materie e aule (26 novembre 2026, richiesta di Alb): ogni lezione ha una materia, un docente può averne più di
  una, e ogni lezione occupa un'aula del tipo della sua materia (regola 10; tutte le aule sono contate). Il
  generatore le aggiunge con numeri casuali a parte: classi, docenti, lezioni e ritardi di ogni seme non sono
  cambiati, e non devono cambiare.
- `DESCRIZIONE.md` descrive tutto (regole, generatore, metodo, verifica, comandi, ogni pagina e ogni indirizzo del
  sito, i limiti) con il dettaglio per rifare il programma da zero (richiesta di Alb): ogni modifica che cambia una
  di queste cose si riporta anche lì, nello stesso commit.
- I commenti sono tanti e si tengono aggiornati (richiesta di Alb): in cima a ogni file che cosa fa, e riga per riga
  nelle parti difficili.
- Su questo repository lavora anche un'altra conversazione: prima di un push controllare che il ramo non sia cambiato.

## Raspberry

`raspberry@192.168.1.5`, servizio `orario-sito`, repository `~/orario`, ambiente Python `~/orario-venv`. Da qui non si
raggiunge: i comandi li lancia Alb. Aggiornare (sul Pi):
`cd ~/orario && git pull && ~/orario-venv/bin/pip install -r sito/requirements.txt && sudo systemctl restart orario-sito`,
poi Ctrl+F5. Serve scipy (senza, le prove finiscono «interrotta»); non serve più gcc.
Il sito non usa un database: i suoi file stanno in `sito/lavoro/`. Le prove e i giri fatti con il programma
in C restano leggibili (esito e orario, senza i passi); i casi dei test notturni restano.

## Rami

- `main`: il programma in Python. Qui si lavora.
- `stabile/2026-11-26`: l'ultima versione con la ricerca in C (e il metodo esatto accanto).
- `stabile/2026-11-21`: la ricerca in C al 21 novembre. `riscrittura`: il ramo dove era nata.
- `stabile/2026-11-11`, `-12`, `-13` e `-18`: punti fermi del programma ancora prima; `-18` è l'ultimo.
- `sito/terza-versione` e `archivio/2026-11-04`: rami vecchi, non si toccano.
