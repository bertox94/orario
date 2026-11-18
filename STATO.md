# La situazione al 18 novembre 2026

Questo file descrive il punto fermo `stabile/2026-11-18`: che cosa c'è, che cosa è acceso e che cosa resta aperto.
`main` e lo stabile coincidono nel commit in cui questo file è entrato. Come lavorare sul progetto è scritto in
`CLAUDE.md`; come avviare il sito in `sito/LEGGIMI.md`.

## Il solver

- **La ricerca** (`ricerca/`): backtracking cella per cella, un orario solo, senza variabili globali. Che cosa deve
  fare la classe in una cella (lezione, scelta, ora buca) lo decide una regola sola, `stato_della_classe`, che vale con
  qualunque numero di ore obbligatorie (semplificata il 18 novembre: la ricerca è identica a prima).
- **La verifica dei dati** (`verifiche/verifica_dati.c`), prima della ricerca: sei controlli; se uno non vale l'esito è
  «errore nei dati» e la ricerca non parte. La frase del terzo (le classi-giorno del docente) è quella scelta il 13 novembre.
- **Le potature** (`ricerca/potature.c`), durante la ricerca: le stesse sei condizioni rifatte a ogni nodo su quello che
  resta, più la settima (una classe, guardata da sola, deve avere ancora un orario suo). Accese la 1, la 2, la 5 e la
  7; la 3, la 4 e la 6 sono nel codice ma spente. Scartano solo rami senza orario: dove la ricerca senza potature
  trova un orario, esce lo stesso orario con gli stessi nodi o meno.
- **La verifica dell'orario** (`verifiche/verifica_orario.c`), dopo la ricerca: ricontrolla da capo l'orario trovato.
- **I comandi** (`comandi/`): `test`, `caso`, `dati`, `passo` e `ostacoli`, che rilegge i passi salvati e ne ricava i
  backtrack; i casi casuali hanno anche l'ordine delle classi.
- **Le misure** su 341 casi casuali (semi 0–19, classi 3–14, senza e con ritardi), a 2 milioni di nodi: 175 orari
  trovati senza potature, 306 con quelle accese, nessuno perso.

## Il sito

- **Test in tempo reale**: il modulo, lo storico (con «Rifai» e le caselle per eliminare) e la pagina di un test.
  Nella pagina: i dati del caso, con classi, docenti e codocenze su una riga di linguette; poi due sezioni, ognuna
  con il suo slider subito sopra il suo orario. «La ricerca, passo per passo»: l'orario a ogni passo (per classi e,
  chiuso finché non lo si apre, per docenti), con i salti di 10 e 100 passi e i tasti «Ritorno». «I ritorni
  indietro»: una cella per volta, con lo slider sui ritorni da quella cella e l'orario di quando la ricerca si è
  incagliata lì; in fondo il profilo della ricerca e gli ostacoli che hanno richiesto più passi, scritti
  «cella 81 (25)», con il percorso di quello scelto (30 29 30 31, con le ripetizioni raccolte).
- **I controlli a ogni ritorno**: le spiegazioni di prima (potatura, controllo della classe, coda, controlli dei
  docenti) sono state tolte per rifarle una alla volta sotto l'orario del ritorno. Per ora c'è la prima: che cosa
  deve fare la classe.
- **Test notturni**: l'elenco dei casi raggruppato per numero di classi, e dentro ogni numero una tabella con una
  riga per caso (prima i semi senza ritardi, poi quelli con) e una colonna per ordine delle classi; un limite di nodi
  unico; il giro che li riesegue; il report caso per caso in quattro gruppi (orario impossibile, trovato con ogni
  ordine, trovato solo con alcuni ordini, mai trovato) e «Da verificare» con la presa visione. Il numero di un test
  eliminato non si riusa e la storia di un test parte dalla sua aggiunta.
- **Il giro della notte**: parte da solo all'ora scelta; prima aggiunge i casi di base (semi e classi scelti nella
  pagina, senza e con ritardi) con un ordine delle classi nuovo, uno in più ogni sera. Se finisce dopo l'ora di fine,
  la sera dopo non aggiunge niente.
- **La grafica**: in rosa, con il lilla per le sfumature, il nome del sito in corsivo, la testata, i moduli nei
  riquadri e le ore in coppia in blu polvere; tutti i colori sono variabili in cima a `sito/static/stile.css`.

## I rami

- `main`: tutto il lavoro.
- `stabile/2026-11-18`: questo punto fermo. `stabile/2026-11-13`, `stabile/2026-11-12` e `stabile/2026-11-11`:
  quelli di prima.
- `cella-piu-avanti`: il `main` di prima del 13 novembre, con la sezione «La cella più avanti» e il comando `ultima`, che
  qui non ci sono.
- `potature`: coincide con un commit di `main`, non serve più.

## Che cosa resta aperto

- Riportare «La cella più avanti» in `main`, adattata a ordine delle classi, cartelle e potature.
- I 35 casi (su 341) che restano fermi al limite anche con le potature: guardare che cosa hanno in comune.
- `comandi/json.h` è usato anche da `dati/`: decidere se spostarlo.
- I controlli a ogni ritorno, uno alla volta: dopo «che cosa deve fare la classe» vengono i controlli dei docenti
  (una riga per docente, con la risposta e il primo motivo che lo ferma), le coppie, la coda e le potature.
- Segnare nell'orario di un ritorno che cosa è cambiato rispetto al ritorno prima dalla stessa cella.
- Test notturni: al posto dell'unica tabella «Da verificare», una tabella per tipo di cambiamento (non risolto →
  risolto, risolto → non risolto, meno nodi, più nodi, allarmi). Idea di Alb del 18 novembre, da fare più avanti.
- L'ordine delle classi: su 369 casi con ritardi, mettere prima le classi con meno ore ne trova 246, l'ordine
  generato 233, a caso 224, prima quelle con più ore 185 (prove del 16 novembre, fuori dal repository). Farne un
  ordine vero della ricerca cambia come il solver calcola: solo se Alb lo chiede.
- Il caso seme 10, 3 classi, con ritardi: nessun ordine trova l'orario e nessuno dimostra che non c'è. C03 ha
  pochissimo margine nelle prime ore (tre dei suoi docenti entrano tardi) e la ricerca se ne accorge solo venerdì.
- Quanto dura un giro della notte sul Raspberry: da misurare dopo la prima notte, per scegliere quanti semi tenere.
