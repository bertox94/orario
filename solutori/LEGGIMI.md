# Solver alternativi

Ogni cartella ha solo la sua ricerca (`main.c`) e, se serve, la sua parte di traccia; data layer, stampa, JSON, coda,
costanti, potature e traccia sono i file della radice (`../../` nel suo `CMakeLists.txt`). Lo stato della ricerca e i
controlli uguali a `main` (sul docente, sulla coppia, mettere e togliere un docente, la coda) sono in
`controlli/comuni.h`, incluso dal `main.c` di `main`, `per-classe`, `giorni-liberi` e `ordini-classi`: nel `main.c` di un
solver restano solo i controlli che cambiano con l'ordine delle celle o con i giri. `controlli/controlli.h` dichiara le
funzioni che la traccia, le potature e il controllo in avanti chiamano per nome. Tutti accettano la stessa
riga di comando di `main` (`--input`, `--traccia`, `-n`, `--txt`, `--max-backtrack`, `--max-nodi`, `--ferma-nodi`,
`--progresso`) e scrivono la stessa traccia, quindi il sito li usa senza cambiare: nel modulo del test ci sono le
caselle dei solver da non usare: gli altri girano insieme, ognuno nella sua tab. Il sito li compila dai `.c` del loro
`CMakeLists.txt`; a mano, dalla loro cartella:

```bash
cd solutori/giorni-liberi
gcc -O2 -std=gnu11 -w main.c ../../data_initialization/database_initializers.c ../../printer/printer.c \
    ../../priority_queue/prio_queue.c ../../ingresso_uscita/*.c ../../potature/potature.c \
    ../../controllo_in_avanti/controllo_in_avanti.c -o /tmp/solver-giorni-liberi -lm
```

## Gli stessi controlli in tutti i solver

Dal 6 novembre 2026 tutti i solver, `main` compreso, hanno gli stessi controlli (richiesta di Alb): quelli di `main`, le
potature L, C e T, il controllo in avanti A e la verifica dei dati prima della ricerca (`controllo_in_avanti/`, prima
solo nel solver `in-avanti`), e alla fine il controllo «ogni classe ha fatto tutte le sue ore». Cambia solo come cercano
(l'ordine delle celle, i giri); A si fa entrando nella cella dopo (una potatura: nella traccia
`situazione_risolvibile`). Restano propri di un solver solo i
controlli che hanno senso solo lì: i giorni fissati di `giorni-liberi`.

## I solver (oltre a main)

Il solver `in-avanti` (la ricerca di `main` con A fatto prima di scendere alla cella dopo) è stato tolto il 7
novembre 2026 (richiesta di Alb): da quando A è in tutti i solver trovava gli stessi orari di `main`, più lentamente.
I suoi test notturni restano nel database, ma i giri li saltano (come quelli di `due-livelli`).

- **`per-classe`**: la ricerca di `main`, ma dentro ogni giorno riempie una classe alla volta (tutte le ore di C01,
  poi C02...) invece di un'ora alla volta. Le regole di una classe (ore contigue, massimo di ore, ingresso) si vedono
  subito; i conflitti tra classi alla stessa ora si vedono più tardi. L'orario resta indicizzato per cella come in
  `main`; la traccia ha in più `"ordine"` nei dati (la cella decisa a ogni passo) e scrive posizioni e orari in
  quell'ordine.
