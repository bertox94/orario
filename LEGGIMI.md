# Orario scolastico

Il programma dell'orario, riscritto da capo il 20 novembre 2026: il solver in C e il sito. Il programma di prima è
nel ramo `stabile/2026-11-18`, intatto.

## Che cosa c'è

- `solver/`: il programma che cerca l'orario.
  - `scuola`: i dati (giorni, ore, classi, docenti, voci) e la lettura di un caso da file;
  - `genera`: i casi casuali, riproducibili da un seme;
  - `ricerca`: la ricerca; `limiti` e `classe_sola`: i conti che la tengono lontana dai rami senza uscita;
  - `verifica`: l'orario trovato, ricontrollato da capo senza usare la ricerca;
  - `passi`: ogni nodo della ricerca su file, per rivederla dal sito;
  - `main`: i comandi.
- `sito/`: il sito (Flask), con le prove in tempo reale e i test notturni.
- `prove/prova.py`: la prova automatica del solver.

## Usarlo a mano

```
make
./orario genera 10 3 1 > caso.txt          # seme 10, 3 classi, con ritardi
./orario risolvi caso.txt --limite 2000000  # stampa lo stato, in JSON
```

Un caso è un file di testo, una riga per cosa, con i campi separati da TAB:

```
giorni	Lun	Mar	Mer	Gio	Ven	Sab
ore	08	09	10	11	12	13
obbligatorie	5
docente	Sirius	0
docente	Zelda	2
lezione	C01	4	Sirius
lezione	C01	2	Sirius	Zelda
```

`obbligatorie`: le prime ore di ogni giorno, che hanno sempre lezione. `docente`: il nome e l'ora (da 0) da cui è a
scuola. `lezione`: la classe, le ore in una settimana, il docente e, se sono in due, il codocente. Più righe con la
stessa classe e gli stessi docenti si sommano.

## Come cerca

L'orario si riempie un'ora alla volta, da lunedì alla prima ora a sabato all'ultima. In ogni ora si decide per prima
la classe che ha meno scelte; le scelte si provano mettendo per primi i docenti con meno margine, poi chi sta
continuando le sue ore di seguito, poi le voci quasi finite. A ogni passo si guardano i limiti: le ore di un docente
in tutto e in ogni classe, le lezioni possibili nelle prime ore di una classe, chi deve insegnare per forza adesso, e
se la classe, guardata da sola, ha ancora un orario. Quando da un punto non esce niente si torna indietro di una
scelta e si prova la successiva. Il perché di ogni cosa è nei commenti di
`solver/ricerca.h`, `solver/limiti.c` e `solver/classe_sola.c`.

## Il sito

```
pip install -r sito/requirements.txt
python3 -m sito avvia --porta 8000           # sul Raspberry: --host 0.0.0.0
```

Serve `gcc`: il sito compila il solver da solo. I suoi file stanno in `sito/lavoro/` (o nella cartella di
`SITO_LAVORO`): `prove/` per le prove in tempo reale, `notti/` per i test notturni.

- **Prove in tempo reale**: si sceglie un caso e si guarda la ricerca mentre va; poi la si rivede passo per passo, con
  l'orario a ogni passo e, dove la ricerca non va avanti, il conto che non torna con i pezzi da cui vengono i suoi
  numeri (quante ore oggi e nei giorni dopo, voce per voce, o perché ogni classe non prende un docente). «Mostra nell'orario» colora le
  celle che il conto ha contato (già fatte, possibili, non possibili, da riempire per forza), con la legenda sopra
  l'orario: le calcola il solver (`orario nodo`), con le stesse funzioni che fanno il conto. I tasti «Ritorno» saltano da un ritorno indietro all'altro; un clic su una cella dell'orario sceglie solo
  i suoi ritorni, e dice quanti sono. Da un passo all'altro le celle cambiate sono segnate (bordo lilla; quelle tolte restano scritte, sbarrate). Accanto ai ritorni di una cella c'è quanti sono dall'ultimo passo avanti (si azzerano quando la ricerca arriva
  più lontano di sempre): una cella superata non è più il problema. La sezione «I ritorni, per controllo e per chi» li conta per il controllo che non è passato e per il docente o
  la classe che nomina: dice chi è il problema, non in quale cella è stato visto. Ogni controllo ha una lettera (da A a G) e un nome; la sezione «I controlli,
  docente per docente», chiusa finché non la si apre, mostra per ogni docente i numeri dei suoi controlli a quel
  passo. Una prova che si è fermata al limite di nodi si può mandare avanti di altri 100 o 1000 nodi
  («Continua per altri»): riprende dal punto esatto in cui era. Una prova può arrivare a 1000 milioni di nodi (19 MB di disco per milione: il sito controlla che bastino); il profilo mostra tutta la ricerca, e un clic porta a quel punto.
- **Test notturni**: un elenco di casi rieseguiti in un giro, con un limite di nodi unico; il giro può partire da solo
  ogni notte. «I cambiamenti» mostra, una tabella per tipo, i casi il cui risultato non è più quello di cui si è
  presa visione; «Visto» lo conferma.

## La prova automatica

```
python3 prove/prova.py              # 52 casi: esito, nodi e verifica dell'orario
python3 prove/prova.py --aggiorna   # dopo una modifica voluta alla ricerca
```

## Il confronto con il programma di prima

Stessi casi (il generatore dà gli stessi di prima: controllato su 420), limite di 2 milioni di nodi:

| Casi | Orari trovati, prima | Orari trovati, adesso | Nodi, prima | Nodi, adesso |
|---|---|---|---|---|
| semi 0–19, classi 3–14 (480) | 306 | 323 | 74 milioni | 36 milioni |
| semi 20–39, classi 3–14 (480), mai usati per la messa a punto | 310 | 326 | 87 milioni | 52 milioni |
| semi 0–5, classi 20–25 (72) | 33 | 44 | 37 milioni | 14 milioni |

Ogni orario trovato passa la verifica, e i casi dichiarati impossibili sono gli stessi. Nei tre gruppi questo
programma trova 53 orari che quello di prima non trova, e ne perde 9 che quello di prima trova.

Una versione che ripartiva da capo quando si incagliava (a tentativi, con le scelte rimescolate) ne trovava di più:
330, 341 e 48, senza perderne nessuno, e risolveva anche seme 10, 3 classi, con ritardi (406.467 nodi), che senza
ripartenze non si risolve nemmeno con 20 milioni. Tolta il 20 novembre 2026 su richiesta di Alb; è nel commit
`8af8001` di questo ramo.
