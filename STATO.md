# La situazione al 13 novembre 2026

Questo file descrive il punto fermo `stabile/2026-11-13`: che cosa c'è, che cosa è acceso e che cosa resta aperto.
`main` e lo stabile coincidono nel commit in cui questo file è entrato. Come lavorare sul progetto è scritto in
`CLAUDE.md`; come avviare il sito in `sito/LEGGIMI.md`.

## Il solver

- **La ricerca** (`ricerca/`): backtracking cella per cella, un orario solo, senza variabili globali.
- **La verifica dei dati** (`verifiche/verifica_dati.c`), prima della ricerca: sei controlli; se uno non vale l'esito è
  «errore nei dati» e la ricerca non parte. La frase del terzo (le classi-giorno del docente) è quella scelta il 13.
- **Le potature** (`ricerca/potature.c`), durante la ricerca: le stesse sei condizioni rifatte a ogni nodo su quello che
  resta, più la settima (una classe, guardata da sola, deve avere ancora un orario suo). Accese la 1, la 2, la 5 e la
  7; la 3, la 4 e la 6 sono nel codice ma spente. Scartano solo rami senza orario: dove la ricerca senza potature
  trova un orario, esce lo stesso orario con gli stessi nodi o meno.
- **La verifica dell'orario** (`verifiche/verifica_orario.c`), dopo la ricerca: ricontrolla da capo l'orario trovato.
- **I comandi** (`comandi/`): `test`, `caso`, `dati`, `passo`; i casi casuali hanno anche l'ordine delle classi.
- **Le misure** su 341 casi casuali (semi 0–19, classi 3–14, senza e con ritardi), a 2 milioni di nodi: 175 orari
  trovati senza potature, 306 con quelle accese, nessuno perso.

## Il sito

- **Test in tempo reale**: il modulo, lo storico (con «Rifai» e le caselle per eliminare), la pagina di un test con i
  dati del caso, l'orario passo per passo (per docenti e per classi, con i salti di 10 e 100 passi), i controlli di
  ogni cella e il riquadro della potatura che fa tornare indietro.
- **Test notturni**: l'elenco dei casi, un limite di nodi unico, il giro che li riesegue e li confronta con la volta
  prima, le 5 esecuzioni precedenti di ogni test, il tasto «In tempo reale», le caselle per eliminare, i gruppi (per
  seme, classi, ritardi, ordine o esito).
- **Il giro della notte**: parte da solo all'ora scelta; prima aggiunge i casi di base (semi e classi scelti nella
  pagina, senza e con ritardi) con un ordine delle classi nuovo, uno in più ogni sera. Se finisce dopo l'ora di fine,
  la sera dopo non aggiunge niente.
- **La grafica**: in rosa, con la testata, i moduli nei riquadri e le ore in coppia in blu polvere; tutti i colori
  sono variabili in cima a `sito/static/stile.css`.

## I rami

- `main`: tutto il lavoro.
- `stabile/2026-11-13`: questo punto fermo. `stabile/2026-11-12` e `stabile/2026-11-11`: quelli di prima.
- `cella-piu-avanti`: il `main` di prima del 13, con la sezione «La cella più avanti» e il comando `ultima`, che
  qui non ci sono.
- `potature`: coincide con un commit di `main`, non serve più.

## Che cosa resta aperto

- Riportare «La cella più avanti» in `main`, adattata a ordine delle classi, cartelle e potature.
- I 35 casi (su 341) che restano fermi al limite anche con le potature: guardare che cosa hanno in comune.
- `comandi/json.h` è usato anche da `dati/`: decidere se spostarlo.
- Quanto dura un giro della notte sul Raspberry: da misurare dopo la prima notte, per scegliere quanti semi tenere.