- **`giorni-liberi`** (idea di Alb): la ricerca di `main` (stessi controlli, stessa coda, stesse potature) a giri.
  Quando la ricerca resta bloccata in un giorno, per esempio il sabato, si libera il primo giorno (il lunedì), si
  tengono fissati i giorni in mezzo (da martedì a venerdì, come li aveva riempiti la ricerca nel punto più avanti), si
  riempie il sabato, che ora è più facile, e per ultimo il lunedì. E così di seguito, a ogni giro.
  - Un giro riempie i giorni in un ordine; dentro il giorno come `main`. Nei giorni fissati va solo la scelta del giro
    prima (il controllo `cella_del_giorno_fissato`, alla fine delle coppie e del «da solo», e per l'ora buca).
  - Le regole che in `main` guardano «l'ultimo giorno» o «i giorni dopo» guardano l'ordine del giro: la regola del
    sabato (`sabato_esaurisci_priorita_subito`) vale per il giorno che si riempie per ultimo (dopo il primo giro il
    lunedì) e non più per il sabato; le ore obbligatorie, l'ora buca e le potature contano i giorni che si riempiono
    dopo. Le priorità della coda si ricalcolano da sole, perché ogni giro riparte da zero.
  - Un giro finisce quando trova un orario, quando ha provato tutto, o dopo `--nodi-per-giro` nodi (default
    1.000.000). Se si è bloccato nel giorno al posto X (il punto più avanti), il giro dopo fissa i giorni ai posti
    1..X-1, poi cerca il giorno X e i seguenti e per ultimo quello al posto 0. Se ha provato tutto, i giorni fissati
    non vanno bene: il giro dopo ne fissa almeno uno di meno. Un giro senza giorni fissati che prova tutto è una
    ricerca completa: nessun orario. Se si blocca già nel primo giorno, o dopo `--giri` giri (default 30), l'ultimo
    giro è la ricerca di `main`, completa e senza limite.
  - `--ferma-nodi` conta i nodi di tutti i giri. Nella traccia ogni backtrack ha in più `giro`, `fissati` e `giorni`
    (l'ordine dei giorni del giro: `pos` è il passo in quell'ordine); l'orario trovato è in ordine di calendario e la
    fine ha `giri`. Si registrano i primi `--max-backtrack` backtrack di ogni giro. I nodi (`--max-nodi`) solo del primo giro. Nella pagina del test i gruppi di backtrack finiscono
    col giro.

- **`ordini-classi`**: la ricerca di `main` a giri, con le classi di ogni ora in un ordine diverso a ogni giro (il
  primo giro: l'ordine di `main`) e un limite di nodi per giro, che segue la successione di Luby (`--nodi-base N`,
  default 10.000: N, N, 2N, N, N, 2N, 4N...). Se un giro arriva al limite si riparte da capo con un altro ordine; un
  giro che finisce prima del limite ha provato tutto (se non ha trovato, nessuna soluzione).
  - Da dove viene: l'idea di Alb di far partire la ricerca da giorni diversi (un thread per giorno). Provata: con i
    giorni ruotati la ricerca è identica (stessi nodi, stessi backtrack, stesso esito su tutti i casi provati), perché
    i giorni sono tutti uguali nei dati. L'ordine delle classi dentro l'ora invece cambia molto dove la ricerca si
    incastra: su 19 casi in cui `main` si ferma a 2 milioni di nodi, con 5 altri ordini l'orario si trova in 15, quasi
    sempre in poche centinaia di nodi. Quindi i giri cambiano l'ordine delle classi, uno dopo l'altro (non in
    parallelo: cosi' i nodi e il limite --ferma-nodi restano gli stessi su ogni computer).
  - Sui 144 casi di prova (2 milioni di nodi): trova 126 orari (`main` 110, `giorni-liberi` 118), 7 che nessun altro
    trova, e sui casi che `main` risolve usa un quinto dei nodi.
  - Nella traccia ogni backtrack ha `giro` e `classi` (l'ordine delle classi di ogni ora nel giro); il resto come
    `giorni-liberi`.

## Le potature (in tutti i solver)

Le due potature del solver precedente (`archivio/2026-11-04`, `ricerca/vincoli.c`) e il controllo in avanti sono in
`potature/`, lo stesso file in tutti i solver (`giorni-liberi` le chiama con il suo ordine dei giorni,
`potature_ordine_giorni`). Sono condizioni
necessarie: scartano solo rami senza soluzioni, quindi il primo orario trovato non cambia; cambia quanto presto la
ricerca torna indietro. Si controllano all'ingresso della cella dopo una scelta:

- **a ogni cella**, per la classe appena decisa: le sue lezioni (un docente da solo o una coppia) bastano per le sue
  ore obbligatorie di ogni giorno che resta e per tutte le ore che le restano; le ore che restano a ogni suo docente
  stanno in quelle che può ancora fare lì (oggi dal suo ingresso e se il suo blocco non si è chiuso, poi 3 al giorno);
- **a ogni ora chiusa e dopo ogni ora buca**, per tutti: le ore di ogni docente in ogni classe, e in tutto nelle ore
  libere che gli restano dal suo ingresso;
- **a ogni cella, alla fine**: A, il controllo in avanti:
  1. ogni cella obbligatoria che resta oggi deve avere ancora almeno un docente possibile (ha ancora ore nella
     classe, è libero a quell'ora, è già entrato, non ha già il massimo di ore oggi nella classe, il suo blocco non è
     chiuso, non ha già il massimo di classi oggi);
  2. il conteggio delle lezioni di ogni classe: per ogni ora obbligatoria h, le lezioni possibili fino a h (oggi e in
     ognuno dei giorni che restano) devono bastare. Le lezioni si contano per voce, come le fa la ricerca: un docente
     da solo, o una coppia (una lezione con due docenti, possibile solo quando tutti e due sono entrati); ogni voce
     al massimo le ore che le restano e 3 al giorno. Esempio (test 309, seme 5, 8 classi, ritardi): alla C07 restano
     Kakashi da solo e Hinata con Zoro, e Zoro entra alle 11; le ore 08–10 dei giorni che restano non hanno abbastanza
     lezioni possibili, e la scelta che porta lì si scarta subito;
  3. fino all'ultima ora di ogni giorno le lezioni bastano per tutte le ore che restano alla classe;
  4. le ore dei docenti: in ogni classe le ore che restano stanno in quelle che il docente può ancora fare lì, e in
     tutto nelle ore libere che gli restano dal suo ingresso.

Sono scritte sulle celle già decise (per ogni classe la sua prima cella non decisa), quindi valgono anche per l'ordine
di `per-classe`. Nella traccia una potatura è un backtrack nella cella dopo la scelta, con il
controllo `ore_dei_docenti_stanno_nel_tempo`, `lezioni_della_classe_bastano` o `situazione_risolvibile` (A) e i suoi
numeri.

## Due regole che `main` all'inizio non aveva

Il verificatore delle prove (indipendente dal solver) aveva trovato orari di `main` che non rispettavano le regole: il
codocente messo prima del suo ingresso, e classi che chiudevano la settimana con meno ore di quelle da fare. Ora
`main` ha tutte e due le correzioni (PR #67: `verifica_ora_ingresso` anche per il codocente; PR #68:
`ora_buca_possibile`), e i solver qui pure. Tutti hanno in più, alla fine, il controllo «ogni classe ha fatto tutte le
sue ore», che con `ora_buca_possibile` non scatta mai.
