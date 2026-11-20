# Orario scolastico: solver e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice e messaggi di commit sono in italiano.

## Cosa c'è in questo ramo

Il ramo `riscrittura` è il programma riscritto da capo il 20 novembre 2026 (richiesta di Alb: «come lo faresti tu»,
guardando solo a leggibilità e prestazioni, senza vincoli sul metodo): vedi `LEGGIMI.md`. Il programma di prima è in
`main` e negli stabili, e lì non si tocca da qui.

- `solver/` (C): `scuola` (i dati e la lettura di un caso), `genera` (i casi casuali, gli stessi di prima), `ricerca`
  (un'ora alla volta, la classe con meno scelte per prima, a tentativi), `limiti` e `classe_sola` (i conti che
  scartano i rami senza orario), `verifica` (l'orario ricontrollato da capo), `passi` (ogni nodo su file), `main`.
- `sito/` (Flask, senza login né database): prove in tempo reale (`prove.py`, `passi.py`) e test notturni
  (`notturni.py`). Mostra soltanto: casi e ricerca sono del solver.
- `prove/prova.py`: la prova automatica (esito, nodi e verifica su 52 casi).
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

## Regole

- Qui la ricerca si può cambiare: l'obiettivo è che trovi più orari, in meno nodi, restando leggibile. Ogni modifica
  alla ricerca si misura (orari trovati e nodi su casi fissi) e si controlla che ogni orario trovato passi la
  verifica e che nessun caso risolvibile diventi «impossibile».
- Prima di ogni commit che tocca il solver: `python3 prove/prova.py` deve finire con «tutto come atteso»; dopo una
  modifica voluta alla ricerca, `--aggiorna`, e dirlo nel commit.
- Prima di portare qualcosa in `main` o in uno stabile: spiegare ad Alb le modifiche e aspettare il suo «ok».
- Su questo repository lavora anche un'altra conversazione: prima di un push controllare che il ramo non sia cambiato.

## Raspberry

`raspberry@192.168.1.5`, servizio `orario-sito`, repository `~/orario`, ambiente Python `~/orario-venv`. Da qui non si
raggiunge: i comandi li lancia Alb. Aggiornare:
`ssh -t raspberry@192.168.1.5 'cd ~/orario && git pull && sudo systemctl restart orario-sito'`, poi Ctrl+F5.

## Rami

- `riscrittura`: questo ramo, il programma riscritto.
- `main`: il programma di prima, con tutto il lavoro fino al 20 novembre 2026.
- `stabile/2026-11-11`, `-12`, `-13` e `-18`: punti fermi del programma di prima.
- `sito/terza-versione` e `archivio/2026-11-04`: rami vecchi, non si toccano.
