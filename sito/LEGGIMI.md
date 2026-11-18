# Il sito: test in tempo reale

Due pagine, senza login e senza database:
- **Test e storico** (`/`): il modulo per avviare un test (seme, classi, ritardi, ordine delle classi, limite in milioni di nodi, da 0,1
  a 20) e lo storico di tutti i test fatti (esito, nodi, cella più avanti, tempo), con la × per eliminarne uno.
- **Un test** (`/test/<n>`): prima i dati del caso (un riepilogo; le classi a schede, con un quadretto per ogni ora
  di lezione, da solo o in coppia; i docenti con ore, ingresso, classi e codocenti; le codocenze con il grafo: le tre
  parti stanno su una riga di linguette e se ne apre una alla volta, o nessuna), poi la ricerca passo per passo: uno slider su tutti i nodi, dal primo all'orario
  completo o al limite, con «Prec.» e «Succ.» (o ← e →) e i salti di 10 e 100 passi; per ogni passo l'orario fin lì, con la cella che la ricerca
  sta per decidere (gialla), la scelta appena messa (rosa) e quando torna indietro. Quando la ricerca torna indietro la pagina
  lo dice, con la cella da cui riparte; i tasti «Ritorno» portano al ritorno indietro prima o dopo. In fondo alla sezione dei ritorni, «Il profilo e gli ostacoli»: il profilo della
  ricerca (per ogni tratto, da quale cella a quale si è mossa: si vede dove avanza e quanto torna indietro; un clic porta
  a quel punto) e gli ostacoli che hanno richiesto più passi. Un ostacolo è una cella da cui la ricerca torna indietro,
  finché non la supera: «cella 81 (25)» vuol dire che per superare la 81 è dovuta tornare indietro fino a 25 celle.
  Un clic su un ostacolo porta al suo primo ritorno e ne scrive il percorso con le celle su cui la ricerca lavora
  (30 29 30 31: si incaglia alla 30, cambia la 29, rimette la 30, arriva alla 31), con le ripetizioni raccolte:
  30 (29 30)⁴ 31. Mentre il test gira si aggiornano ogni quattro secondi. I numeri vengono dal comando `ostacoli` del
  solver, che rilegge i passi salvati. Mentre il test gira lo slider
  si allunga e, se è in fondo, segue la ricerca; c'è il pulsante «Ferma». Le celle hanno tutte la stessa misura.
  Le sezioni sono due, ognuna con il suo slider subito sopra il suo orario, e si muovono per conto loro: «La ricerca,
  passo per passo» e «I ritorni indietro», che si guardano una cella per volta: si sceglie la cella da cui la ricerca
  torna indietro (all'inizio la più avanti), lo slider ha una posizione per ognuno dei suoi ritorni e l'orario è quello
  di quando la ricerca si è incagliata lì, con la cella in giallo («vai a questo passo» porta lì lo slider dei passi).
  Ogni volta che la ricerca torna a incagliarsi nella stessa cella, i ritorni dalle celle prima sono stati superati.
  Il sito tiene l'indice dei ritorni in `ritorni.idx` e `ritorni.celle`, accanto ai passi. Sotto l'orario dei passi, chiuso finché non lo si apre, lo stesso passo visto dai docenti: una riga per docente, con la classe dove insegna a ogni ora, con chi è
  in coppia e, tratteggiate, le ore prima del suo ingresso.
  Sotto la frase del passo, se da lì la ricerca torna indietro, c'è scritto da quale cella riparte. Le spiegazioni del perché di un ritorno (la
  potatura, il controllo della classe, la coda di priorità, i controlli dei docenti) sono state tolte il 18 novembre
  2026 per rifarle una alla volta: il comando `passo` del solver continua a calcolarle. Tornano una alla volta sotto l'orario del
  ritorno, in «I controlli nella cella del ritorno»: una riga per controllo, con il nome, se vale e i numeri da cui
  esce. Per ora: che cosa deve fare la classe (deve avere lezione, può scegliere, deve avere un'ora buca).
  Quando l'orario è trovato, in cima c'è «L'orario trovato»: l'esito della verifica indipendente del solver
  (`verifica_orario.h`), controllo per controllo, e un pulsante che porta lo slider all'orario completo.
  Se i dati non passano la verifica del solver (`verifica_dati.h`) l'esito è «errore nei dati»: la ricerca non parte e
  la pagina mostra il motivo, sopra i dati del caso.

- **Test notturni** (`/notturni`): un elenco di casi (seme, classi, ritardi, ordine delle classi) che si rieseguono
  tutti insieme, in un giro. Un caso si aggiunge dalla pagina di un test in tempo reale («Aggiungi ai notturni»). Il
  limite di nodi è uno solo per tutti e si cambia dalla pagina. «Avvia il giro» esegue i test uno dopo l'altro e, per
  ognuno, confronta esito, nodi e cella più avanti con l'ultima volta fatta con lo stesso limite: «uguale», «diverso»,
  «primo» o «limite cambiato». Ogni test si può rifare in tempo reale con il suo tasto (stesso caso, con il
  limite dei notturni fino al massimo dei test in tempo reale), per vederlo passo per passo. L'elenco è raggruppato per numero di classi, con i gruppi chiusi:
  ogni gruppo mostra quanti test ha per esito e per giudizio rispetto alla volta prima, e si apre con un clic. Aperto, mostra una tabella con una
  riga per caso (prima i semi senza ritardi, poi quelli con, ogni parte con il conto dei suoi esiti) e una colonna per ordine delle classi: ogni casella è un test, colorata secondo
  l'esito, con i nodi se l'orario c'è o il punto dell'orario dove la ricerca si è fermata, e un segno se è cambiato
  dalla volta prima; un clic sulla casella apre il test sotto la tabella. I test si tolgono con la × della riga, o tanti
  insieme scegliendoli con le caselle («Elimina i test scelti»). Il numero di un test eliminato non si riusa: i giri ricordano i
  risultati per numero, e la storia di un test comincia dal giro dopo la sua aggiunta. Con un clic sulla riga di un test si vedono le sue 5 esecuzioni precedenti (di qualunque
  limite), una riga l'una nelle stesse colonne della tabella, e i loro tempi. «Rispetto a prima» dice com'è andata
  ogni esecuzione rispetto a quella prima con lo stesso limite: uguale, migliorato o peggiorato (ora c'è una risposta
  e prima no, o il contrario; oppure la stessa risposta con meno o più nodi), più avanti o più indietro (ferma al
  limite tutte e due le volte, con la cella più avanti cambiata), con i numeri accanto; nella riga del test c'è anche
  lo stesso giudizio su tutte le esecuzioni che si vedono, dalla più vecchia all'ultima. Sopra l'elenco dei test c'è il report, caso per caso: un caso (seme, classi, ritardi) sta su una riga con tutti i
  suoi ordini delle classi, dall'ultima esecuzione di ognuno. I casi sono in quattro gruppi secondo il loro stato:
  orario impossibile (con ogni ordine nessuna soluzione o errore nei dati), orario trovato con ogni ordine, trovato
  solo con alcuni ordini, mai trovato (fermo al limite di nodi con tutti); con quali ordini sì e quali no, i nodi del
  migliore, il record (la stessa risposta con meno nodi di sempre, con quel limite) e la cella più avanti di quelli
  che non lo trovano. Lo stesso stato è in fondo a ogni riga della tabella di un numero di classi, e la riga del
  gruppo conta i casi per stato. Prima del report, «Da verificare»: i casi che hanno almeno un ordine (ne basta uno)
  il cui ultimo risultato si scosta dal meglio di sempre fatto con lo stesso limite di nodi (cambiando il limite il
  meglio di sempre riparte da capo): gli allarmi (trovato dove una volta, con qualunque limite, era
  impossibile, o il contrario; un test non riuscito; un orario che non passa la verifica), i migliorati (una risposta
  dove c'era solo il limite, o meno nodi del record) e i peggiorati (più nodi del record, o fermo al limite dove
  aveva una risposta). Un test fermo al limite che non ha mai avuto una risposta non ci entra. I test restano lì
  finché non si conferma la presa visione con «Visto» (uno, o tutti): poi tornano solo se il risultato cambia ancora
  (le conferme stanno in `lavoro/notturni/visti.json`). Dei test notturni si tengono solo i
  numeri (in `lavoro/notturni/`), non i passi. Con il giro della notte acceso, ogni notte all'ora scelta nella pagina
  si aggiungono all'elenco i casi di base con un ordine delle classi nuovo, e parte il giro. I casi di base si
  scelgono nella pagina (semi da 0 a…, classi da… a…, senza e con ritardi; senza filtri: che il caso abbia un errore nei
  dati, un orario o nessuna risposta non conta). La prima sera entrano con l'ordine 0, quello generato, poi ogni sera
  con l'ordine successivo: stessi dati, classi in un altro ordine. Il giro della notte dovrebbe finire entro l'ora di
  fine (dalle 22 alle 8, se non si cambiano): se va oltre finisce lo stesso, ma dalla sera dopo non si aggiungono altri
  ordini finché un giro della notte non torna a finire in tempo; i test si tolgono a mano, con ×. Se all'ora di
  partenza gira altro la notte salta, senza test nuovi (e la pagina lo dice); se il sito era spento si aspetta la notte
  dopo. Un giro e un test in tempo reale non girano
  insieme.

I test dello storico si eliminano con la × della riga, o tanti insieme scegliendoli con le caselle. Un test si può rifare con «Rifai», dallo storico o dalla sua pagina: parte un test nuovo sullo stesso caso, con lo
stesso limite. Un test alla volta, con un limite di nodi da 0,1 a 20 milioni (ogni passo si registra: 6 byte per nodo).

Il sito mostra soltanto: tutto il calcolo lo fa il solver, che il sito lancia in tre modi (vedi `main.c`):
- `./untitled caso SEME CLASSI RITARDI ORDINE LIMITE FILE PASSI` (`caso.h`): la ricerca; scrive lo stato in `FILE` ogni mezzo
  secondo e ogni nodo in `PASSI` (`passi.h`);
- `./untitled dati SEME CLASSI RITARDI ORDINE`: i dati del caso (lezioni e ore di ingresso);
- `./untitled passo SEME CLASSI RITARDI ORDINE PASSI K` (`passo.h`): il nodo K, con l'orario fin lì, la potatura
  (se una fa tornare indietro), che cosa deve fare la classe con i suoi numeri, la coda e i controlli dei docenti.

Ogni test è una cartella in `sito/lavoro/test/<n>` (o in `SITO_LAVORO`) con `info.json`, `stato.json` e i passi.

## Avvio

Serve Python 3 con Flask (e, se c'è, waitress) e gcc: il solver si compila da solo al primo test e quando cambiano i
sorgenti.

```bash
python3 -m pip install -r sito/requirements.txt
python3 -m sito avvia --porta 8000                  # sul Mac: http://127.0.0.1:8000
python3 -m sito avvia --host 0.0.0.0 --porta 8000   # sul Raspberry: visibile da tutta la rete di casa
```
