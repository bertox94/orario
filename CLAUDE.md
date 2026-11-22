# Orario scolastico: solver e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice e messaggi di commit sono in italiano.

## Cosa c'è

Il programma riscritto da capo il 20 novembre 2026 (richiesta di Alb: «come lo faresti tu», guardando solo a
leggibilità e prestazioni, senza vincoli sul metodo): vedi `LEGGIMI.md`. È in `main` dal 21 novembre 2026 (richiesta
di Alb). Il programma di prima (solver con le potature, sito con Postgres, test di non regressione) è in
`stabile/2026-11-18` e negli altri stabili.

- Il programma in C, una cartella per cosa (dal 22 novembre 2026, richiesta di Alb: «generatore non in solver»):
  `scuola/` (i dati e la lettura di un caso), `generatore/` (i casi casuali, gli stessi di prima), `solver/`
  (`ricerca`: un'ora alla volta, la classe con meno scelte per prima, senza ripartenze; `limiti` e `classe_sola`: i
  conti che scartano i rami senza orario), `verifica/` (l'orario ricontrollato da capo), `passi/` (ogni nodo su
  file), `spiegazioni/` (quello che il sito mostra di un nodo), `programma/` (`main`: i comandi; `stato`; `fine`).
  Si compilano tutte insieme: l'elenco delle cartelle è nel `Makefile`, in `sito/solver.py` e in `prove/prova.py`.
- `sito/` (Flask, senza login né database): prove in tempo reale (`prove.py`, `passi.py`) e test notturni
  (`notturni.py`). Mostra soltanto: casi e ricerca sono del solver. Nella pagina della prova «Mostra nell'orario»
  colora le celle di cui parla un limite: le calcola il solver (`orario nodo`, `spiegazioni/spiega.c`). Un
  limite nuovo va anche lì, e nelle frasi e nella legenda di `sito/templates/prova.html` (`spiega`, `legenda`).
- `prove/prova.py`: la prova automatica (esito, nodi e verifica su 52 casi).
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

## Regole

- Qui la ricerca si può cambiare: l'obiettivo è che trovi più orari, in meno nodi, restando leggibile. Ogni modifica
  alla ricerca si misura (orari trovati e nodi su casi fissi) e si controlla che ogni orario trovato passi la
  verifica e che nessun caso risolvibile diventi «impossibile».
- Prima di ogni commit che tocca il solver: `python3 prove/prova.py` deve finire con «tutto come atteso»; dopo una
  modifica voluta alla ricerca, `--aggiorna`, e dirlo nel commit.
- Si lavora in `main`. Prima di ogni commit: spiegare ad Alb le modifiche (cosa cambia, come è stato provato) e
  aspettare il suo «ok». Gli stabili si toccano solo quando lo chiede.
- Continuare una ricerca (`orario risolvi --continua`, `riprendi_la_ricerca`) deve dare gli stessi file di una
  ricerca sola con il limite nuovo: dopo una modifica alla ricerca o ai passi, ricontrollarlo (passi, istantanee,
  profilo e stato uguali byte per byte).
- `DESCRIZIONE.md` descrive tutto (regole, generatore, ricerca, controlli, comandi, formati dei file, ogni pagina e
  ogni indirizzo del sito, i limiti) con il dettaglio per rifare il programma da zero (richiesta di Alb): ogni
  modifica che cambia una di queste cose si riporta anche lì, nello stesso commit.
- «Cambia questa scelta…» (`orario risolvi --da`, `cerca_da_una_strada`): una prova che parte da una scelta fatta
  a mano non prova le scelte che la ricerca avrebbe provato prima, quindi lì «impossibile» non è una dimostrazione.
- I commenti sono tanti e si tengono aggiornati (richiesta di Alb): in cima a ogni file che cosa fa, e riga per riga
  nelle parti difficili.
- Esperimenti sulla ricerca non entrati: le ripartenze a tentativi (tolte su richiesta di Alb, commit `8af8001`) e il
  «giro» (se la ricerca si incaglia in un giorno, tiene fermi i giorni in mezzo, libera il primo, poi i primi due...,
  e riscrive: trova qualche orario in più, ma non è nei passi né nel sito).
- Su questo repository lavora anche un'altra conversazione: prima di un push controllare che il ramo non sia cambiato.

## Raspberry

`raspberry@192.168.1.5`, servizio `orario-sito`, repository `~/orario`, ambiente Python `~/orario-venv`. Da qui non si
raggiunge: i comandi li lancia Alb. Aggiornare:
`ssh -t raspberry@192.168.1.5 'cd ~/orario && git pull && sudo systemctl restart orario-sito'`, poi Ctrl+F5.
Il sito non usa più il database: i suoi file stanno in `sito/lavoro/`.

## Rami

- `main`: il programma riscritto. Qui si lavora.
- `riscrittura`: il ramo dove è nato, fermo al punto in cui è entrato in `main`.
- `stabile/2026-11-21`: punto fermo del programma riscritto (richiesta di Alb).
- `stabile/2026-11-11`, `-12`, `-13` e `-18`: punti fermi del programma di prima; `-18` è l'ultimo.
- `sito/terza-versione` e `archivio/2026-11-04`: rami vecchi, non si toccano.
