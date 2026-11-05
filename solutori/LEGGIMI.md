# Solver alternativi

Ogni cartella è un solver completo e indipendente: una copia dei sorgenti di `main` (ricerca, controlli, ingresso e
uscita) con le sue modifiche e il suo `CMakeLists.txt`. Nessuna cartella usa file delle altre o della radice. Tutti
accettano la stessa riga di comando di `main` (`--input`, `--traccia`, `-n`, `--txt`, `--max-backtrack`,
`--max-nodi`, `--progresso`) e scrivono la stessa traccia, quindi il sito li usa senza cambiare: nel modulo del test
c'è il menu «Solver».

Si compilano come `main`, dalla loro cartella:

```bash
cd solutori/in-avanti
gcc -O2 -std=gnu11 -w main.c data_initialization/database_initializers.c printer/printer.c \
    priority_queue/prio_queue.c ingresso_uscita/*.c -o /tmp/solver-in-avanti -lm
```

## I tre solver

- **`per-classe`**: la ricerca di `main`, ma dentro ogni giorno riempie una classe alla volta (tutte le ore di C01,
  poi C02...) invece di un'ora alla volta. Le regole di una classe (ore contigue, massimo di ore, ingresso) si vedono
  subito; i conflitti tra classi alla stessa ora si vedono più tardi. L'orario resta indicizzato per cella come in
  `main`; la traccia ha in più `"ordine"` nei dati (la cella decisa a ogni passo) e scrive posizioni e orari in
  quell'ordine.
- **`in-avanti`**: la ricerca di `main` con un controllo in avanti. Dopo ogni scelta, ogni cella obbligatoria che resta
  nella giornata deve avere ancora almeno un docente possibile (ha ancora ore nella classe, è libero a quell'ora, è
  già entrato, non ha già il massimo di ore oggi nella classe, il suo blocco non è chiuso, non ha già il massimo di
  classi oggi); se no, la scelta si scarta subito. Nella traccia è un controllo in più, alla fine delle coppie e del
  «da solo»: `lascia_un_docente_alle_celle_di_oggi`. Sui dati di `inizializza_dati()` dà lo stesso `orario.txt` di
  `main` (md5 `777dc4909e4e48ed721bf0a52d664e91`).
- **`due-livelli`**: livello 1, all'inizio di ogni giorno, le quote del giorno (quante ore fa oggi ogni docente in
  ogni classe, da solo e in ogni coppia), con i vincoli che si vedono senza l'orario delle ore e guardando che i giorni
  dopo restino fattibili; livello 2, l'orario del giorno con la ricerca di `main` (una classe alla volta) e i controlli
  sulle quote del giorno invece che sui totali della settimana. In più: le ore buche solo a quota finita, e il
  controllo `lascia_spazio_alle_ore_di_oggi_della_classe` (le ore di un docente in una classe sono un blocco: chi ha
  cominciato deve continuare, chi non ha cominciato deve trovare spazio). Ogni tentativo di disporre un giorno ha un
  limite di nodi (`NODI_PER_TENTATIVO`): se è stato superato almeno una volta e non si trova niente, l'esito è «non
  trovato» invece di «nessuna soluzione», perché non tutto è stato provato.

## Due controlli che `main` non ha

Il verificatore delle prove (indipendente dal solver) ha trovato orari di `main` che non rispettano le regole:

1. `main` controlla l'ora di ingresso solo per il primo docente di una coppia: il codocente può essere messo prima
   del suo ingresso;
2. il controllo finale delle ore (`verifica_ore_classe_materia`) è commentato: una classe può chiudere la settimana
   con ore buche al posto di lezioni che le mancavano.

I tre solver qui hanno tutti e due i controlli: `verifica_ora_ingresso` anche per il codocente (nella traccia, alla
fine dei controlli di coppia) e, alla fine, ogni classe deve aver fatto tutte le sue ore.
