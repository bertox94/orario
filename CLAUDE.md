# Orario scolastico: solver e sito

Repository di Alb (bertox94/orario). Si parla in italiano e ci si rivolge a lei al femminile. Anche commenti, nomi nel
codice del sito e messaggi di commit sono in italiano.

## Cosa c'è

- **Il solver** (C, radice del repository): ricerca con backtracking dell'orario. `main.c` contiene la ricerca
  (`print_orario`) e i controlli che dipendono dall'ordine delle celle; lo stato e gli altri controlli sono in
  `controlli/comuni.h` (definizioni, condivise con i solver di `solutori/`), le dichiarazioni per la traccia e le
  potature in `controlli/controlli.h`; i dati di prova sono in `data_initialization/database_initializers.c`
  (`inizializza_dati()`). `ingresso_uscita/` è solo ingresso e uscita per il sito: `--input` (dati da JSON),
  `--traccia` (un JSON per riga: dati, backtrack con i controlli, nodi, orari trovati, fine), `-n`, `--txt`, `--max-nodi`,
  `--max-backtrack`, `--progresso`, `--ferma-nodi` (ferma la ricerca esattamente dopo N nodi, sempre allo stesso punto: test in tempo reale e notturni). Senza opzioni il solver fa esattamente quello che faceva prima.
- **Il sito** (Flask + Postgres, cartella `sito/`, in `main`; si lavora in `sito/terza-versione`): dati della scuola e test in tempo
  reale. A ogni backtrack la pagina del test mostra l'orario fino a quel punto e, per ogni docente, i controlli della
  ricerca con ingressi ed esito. Vedi `sito/INSTALLAZIONE.md`.
- **I solver alternativi** (cartella `solutori/`, nel branch del sito): `per-classe`,
  `giorni-liberi`, `ordini-classi`.
  Ognuno ha solo la sua ricerca (`main.c`) e, se serve, la sua parte di traccia; data layer, stampa, JSON, coda,
  costanti e potature sono i file della radice (`../../` nel suo `CMakeLists.txt`). Stessa riga di comando e stessa
  traccia di `main`. Il sito li compila dai `.c` del loro `CMakeLists.txt` e li offre come caselle nel modulo del
  test (girano insieme, una tab ciascuno). Vedi
  `solutori/LEGGIMI.md`. Le regole su «non cambiare come il solver calcola» valgono per `main`; i solver di
  `solutori/` sono esperimenti chiesti da Alb.
- `bf_search/`: SQL di Alb, da non toccare senza chiedere.

## Branch

- `main`: il solver, i solver alternativi di `solutori/`, il sito e i test di non regressione (la terza versione, unita
  in `main` il 6 novembre 2026 su richiesta di Alb).
- `sito/terza-versione`: il ramo di lavoro del sito. Le modifiche al sito si fanno direttamente qui, con push diretto;
  in `main` si portano con una PR quando Alb lo chiede. Il Raspberry segue `sito/terza-versione` (dal 7 novembre 2026,
  richiesta di Alb): una modifica arriva sul Pi con il push e un aggiornamento del Pi.
- `sito/seconda-versione`: la versione precedente del sito.
- `archivio/2026-11-04`: tutto il lavoro precedente (solver avanzato, vecchio sito, esperimenti). Non si tocca.
- `prototipo/scheletro-accoppiamenti`: prototipo in Python (accoppiamenti, Monte Carlo, SAT), non unito.

## Regole per il solver

- **Non cambiare come il solver calcola gli orari** (ricerca, ordine della coda, controlli) se Alb non lo chiede
  esplicitamente. Ingresso, uscita e traccia si possono toccare: non devono cambiare il percorso della ricerca.
