# Solver alternativi

Ogni cartella ha solo la sua ricerca (`main.c`) e, se serve, la sua parte di traccia; data layer, stampa, JSON, coda,
costanti, potature e traccia sono i file della radice (`../../` nel suo `CMakeLists.txt`). Tutti accettano la stessa
riga di comando di `main` (`--input`, `--traccia`, `-n`, `--txt`, `--max-backtrack`, `--max-nodi`, `--ferma-nodi`,
`--progresso`) e scrivono la stessa traccia, quindi il sito li usa senza cambiare: nel modulo del test ci sono le
caselle dei solver da non usare: gli altri girano insieme, ognuno nella sua tab. Il sito li compila dai `.c` del loro
`CMakeLists.txt`; a mano, dalla loro cartella:

```bash
cd solutori/giorni-liberi
gcc -O2 -std=gnu11 -w main.c ../../data_initialization/database_initializers.c ../../printer/printer.c \
    ../../priority_queue/prio_queue.c ../../ingresso_uscita/*.c ../../potature/potature.c -o /tmp/solver-giorni-liberi -lm
```

## I solver (oltre a main)

- **`per-classe`**: la ricerca di `main`, ma dentro ogni giorno riempie una classe alla volta (tutte le ore di C01,
  poi C02...) invece di un'ora alla volta. Le regole di una classe (ore contigue, massimo di ore, ingresso) si vedono
  subito; i conflitti tra classi alla stessa ora si vedono più tardi. L'orario resta indicizzato per cella come in
  `main`; la traccia ha in più `"ordine"` nei dati (la cella decisa a ogni passo) e scrive posizioni e orari in
  quell'ordine.
- **`in-avanti`**: la ricerca di `main` (identica: stessi controlli, stesso ordine) più un controllo in avanti, dopo
  ogni scelta:
  1. ogni cella obbligatoria che resta oggi deve avere ancora almeno un docente possibile (ha ancora ore nella
     classe, è libero a quell'ora, è già entrato, non ha già il massimo di ore oggi nella classe, il suo blocco non è
     chiuso, non ha già il massimo di classi oggi);
  2. il conteggio delle lezioni di ogni classe: per ogni ora obbligatoria h, le lezioni possibili fino a h (oggi e in
     ognuno dei giorni che restano) devono bastare. Le lezioni si contano per voce, come le fa la ricerca: un docente
     da solo, o una coppia (una lezione con due docenti, possibile solo quando tutti e due sono entrati); ogni voce
     al massimo le ore che le restano e 3 al giorno. Esempio (test 309, seme 5, 8 classi, ritardi): alla C07 restano
     Kakashi da solo e Hinata con Zoro, e Zoro entra alle 11; le ore 08–10 dei giorni che restano non hanno abbastanza
     lezioni possibili, e la scelta che porta lì si scarta subito.
  3. fino all'ultima ora di ogni giorno le lezioni bastano per tutte le ore che restano alla classe;
  4. le ore dei docenti (le potature del solver precedente): in ogni classe le ore che restano stanno in quelle che il
     docente può ancora fare lì, e in tutto nelle ore libere che gli restano dal suo ingresso.

  Si controllano dopo ogni scelta, anche dopo un'ora buca. Se uno non va, la scelta si scarta subito. Nella traccia è
  un controllo in più, alla fine delle coppie e del «da solo»: `lascia_un_docente_alle_celle_di_oggi` (con `motivo`:
  `cella`, `ore`, `ore_classe` o `ore_docente`, e per gli ultimi due i numeri). Sui dati di
  `inizializza_dati()` l'orario è lo stesso di `main`.
  Con `--txt /dev/null` (come dal sito) `orario.txt` non si scrive per niente: la ricerca è la stessa, ma non formatta
  la griglia a ogni backtrack, che era quasi tutto il tempo (circa 14 volte più veloce).
- **`due-livelli`**: livello 1, all'inizio di ogni giorno, le quote del giorno (quante ore fa oggi ogni docente in
  ogni classe, da solo e in ogni coppia), con i vincoli che si vedono senza l'orario delle ore e guardando che i giorni
  dopo restino fattibili; livello 2, l'orario del giorno con la ricerca di `main` (una classe alla volta) e i controlli
  sulle quote del giorno invece che sui totali della settimana. In più: le ore buche solo a quota finita, e il
  controllo `lascia_spazio_alle_ore_di_oggi_della_classe` (le ore di un docente in una classe sono un blocco: chi ha
  cominciato deve continuare, chi non ha cominciato deve trovare spazio). Ogni tentativo di disporre un giorno ha un
  limite di nodi (`NODI_PER_TENTATIVO`): se è stato superato almeno una volta e non si trova niente, l'esito è «non
  trovato» invece di «nessuna soluzione», perché non tutto è stato provato.

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

## Le potature (in tutti i solver)

Le due potature del solver precedente (`archivio/2026-11-04`, `ricerca/vincoli.c`) sono in `potature/`, lo stesso
file in `main`, `per-classe`, `due-livelli` e `giorni-liberi` (che le chiama con il suo ordine dei giorni,
`potature_ordine_giorni`). In `in-avanti` invece sono dentro il suo controllo in avanti (vedi sopra),
insieme al conteggio delle lezioni, che fa gia' il lavoro della potatura sulle lezioni. Sono condizioni necessarie: scartano solo rami senza soluzioni, quindi il primo orario
trovato non cambia; cambia quanto presto la ricerca torna indietro. Si controllano all'ingresso della cella dopo una
scelta:

- **a ogni cella**, per la classe appena decisa: le sue lezioni (un docente da solo o una coppia) bastano per le sue
  ore obbligatorie di ogni giorno che resta e per tutte le ore che le restano; le ore che restano a ogni suo docente
  stanno in quelle che può ancora fare lì (oggi dal suo ingresso e se il suo blocco non si è chiuso, poi 3 al giorno);
- **a ogni ora chiusa e dopo ogni ora buca**, per tutti: le ore di ogni docente in ogni classe, e in tutto nelle ore
  libere che gli restano dal suo ingresso.

Sono scritte sulle celle già decise (per ogni classe la sua prima cella non decisa), quindi valgono anche per l'ordine
di `per-classe` e di `due-livelli`. Nella traccia una potatura è un backtrack nella cella dopo la scelta, con il
controllo `ore_dei_docenti_stanno_nel_tempo` o `lezioni_della_classe_bastano` e i suoi numeri.

## Due regole che `main` all'inizio non aveva

Il verificatore delle prove (indipendente dal solver) aveva trovato orari di `main` che non rispettavano le regole: il
codocente messo prima del suo ingresso, e classi che chiudevano la settimana con meno ore di quelle da fare. Ora
`main` ha tutte e due le correzioni (PR #67: `verifica_ora_ingresso` anche per il codocente; PR #68:
`ora_buca_possibile`), e i solver qui pure. `per-classe` e `due-livelli` hanno in più, alla fine, il controllo «ogni
classe ha fatto tutte le sue ore», che con `ora_buca_possibile` non scatta mai.
