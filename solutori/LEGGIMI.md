# Solver alternativi

Ogni cartella è un solver completo e indipendente: una copia dei sorgenti di `main` (ricerca, controlli, ingresso e
uscita) con le sue modifiche e il suo `CMakeLists.txt`. Nessuna cartella usa file delle altre o della radice. Tutti
accettano la stessa riga di comando di `main` (`--input`, `--traccia`, `-n`, `--txt`, `--max-backtrack`,
`--max-nodi`, `--progresso`) e scrivono la stessa traccia, quindi il sito li usa senza cambiare: nel modulo del test
ci sono le caselle dei solver da non usare: gli altri girano insieme, ognuno nella sua tab.

Si compilano come `main`, dalla loro cartella:

```bash
cd solutori/in-avanti
gcc -O2 -std=gnu11 -w main.c data_initialization/database_initializers.c printer/printer.c \
    priority_queue/prio_queue.c ingresso_uscita/*.c -o /tmp/solver-in-avanti -lm
```

## I quattro solver (oltre a main)

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

  Se una delle due non va, la scelta si scarta subito. Nella traccia è un controllo in più, alla fine delle coppie e
  del «da solo»: `lascia_un_docente_alle_celle_di_oggi` (con `motivo`: `cella` o `ore`). Sui dati di
  `inizializza_dati()` l'orario è lo stesso di `main`.
  Con `--txt /dev/null` (come dal sito) `orario.txt` non si scrive per niente: la ricerca è la stessa, ma non formatta
  la griglia a ogni backtrack, che era quasi tutto il tempo (circa 14 volte più veloce).
- **`in-avanti-dopo`**: gli stessi controlli in avanti di `in-avanti`, ma fatti all'ingresso della cella dopo la
  scelta invece che prima di scendere: il ciclo dei candidati è quello di `main`, riga per riga, e all'inizio di
  `print_orario` c'è un controllo in più (se la scelta nella cella prima rende il resto impossibile, si torna indietro).
  Trova gli stessi orari di `in-avanti`; ogni scelta scartata è un nodo in più, e nella traccia un backtrack nella
  cella dopo, con il controllo `situazione_risolvibile` (la cella che non si può riempire e il motivo).
- **`due-livelli`**: livello 1, all'inizio di ogni giorno, le quote del giorno (quante ore fa oggi ogni docente in
  ogni classe, da solo e in ogni coppia), con i vincoli che si vedono senza l'orario delle ore e guardando che i giorni
  dopo restino fattibili; livello 2, l'orario del giorno con la ricerca di `main` (una classe alla volta) e i controlli
  sulle quote del giorno invece che sui totali della settimana. In più: le ore buche solo a quota finita, e il
  controllo `lascia_spazio_alle_ore_di_oggi_della_classe` (le ore di un docente in una classe sono un blocco: chi ha
  cominciato deve continuare, chi non ha cominciato deve trovare spazio). Ogni tentativo di disporre un giorno ha un
  limite di nodi (`NODI_PER_TENTATIVO`): se è stato superato almeno una volta e non si trova niente, l'esito è «non
  trovato» invece di «nessuna soluzione», perché non tutto è stato provato.

## Le potature (in tutti i solver)

Le due potature del solver precedente (`archivio/2026-11-04`, `ricerca/vincoli.c`) sono in `potature/`, lo stesso
file in `main` e in ogni cartella. Sono condizioni necessarie: scartano solo rami senza soluzioni, quindi il primo orario
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