- I controlli sono gli stessi in tutti i solver, `main` compreso: quelli di `main`, le potature (`potature/`: ore dei
  docenti nel tempo rimasto, lezioni delle classi, e il controllo in avanti), la verifica dei dati prima della ricerca
  (`controllo_in_avanti/`) e il controllo finale delle ore. Nel commento di `potature.h` quando avvengono e perché
  sono necessarie. Altri controlli (dall'archivio) si aggiungono **uno alla volta**, quando lei lo chiede, e a tutti
  i solver. Un controllo nuovo va anche nella traccia (`ingresso_uscita/traccia.c`), nello stesso ordine in cui la
  ricerca lo valuta, e nelle descrizioni della pagina (`sito/static/orario.js`, `descrizioneControllo`,
  `descriviIngressi`, `motivoBreve`).
- Ogni modifica al solver: branch separato da `main`, PR su GitHub, merge in `main`, poi `git merge origin/main` nel
  branch del sito e push. Dopo il merge il branch della PR si cancella.

### Verificare che il calcolo non cambia

```bash
gcc -O2 -std=gnu11 -w main.c data_initialization/database_initializers.c printer/printer.c \
    priority_queue/prio_queue.c ingresso_uscita/*.c potature/potature.c controllo_in_avanti/controllo_in_avanti.c \
    -o /tmp/solver -lm
cd /tmp && rm -f orario.txt && /tmp/solver > /dev/null && grep -v '^Current time:' orario.txt | md5sum
```

Riferimento (dati di `inizializza_dati()`, 11.000 orari): `8e39aec451cd1a75ddd35c266ecad126`, uguale su Mac e su
Linux (il Raspberry): a parità la coda è ordinata per indice del docente, quindi l'ordine non dipende da `qsort`. Dal
7 novembre 2026 (richiesta di Alb) la coda mette prima il docente con meno margine (ore che può ancora fare da adesso,
dal suo ingresso, meno ore che gli restano: `sort_queue_mul` in `controlli/comuni.h`), in tutti i solver; senza
ritardi l'ordine è quello di prima, ma i dati di prova hanno docenti che entrano tardi: il riferimento con la coda di
prima era `b7e8ad1800ea16bc0fc8be628c926fe6`. Il controllo in avanti e la verifica dei dati (6 novembre 2026) non
lo cambiano: su questi dati non scartano niente. Prima dell'ordine per indice a parità lo stesso solver dava
`1f200e…` su Linux e `777dc…` su macOS. Una modifica che
non deve cambiare la ricerca deve dare lo stesso valore. Se Alb chiede di cambiare la ricerca (per esempio un
controllo nuovo), il valore cambia: si annota il nuovo riferimento qui.

Attenzione: `main` stampa l'intera griglia in `orario.txt` a ogni backtrack, quindi sui casi difficili il file diventa
di gigabyte in pochi secondi. Per le prove usare `--txt /dev/null`, oppure `ulimit -f` per confrontare due
esecuzioni fino allo stesso byte. Cancellare `orario.txt` dopo l'uso.

## Il sito

- Avvio sul Mac: `DATABASE_URL=postgresql://localhost/orario python3 -m sito avvia --porta 8000`. Il database è
  Postgres 18 (Postgres.app) sul Mac; il solver si compila da solo al primo test.
- Migrazioni in `sito/migrazioni/`, applicate da sole all'avvio. Prima di una migrazione che cancella dati, copia del
  database: `pg_dump orario > ~/orario-copia.sql`.
- Le lezioni del sito sono le chiamate di `main`: il docente fa `ore` ore, di cui `ore_codocente` con il codocente
  (`set_classe_insegnante_ore_cod`). Verso il solver vanno nell'ordine di inserimento (`modello.per_main`): l'ordine
  delle chiamate decide gli indici dei docenti e quindi la coda.
- I dati casuali vengono da `sito/generatore.py` (xorshift64*: stesso seme, stessi dati). I test notturni rigenerano i
  casi dai semi: cambiare il generatore cambia i dati di ogni seme e quindi i riferimenti dei test.

## Raspberry

- `raspberry@192.168.1.5`, servizio `orario-sito`, repository `~/orario` (ramo `sito/terza-versione`), ambiente Python `~/orario-venv`. Il sito usa
  il database del Mac.
- Aggiornare: `ssh -t raspberry@192.168.1.5 'cd ~/orario && git pull && sudo systemctl restart orario-sito'`, poi nel
  browser Ctrl+F5. Log: `journalctl -u orario-sito -n 50 --no-pager`.
- Mai `pkill -f` nello stesso comando che riavvia il sito (la riga di comando contiene il testo cercato e il comando
  uccide se stesso). Per cercare il processo usare uno schema come `pgrep -af "[p]ython3 -m sito"`.
