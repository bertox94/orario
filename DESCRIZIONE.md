# Orario scolastico: descrizione completa

Questo documento descrive tutto il programma, il solver e il sito, con il dettaglio che serve per rifarli da zero
senza avere il codice: che cosa fanno, con quali dati, con quali regole e numeri, che cosa mostrano, quali comandi e
quali indirizzi hanno. Dove un numero o un ordine conta (perché cambiandolo cambiano i risultati) è scritto.

Stato descritto: `main` al 21 novembre 2026.

Indice

1. A che cosa serve
2. Le parole
3. La scuola e le regole dell'orario
4. Il file di un caso
5. Il generatore dei casi casuali
6. La ricerca
7. I controlli (da A a G)
8. La verifica dell'orario trovato
9. Riprendere una ricerca, e partire da una scelta fatta a mano
10. I comandi del solver
11. Lo stato della ricerca (JSON)
12. I file dei passi
13. Il sito: com'è fatto
14. La pagina «Prove in tempo reale»
15. La pagina di una prova
16. La pagina «Test notturni»
17. Tutti gli indirizzi del sito
18. I limiti e i numeri fissi, in un posto solo
19. La prova automatica e le verifiche da rifare dopo una modifica
20. Installazione e avvio
21. I file del repository
22. Il metodo esatto

---

## 1. A che cosa serve

Dato l'elenco delle lezioni di una scuola (quale docente, o quale coppia di docenti, fa quante ore in quale classe),
il **solver** cerca un orario settimanale che rispetti le regole del capitolo 3: una lezione, o un'ora buca, in ogni
cella (una classe, in un'ora, in un giorno).

Il **sito** non calcola niente: lancia il solver e mostra il suo lavoro. Ha due parti.

- Le **prove in tempo reale**: si sceglie un caso, si guarda la ricerca mentre va, e poi la si rivede un passo alla
  volta, con l'orario a ogni passo e, dove la ricerca torna indietro, il controllo che non è passato e i suoi numeri.
- I **test notturni**: un elenco di casi che si rieseguono tutti insieme, per vedere che cosa cambia quando cambia il
  solver.

I casi sono **casuali ma riproducibili**: un caso è identificato da tre cose (un seme, il numero di classi, con o
senza ritardi) e le stesse tre danno sempre la stessa scuola.

## 2. Le parole

| Parola | Che cosa vuol dire |
|---|---|
| cella | Un posto dell'orario: una classe, in un'ora, in un giorno. |
| voce | Una lezione di una classe: un docente da solo, oppure una coppia (docente e codocente insieme), con le ore che devono fare in una settimana. |
| ora buca | Una cella senza lezione. |
| ingresso | L'ora (indice, da 0) da cui un docente è a scuola. Prima non può insegnare. Vale per tutti i giorni. |
| ore obbligatorie | Le prime ore di ogni giorno, che in ogni classe hanno sempre lezione. |
| nodo, o passo | Un punto della ricerca: un orario riempito in parte. Il primo è l'orario vuoto. |
| profondità | Quante celle sono decise in un nodo. |
| strada | Le scelte fatte per arrivare a un nodo: l'elenco (cella, voce) nell'ordine in cui le celle sono state decise. |
| resto | Le ore ancora da mettere: di una voce, di una classe, di un docente, di un docente in una classe. |
| controllo (o limite) | Un conto che deve tornare perché da un nodo possa ancora uscire un orario. Sono sette, da A a G. |
| motivo | Perché da un nodo non si va avanti: quale controllo non passa, per chi, e i suoi due numeri («servono» e «possibili»). |
| ritorno indietro | Un nodo con un motivo: la ricerca toglie l'ultima scelta e prova la successiva. |
| passo avanti | Un nuovo massimo di celle decise: la ricerca è arrivata più lontano di sempre. |

Numerazione. Giorni, ore, classi, docenti e voci sono numerati da 0. La cella di (giorno, ora, classe) ha il numero
`(giorno × ore + ora) × classi + classe`. Quindi le celle sono in ordine di tempo, e dentro la stessa ora in ordine
di classe. Da una cella: `classe = cella mod classi`, `ora della settimana = cella div classi`,
`giorno = ora della settimana div ore`, `ora = ora della settimana mod ore`.

Nell'orario una cella contiene il numero di una voce, oppure −1 (ora buca), oppure −2 (non ancora decisa).

## 3. La scuola e le regole dell'orario

Una scuola ha: i giorni e le ore (con i loro nomi), le classi, i docenti (ognuno con il suo ingresso), le voci, e il
numero di ore obbligatorie.

Limiti di grandezza: al massimo 7 giorni, 10 ore al giorno, 120 classi, 250 docenti, 4000 voci; nomi lunghi al
massimo 47 caratteri.

Le voci sono tenute in ordine di classe (poi di docente, poi di codocente); in una coppia il docente con l'indice
più basso viene per primo, così la stessa coppia nella stessa classe è sempre la stessa voce.

Le regole di un orario valido:

1. Ogni voce fa esattamente le sue ore, nella sua classe.
2. Un docente è in una sola classe per volta.
3. Un docente non insegna prima della sua ora di ingresso.
4. In una classe, in un giorno, un docente fa al massimo **3** ore (`MAX_ORE_DI_SEGUITO`).
5. Le ore di un docente in una classe, in un giorno, sono di seguito (senza buchi).
6. In un giorno un docente insegna in al massimo **3** classi (`MAX_CLASSI_AL_GIORNO`).
7. In ogni classe le prime ore di ogni giorno (le obbligatorie) hanno lezione.
8. Dopo un'ora buca, la classe quel giorno non ha più lezione.

Le regole 4 e 5 contano le ore di un docente in una classe sommando tutte le sue voci lì (da solo e in coppia).

## 4. Il file di un caso

Un file di testo, una riga per cosa, con i campi separati da TAB. Le righe vuote e quelle che cominciano con `#`
si saltano.

```
giorni	Lun	Mar	Mer	Gio	Ven	Sab
ore	08	09	10	11	12	13
obbligatorie	5
docente	Sirius	0
docente	Zelda	2
lezione	C01	4	Sirius
lezione	C01	2	Sirius	Zelda
```

- `giorni` e `ore`: i nomi, nell'ordine.
- `obbligatorie N`: le prime N ore di ogni giorno hanno sempre lezione (ridotto tra 0 e il numero di ore).
- `docente NOME INGRESSO`: l'ingresso è l'indice dell'ora (0 = dalla prima). Deve essere un'ora della giornata.
- `lezione CLASSE ORE DOCENTE [CODOCENTE]`: la classe nasce la prima volta che compare; i docenti devono essere già
  stati dichiarati; i due di una coppia devono essere diversi; le ore devono essere più di 0. Più righe con la stessa
  classe e gli stessi docenti si sommano in una voce sola.

Il file è rifiutato (con la riga dell'errore) se una riga non si capisce o supera i limiti, o se mancano giorni,
ore o lezioni.

## 5. Il generatore dei casi casuali

`orario genera SEME CLASSI RITARDI` scrive un caso nel formato del capitolo 4. Lo stesso seme, lo stesso numero di
classi e la stessa scelta sui ritardi danno **sempre lo stesso caso**: i test notturni e la prova automatica
rigenerano i casi dai semi, quindi cambiare qualunque cosa qui sotto (anche l'ordine in cui si chiedono i numeri
casuali) cambia tutti i casi e tutti i risultati di riferimento.

La scuola generata: 6 giorni (Lun … Sab), 6 ore (08 … 13), 5 ore obbligatorie. Le classi si chiamano C01, C02, …
I docenti prendono i nomi, nell'ordine, da un elenco fisso di personaggi di fantasia (`generatore/nomi.h`, circa 430
nomi, tutti diversi); oltre la fine dell'elenco il docente numero d si chiama `D` seguito da d + 1 su tre cifre.

**I numeri casuali.** Generatore xorshift64*: lo stato è un intero a 64 bit; per il prossimo numero
`x ^= x >> 12; x ^= x << 25; x ^= x >> 27;` lo stato diventa x e il numero è `x × 2685821657736338717` (modulo
2^64). `tra(da, a)` = `da + numero mod (a − da + 1)`. Seminare: `stato = seme × 0x9E3779B97F4A7C15 + 0x1234567`
(modulo 2^64; se viene 0 si usa 1), poi si buttano i primi 8 numeri.

**Un tentativo.** Si ripete, continuando con gli stessi numeri casuali, finché un tentativo riesce (al massimo
20.000; se nessuno riesce il caso «non si riesce a generare»).

1. `percento_in_coppia = tra(15, 60)`.
2. Per ogni classe, in ordine: `resto = tra(30, 36)` ore; finché resto > 0: `lunga = tra(1, 6)`,
   `ore = min(lunga, resto)`, si toglie da resto, e nasce una lezione di quelle ore; se
   `tra(1, 100) ≤ percento_in_coppia` la lezione è in coppia per `insieme = tra(1, ore)` delle sue ore. Si sommano
   `ore + insieme` nel totale delle ore di tutti i docenti. (Al massimo 4000 lezioni.)
3. Quanti docenti: `pochi = ceil(totale / 24)`, `tanti = totale div 5`; `da = max(ceil(totale / 20), pochi)`,
   `a = min(totale div 10, tanti)`; se da > a allora da = pochi, a = tanti; se ancora da > a, o a ≥ 250, il
   tentativo fallisce. `docenti = tra(da, a)`. (Cioè: in media tra 10 e 20 ore l'uno, mai meno di 5 né più di 24.)
4. Le lezioni si mescolano: per i da 0 all'ultima, si scambia la i con `tra(i, ultima)`.
5. Per ogni lezione, nell'ordine mescolato: il docente è «il meno carico fra tre»: tre volte `d = tra(0, docenti −
   1)`; si scarta d se è l'escluso o se con queste ore supererebbe 24; tra i rimasti quello con meno ore (a parità il
   primo estratto); se non ne resta nessuno il tentativo fallisce. Gli si aggiungono le ore. Se la lezione è in
   coppia, allo stesso modo si sceglie il codocente (escluso il docente) per le ore `insieme`.
6. Il tentativo riesce se nessun docente ha meno di 5 ore, e ci sono almeno una lezione in coppia e una da soli.

**I ritardi** (solo se chiesti, dopo il tentativo riuscito). `quanti = tra(1, docenti)`; si prendono i primi
`quanti` di un ordine casuale (per i da 0: si scambia il posto i con `tra(i, docenti − 1)`). Per ogni docente, in
ordine di indice, se è tra quelli: `ingresso = tra(1, 3)`, poi si anticipa di un'ora alla volta finché le sue ore
stanno in `(6 − ingresso) × 6`. Con i ritardi un caso può non avere nessun orario.

**Che cosa si scrive.** Una riga di commento con seme, classi e ritardi; giorni, ore, obbligatorie; i docenti in
ordine di indice con il loro ingresso; poi, classe per classe, le lezioni nell'ordine mescolato: per ognuna una riga
con le ore da solo (`ore − insieme`, se sono più di 0) e una riga con le ore in coppia (`insieme`, se ci sono).

## 6. La ricerca

È una ricerca in profondità con ritorno indietro (backtracking). Sempre la stessa per lo stesso caso: niente di
casuale.

### 6.1 Lo stato

Oltre all'orario si tengono, aggiornati a ogni scelta messa o tolta:

- le ore che restano a ogni voce, a ogni classe, a ogni docente, e a ogni docente in ogni classe;
- per ogni ora della settimana e docente: se è occupato (insegna in una classe);
- per ogni giorno, classe e docente: quante ore ha fatto quel giorno in quella classe;
- per ogni giorno e docente: in quante classi ha insegnato quel giorno;
- per ogni giorno e classe: se è «chiusa» (ha già avuto un'ora buca);
- per ogni classe: le ore buche che deve ancora avere (le ore della sua Buca, paragrafo 6.3).

Mettere una scelta in una cella: se è un'ora buca, la classe quel giorno diventa chiusa; se è una voce, un'ora in
meno per la voce, la classe e ognuno dei suoi docenti (in tutto e in quella classe), i docenti diventano occupati in
quell'ora, e per ognuno cresce il conto delle ore di oggi in quella classe (se era 0, cresce anche il conto delle
classi di oggi). Togliere fa le stesse cose al contrario.

### 6.2 L'ordine delle celle

L'orario si riempie **un'ora alla volta**: lunedì alla prima ora, poi alla seconda, … fino all'ultima ora
dell'ultimo giorno. Con `profondità` celle decise, l'ora che si sta decidendo è `profondità div classi`.

Dentro un'ora si decide **per prima la classe con meno scelte** tra quelle non ancora decise (a parità quella con
l'indice più basso). Se una classe non ha scelte è il controllo G; se una classe ne ha una sola la prende subito,
prima che un'altra le porti via il docente.

### 6.3 Le scelte di una classe in un'ora

Sia `dopo` il numero di giorni dopo oggi, e `obbligatoria` vero se l'ora è una delle prime.

**La Buca.** Le ore buche di una classe sono trattate come le ore di un docente tutto suo, la Buca di quella classe
(dal 26 novembre 2026):
- le sue ore in una settimana sono le celle della classe (`giorni × ore`) meno le ore di lezione della classe;
- «entra» dopo le ore obbligatorie: prima non può stare;
- quando in un giorno comincia, va avanti fino all'ultima ora (dopo un'ora buca la classe quel giorno non ha più
  lezione); non ha il massimo di 3 ore di seguito;
- i **posti** dove può ancora andare dall'ora `da` di oggi: `dopo × (ore − obbligatorie) + max(0, ore − max(da,
  obbligatorie))`;
- **deve** cominciare in quest'ora se le ore che le restano sono più dei posti dall'ora dopo;
- **può** stare in quest'ora se l'ora non è obbligatoria e le ore che le restano sono almeno `ore − ora` (bastano
  per arrivare alla fine della giornata).

Le scelte:
- Si può mettere una **lezione** se l'ora è obbligatoria, oppure se la Buca oggi non ha già cominciato e non deve
  cominciare adesso.
- Si può mettere l'**ora buca** se la Buca può stare in quest'ora.
- Una **voce** si può mettere (quando la lezione è ammessa) se ha ancora ore e ognuno dei suoi docenti «può adesso».

(Sono le stesse scelte di prima scritte in un altro modo: «la Buca deve» equivale a «le ore di lezione che restano
sono al massimo `dopo × obbligatorie`», e «la Buca può» a «le ore di lezione che restano stanno nei giorni dopo».)

Un docente **può adesso** in una classe se: è entrato (ingresso ≤ ora); in quest'ora non è occupato; e, se oggi in
quella classe non ha ancora insegnato, oggi è in meno di 3 classi; se invece oggi lì ha già insegnato, ha fatto meno
di 3 ore e c'era nell'ora prima. Quando non può, il perché (uno solo, in quest'ordine) è: non entrato, altrove,
troppe classi, massimo raggiunto, interrotto.

Le scelte sono le voci possibili, nell'ordine delle voci, e in fondo l'ora buca se ammessa.

### 6.4 L'ordine in cui si provano le scelte

All'inizio di ogni ora, per ogni docente, si calcola (controllo A, capitolo 7): `libere` = le ore in cui è a scuola
da adesso alla fine della settimana; `margine = libere − ore che gli restano`; `deve` = vero se le ore che gli
restano sono più delle libere contate dall'ora dopo (cioè: se salta quest'ora non ce la fa più).

A ogni scelta si dà un numero, e si provano dal più basso:

`(margine più basso tra i docenti della voce × 2 + (0 se uno dei suoi docenti oggi in questa classe ha già
insegnato, 1 se no)) × 1000 + ore che restano alla voce`

Cioè: prima il docente con meno margine; a pari margine chi sta continuando le sue ore; a pari anche questo la voce
a cui restano meno ore.

**L'ora buca è in questa coda come un docente** (dal 26 novembre 2026; prima veniva sempre dopo tutte le lezioni). Il
suo numero è quello di un docente che non sta continuando: `(margine × 2 + 1) × 1000 + ore che restano alla Buca`,
con `margine = posti dove la Buca può ancora andare da quest'ora − ore che le restano`.

A parità completa resta l'ordine delle scelte (le voci nel loro ordine, poi la buca: ordinamento stabile, per
inserimento).

Dopo aver scelto la classe si controlla anche F: un docente che `deve`, non è occupato in quest'ora, e non compare
tra le scelte di nessuna classe non ancora decisa.

### 6.5 Un nodo

1. Se c'è un limite di nodi e i nodi contati lo hanno raggiunto: esito «limite di nodi», e si esce da tutto.
2. Si conta il nodo. Se la profondità è un nuovo massimo: è un **passo avanti** (si azzerano i conti «recenti» dei
   ritorni).
3. Se tutte le celle sono decise: esito «trovato».
4. Altrimenti si guardano i controlli, in quest'ordine, fermandosi al primo che non passa:
   - i controlli **dopo la scelta** (se c'è un'ultima scelta): D, B ed E per la classe appena decisa e i suoi
     docenti, poi B per le altre classi dei docenti appena messi (capitolo 7);
   - se si sta cominciando un'ora (profondità multipla del numero di classi), i controlli **di inizio ora**: A per
     tutti i docenti, poi per ogni classe D, B (e, solo alla primissima ora della settimana, prima C e alla fine E);
   - la scelta della classe, con G e F.
5. Se un controllo non è passato, il nodo è un **ritorno indietro**: si conta (in tutto, per la cella dell'ultima
   scelta, per controllo e per chi) e non ha scelte.
6. Chi segue la ricerca viene avvisato (i passi su file, lo stato): una volta per nodo.
7. Le scelte si provano una per una: si mette, si scende in un nodo nuovo, e se la ricerca non è finita si toglie.

Alla fine: «trovato» (l'orario è completo), «impossibile» (tutte le scelte provate senza trovare: un orario non
esiste), o «limite di nodi».

## 7. I controlli (da A a G)

Sono condizioni **necessarie**: scartano solo rami in cui un orario non può esserci, quindi non fanno perdere
nessun orario. Ognuno confronta quello che **serve** con quello che è ancora **possibile** (contato con ottimismo);
se serve più del possibile, non passa.

In tutti: «oggi» è il giorno dell'ora che si sta decidendo; `dopo` = giorni dopo oggi; la **prima ora libera** di
una classe è l'ora che si sta decidendo, o la successiva se la classe in quest'ora è già decisa.

**Quanto può ancora fare oggi** un docente t in una classe k, dall'ora `da` (la prima libera di k) all'ora `fino_a`:
- 0 se `da` è oltre l'ultima ora o la classe oggi è chiusa;
- se oggi lì ha già insegnato (`fatte` ore): 0 se nell'ora `da` è in un'altra classe o se nell'ora prima di `da`
  non c'era in k; altrimenti comincia da `da` e può farne al massimo `3 − fatte`;
- se oggi lì non ha insegnato: 0 se oggi è già in 3 classi; altrimenti comincia da `max(da, ingresso)`, o dall'ora
  dopo se comincerebbe da `da` ma in quell'ora è in un'altra classe; al massimo 3;
- il risultato è `max(0, min(massimo, fino_a − comincia + 1))`.

| Lettera | Nome | Di chi parla | Servono | Possibili |
|---|---|---|---|---|
| A | le ore di un docente in tutto | un docente | le ore che gli restano | `dopo × (ore − ingresso) + max(0, ore − max(ora, ingresso))` |
| B | le ore di un docente in una classe | un docente e una classe | le ore che gli restano lì (se sono 0 non si guarda) | `dopo × min(3, ore − ingresso)` + quanto può ancora fare oggi fino all'ultima ora |
| C | le celle di una classe (solo al primo nodo) | una classe | le ore di lezione della classe | le celle della settimana, `giorni × ore` |
| D | le lezioni per le prime ore | una classe e un'ora obbligatoria h | le celle da riempire fino ad h: `dopo × (h + 1) + max(0, h − prima ora libera + 1)` | la somma, sulle voci della classe con ancora ore, di `min(resto della voce, oggi + dopo × al_giorno)` |
| E | l'orario della classe, da sola | una classe | le ore che le restano | (nessun numero: si prova a costruirlo) |
| F | un docente che deve insegnare adesso | un docente | le ore che gli restano | le ore libere dall'ora dopo |
| G | una classe senza scelte | una classe | — | — |

Per D si guarda ogni ora obbligatoria h, dalla prima; per una voce `oggi` è quanto possono ancora fare oggi i suoi
docenti fino ad h (per una coppia, chi può meno) e `al_giorno = min(3, max(0, h + 1 − ingresso))` (per una coppia,
l'ingresso di chi entra più tardi). È il controllo che si accorge dei docenti che entrano tardi.

**E, la classe da sola.** Si guarda la classe come se le altre non esistessero (i suoi docenti sempre liberi) e si
prova a riempire le celle che le restano con le ore che restano alle sue voci, con le sole regole che riguardano
lei: ogni docente dal suo ingresso, al massimo 3 ore al giorno e di seguito; lezione nelle prime ore (e, dopo, solo
se le ore che restano sono più di `dopo × obbligatorie`); ora buca solo dopo le obbligatorie e solo se le ore che
restano stanno nei giorni dopo (`≤ dopo × ore`), e dopo la buca si passa al giorno dopo; alla fine non deve avanzare
nessuna ora. Si parte dalla prima ora libera della classe (o dal giorno dopo, se oggi ha finito), con le ore che i
docenti hanno già fatto oggi lì e sapendo chi c'era nell'ora prima. È una piccola ricerca in profondità (le voci
nell'ordine, poi l'ora buca) con un tetto di **2000** celle provate: oltre, «non si sa» e il controllo passa.

La risposta dipende solo dallo stato della classe, che si ripete moltissime volte: le risposte si ricordano in una
tabella di **2^18** posti. Lo stato (la classe, l'ora da cui riparte, il resto di ogni voce, e per ogni docente
`ore fatte oggi × 2 + (c'era nell'ora prima)`) si riassume in due impronte di 64 bit calcolate in due modi diversi;
il posto è dato dagli ultimi 18 bit della prima, e una risposta vale solo se coincidono tutte e due. (Impronta a:
parte da 1469598103934665603; per ogni numero x, `a = (a xor x) × 1099511628211`. Impronta b: parte da
0x9E3779B97F4A7C15; `b = (b + x + 1) × 0xD6E8FEB86659FD93; b ^= b >> 32`.)

## 8. La verifica dell'orario trovato

Quando la ricerca trova un orario, lo ricontrolla una parte del programma che **non usa niente della ricerca**:
legge solo la scuola e le celle, e controlla da capo le otto regole del capitolo 3. Per ogni regola dice se è
rispettata e, se no, il primo punto in cui non lo è (per esempio «Ven 10: Zelda è in C01 e in C03»). Nello stato
finisce sotto `verifica`; il sito la mostra in un riquadro. Un orario che non passa la verifica è un errore del
solver, e nei test notturni è un allarme.

## 9. Riprendere una ricerca, e partire da una scelta fatta a mano

**Riprendere** (`--continua`). Una ricerca fermata dal limite di nodi si può mandare avanti fino a un limite più
alto, dal punto esatto in cui era. Si legge la strada dell'ultimo nodo visitato dai file dei passi; la si ripercorre
senza contare niente: a ogni profondità si ricalcolano le scelte del nodo (sono le stesse di allora), si ritrova la
scelta fatta, si scende; l'ultimo nodo della strada era stato visitato ma non le sue scelte, e si provano tutte; al
ritorno ogni nodo della strada continua con le scelte che vengono dopo quella già fatta. I conti (nodi, ritorni,
profondità massima, secondi) si rimettono com'erano dal file `.fine`. **Il risultato deve essere identico**, byte
per byte, a quello di una ricerca sola con il limite nuovo: passi, istantanee, profilo e stato (tranne i secondi).

**Partire da una scelta fatta a mano** (`--da FILE`). La ricerca parte da una strada data: ogni sua scelta deve
essere una di quelle che la ricerca avrebbe avuto in quel punto (se no il comando rifiuta). I nodi lungo la strada
si contano e si scrivono nei passi come nodi senza motivo; l'ultimo, quello della scelta fatta a mano, si visita
normalmente e da lì si cerca. Quando sotto la strada non c'è niente, si continua con le scelte che la ricerca
avrebbe provato **dopo** quelle della strada, risalendo; quelle che avrebbe provato **prima** non si provano. Per
questo, qui, «impossibile» vuol dire solo «nessun orario nella parte cercata».

**Le scelte in fondo a una strada** (`orario scelte`). Per sapere tra che cosa si può scegliere: si rifà la strada
un nodo alla volta e si chiedono le scelte dell'ultimo nodo, nell'ordine in cui la ricerca le proverebbe.

## 10. I comandi del solver

Il programma si chiama `orario`. Esce con 0, o con 2 se il comando non va, il caso non si legge, o quello che gli si
chiede non è possibile.

| Comando | Che cosa fa |
|---|---|
| `orario genera SEME CLASSI RITARDI` | Scrive su stdout un caso casuale. RITARDI: 1 con, 0 senza. |
| `orario risolvi CASO [--limite N] [--stato FILE] [--passi FILE] [--continua] [--da STRADA]` | Cerca un orario. |
| `orario nodo CASO` | Per il sito: tutto quello che c'è da dire di un nodo (arriva su stdin). |
| `orario scelte CASO` | Per il sito: le scelte in fondo a una strada (arriva su stdin). |
| `orario scuola CASO` | Scrive `{"celle": N, "scuola": {...}}`: il caso con docenti e voci numerati come li numera il programma. Per chi fa l'orario con un altro metodo (capitolo 22). |
| `orario verifica CASO` | Ricontrolla un orario fatto da altri: su stdin una voce per cella (−1: ora buca). Scrive `{"orario": [...], "verifica": {...}}`. |

`risolvi`:
- `--limite N`: dopo N nodi si ferma (0 o assente: mai).
- `--stato FILE`: scrive lo stato in FILE subito, poi ogni mezzo secondo (l'orologio si guarda ogni 4096 nodi) e
  alla fine; sempre in un file accanto (`FILE.tmp`) poi rinominato, così chi legge trova sempre un file intero.
  Senza `--stato` lo stato finale va su stdout.
- `--passi FILE`: scrive ogni nodo (capitolo 12) e, alla fine, `FILE.fine`.
- `--continua` (con `--passi`): riprende (capitolo 9). `--limite` è il totale dei nodi, non quelli in più. Rifiuta
  se la ricerca di quei passi non era stata fermata dal limite.
- `--da STRADA`: parte da una strada scelta a mano (capitolo 9). Il file: quante scelte, poi per ognuna cella e
  voce, tutto in numeri separati da spazi.

`nodo`: su stdin, in numeri: `tipo docente classe ora servono possibili` (il motivo, come nei passi; tipo 0 se dal
nodo si va avanti), `profondità`, e poi per ogni scelta `cella voce`, nell'ordine in cui sono state decise. Scrive:

```
{"giorno": G, "ora": O,                     l'ora a cui i conti sono fatti
 "segni": [[cella, segno], ...],            le celle di cui parla il motivo: 1 fatta, 2 può, 3 no, 4 serve
 "conto": {...},                            il conto del motivo, in dettaglio
 "docenti": [[docente, restano, libere, [[classe, restano, possibili], ...]], ...]}
```

L'ora dei conti è di solito l'ora che si sta decidendo; ma se l'ultima scelta ha chiuso un'ora e a non passare è
uno dei controlli «dopo la scelta», è l'ora di quella scelta (lo si capisce rifacendo quei controlli).

I **segni**, per controllo:
- A: in tutte le classi del docente, «fatta» dove insegna già; da adesso in poi «può» nelle ore dal suo ingresso,
  «no» prima.
- B: nella riga della classe, «fatta» dove il docente insegna già; oggi dalla prima ora libera «può» le ore che il
  conto di oggi conta e «no» le altre; nei giorni dopo «può» dall'ingresso, «no» prima.
- C ed E: nella riga della classe, le celle che restano: «può» (oggi «no» se la classe è chiusa).
- D: «serve» le celle della fascia da riempire (oggi dalla prima ora libera ad h, nei giorni dopo da 0 ad h).
- F: «no» le celle di quest'ora nelle classi del docente.
- G: «no» la cella.

Il **conto in dettaglio**, per controllo:
- A: `{"ingresso", "oggi", "dopo", "al_giorno"}` (possibili = oggi + dopo × al_giorno).
- B: lo stesso, più `"perche_oggi"`: `""`, `"finita"`, `"massimo"`, `"altrove"`, `"interrotto"`, `"troppe_classi"`.
- C: `{"oggi", "dopo", "al_giorno", "chiusa"}`.
- D: `{"oggi", "dopo", "al_giorno"}` per le celle, e `"voci": [[voce, resto, oggi, al_giorno, lezioni], ...]`.
- E: `{"celle", "voci": [[voce, resto], ...]}`.
- F: `{"classi": [[classe, perché, [[voce, perché, docente], ...]], ...]}`: un perché per tutta la classe
  (`"decisa"`, `"finite"`, `"senza_lezione"`) oppure `""` e un perché per ogni sua voce lì con ancora ore.
- G: `{"voci": [[voce, perché, docente], ...], "buca": "obbligatoria" | "restano_ore"}`.
I perché di un docente: `"non_entrato"`, `"altrove"`, `"troppe_classi"`, `"massimo"`, `"interrotto"`.

I **docenti**: per ognuno le ore che gli restano e le ore in cui è a scuola e libero da adesso (se in quest'ora è
già stato messo, dall'ora dopo), e per ogni classe dove ha ancora ore il conto B.

Le **buche** (`"buche": [[classe, restano, posti, come], ...]`): per ogni classe le ore che restano alla sua Buca,
i posti dove può ancora andare dalla prima ora che la classe deve decidere, e come sta in quest'ora: 0 la classe è
già decisa, 1 niente di particolare, 2 oggi ha già cominciato, 3 deve cominciare adesso.

`scelte`: su stdin una strada (quante scelte, poi cella e voce). Scrive `{"cella": C, "scelte": [voce | -1, ...]}`.

## 11. Lo stato della ricerca (JSON)

```
{"a_mano": 0 | 1,                       1: partita da una strada scelta a mano
 "in_corso": 0 | 1,
 "esito": "in corso" | "trovato" | "impossibile" | "limite di nodi",
 "nodi": N, "secondi": S, "profondita_massima": P, "celle": C,
 "ritorni": R,                          i ritorni indietro in tutto
 "ritorni_per_cella": [...],            per cella: quelli arrivati subito dopo una scelta in quella cella
 "ritorni_recenti_per_cella": [...],    gli stessi, solo dall'ultimo passo avanti
 "ritorni_per_motivo": [[tipo, docente, classe, quanti, recenti], ...],
                                        per controllo e per chi il motivo nomina (un docente, oppure -1 e una
                                        classe); solo quelli che ne hanno
 "scuola": {"giorni": [...], "ore": [...], "classi": [...], "obbligatorie": M,
            "ore_di_seguito": 3, "classi_al_giorno": 3,
            "docenti": [[nome, ingresso], ...],
            "voci": [[classe, docente, codocente | -1, ore], ...]},
 "motivo": {"tipo", "docente", "classe", "ora", "servono", "possibili"},
                                        solo con «impossibile» già al primo nodo
 "orario": [voce | -1, ...],            solo con «trovato»: una per cella
 "verifica": {"ok": 0 | 1, "controlli": [[regola, 0 | 1, dove], ...]}}   solo con «trovato»
```

I nomi dei tipi di motivo, con il loro numero e la lettera che il sito mostra:

| Numero | Nome | Lettera |
|---|---|---|
| 1 | `ore_docente` | A |
| 2 | `ore_docente_in_classe` | B |
| 3 | `ore_classe` | C |
| 4 | `prime_ore` | D |
| 5 | `docente_senza_posto` | F |
| 6 | `classe_senza_scelte` | G |
| 7 | `orario_della_classe` | E |

Chi è nominato: A, B e F un docente (B anche una classe; F e G anche l'ora); C, D, E e G una classe (D anche l'ora
h). Nei conti «per motivo» conta il docente se c'è, se no la classe.

## 12. I file dei passi

Con `--passi FILE` il solver scrive quattro file. Tutti i numeri sono nell'ordine dei byte del computer.

**FILE**: un record per nodo, nell'ordine in cui la ricerca li visita. Un record sono 9 interi a 16 bit (18 byte):
`profondità, cella, voce, tipo, docente, classe, ora, servono, possibili`. Cella e voce sono l'ultima scelta (al
primo nodo −1 e −2; voce −1 è l'ora buca); gli altri sei sono il motivo (tipo 0: si va avanti; −1 dove non
c'entra). Il nodo numero k (da 1) è il record k. L'orario a un nodo è la sua strada: un record con profondità d
mette la sua scelta al posto d e toglie quelle dopo.

**FILE.ist**, le istantanee: ogni **16.384** nodi, tutta la strada di quel nodo: la profondità, poi una coppia
(cella, voce) per ogni cella dell'orario (contano le prime «profondità»). Sono `1 + 2 × celle` interi a 16 bit.
Servono a ricostruire in fretta un nodo qualunque: si parte dall'ultima istantanea prima e si applicano al massimo
16.384 record. A ogni istantanea i file si svuotano su disco, così il sito può leggere mentre la ricerca va.

**FILE.prof**, il profilo: ogni **256** nodi una coppia di interi a 16 bit, la profondità più bassa e la più alta
di quei 256 nodi. Serve a disegnare tutta la ricerca senza rileggere ogni nodo.

**FILE.fine**, scritto alla fine: l'esito (int), i nodi e i ritorni (interi a 64 bit), la profondità massima (int),
i secondi (double), poi tabelle di interi a 64 bit: i ritorni per cella, i ritorni recenti per cella, i ritorni per
motivo e quelli recenti per motivo (ognuna di queste due lunga `8 × (docenti + classi)`: una riga per tipo, in ogni
riga prima i docenti e poi le classi). Serve a `--continua`; se le ultime tabelle mancano (un file più vecchio) si
riparte da zero per quelle.

Spazio: **19 MB per milione di nodi** (18 del record, il resto istantanee e profilo).

## 13. Il sito: com'è fatto

- Python con Flask; server waitress (4 fili) se c'è, altrimenti quello di Flask. Niente login, niente database.
- Tutti i dati stanno in file dentro la **cartella di lavoro**: `sito/lavoro/`, o quella della variabile
  `SITO_LAVORO`. I file JSON si scrivono a parte e poi si rinominano.
- Il sito **compila il solver da solo** (`gcc -O2 -std=gnu11 -w` su tutti i `.c` delle cartelle del programma in C,
  capitolo 21): l'eseguibile sta in
  `lavoro/bin/orario-<impronta>`, dove l'impronta è calcolata su tutti i sorgenti; se un sorgente cambia, cambia il
  nome e si ricompila.
- Gira **una cosa alla volta**: una prova in tempo reale oppure un giro di test notturni, mai insieme.
- Aspetto: fondo rosa cipria, testo prugna, un accento rosa lampone e il lilla; titoli con le grazie, il nome del
  sito in corsivo; tasti a pillola. In cima a ogni pagina il nome («Orario scolastico») e due schede: «Prove in
  tempo reale» e «Test notturni». I messaggi (errori, conferme) compaiono sotto, in un riquadro.
- I numeri sono scritti all'italiana (punti per le migliaia).

La cartella di lavoro:

```
bin/orario-<impronta>           il solver compilato
prove/<numero>/info.json        il caso (seme, classi, ritardi), il limite, quando è partita, se è stata
                                fermata, e «a_mano» (prova, passo, cella, voce) se parte da una scelta a mano
prove/<numero>/caso.txt         il caso, come lo legge il solver
prove/<numero>/stato.json       lo stato, scritto dal solver
prove/<numero>/passi.bin        i passi, con .ist, .prof e .fine
prove/<numero>/strada.txt       solo per una prova «a mano»: la strada da cui parte
notti/casi.json                 i casi dei test notturni
notti/impostazioni.json         limite di nodi, giro della notte (acceso, ora), ultimo numero dato a un caso
notti/giri/<numero>.json        un giro e i suoi risultati
notti/visti.json                per ogni caso, il risultato di cui si è presa visione
```

## 14. La pagina «Prove in tempo reale» (`/`)

- Un modulo: **Seme** (da 0), **Classi** (da 1 a 120), **Ritardi** (senza / con), **Metodo** («esatto: solo il
  risultato», che è la scelta di partenza, oppure «la ricerca, passo per passo»), **Fermati dopo** N milioni di nodi
  (da 0,01 a **1000**; il campo c'è solo con la ricerca), e il tasto **Avvia** (spento se una prova sta girando). I
  valori iniziali sono quelli dell'ultima prova.
- Una prova con il **metodo esatto** (capitolo 22) non registra passi e non ha limite di nodi: ha al massimo 300
  secondi. La sua pagina mostra l'esito, il tempo, l'orario intero con la verifica, oppure che un orario non esiste;
  non ha lo slider, i ritorni, il profilo, né «Continua» e «Cambia questa scelta». Nell'elenco è segnata «esatto».
- Una nota: che cosa decide il seme; che ogni nodo si registra (19 MB per milione); una prova alla volta; il
  collegamento alla prova che sta girando.
- **Le prove fatte**: una tabella, dalla più recente: casella per sceglierla, numero (collegamento), quando è
  partita, seme, classi, ritardi, esito (etichetta colorata; «non valido» se l'orario non passa la verifica; «a
  mano, dalla N» se parte da una scelta a mano), nodi, celle decise (massimo raggiunto / totale), tempo.
- **Elimina le prove scelte**: le caselle, una in cima per tutte; il tasto dice quante sono e chiede conferma. La
  prova che sta girando non si può scegliere.

Avviare una prova: si genera il caso, si controlla che sul disco ci sia spazio per tutti i nodi del limite (19 byte
l'uno, lasciando liberi 500 MB; se no un messaggio dice quanti GB servono), si lancia il solver con stato e passi,
e si passa alla pagina della prova. Il numero di una prova è il più alto esistente più uno.

Gli esiti che una prova può mostrare: quelli del solver («in corso», «trovato», «impossibile», «limite di nodi»),
più «fermata» (fermata a mano) e «interrotta» (il sito si è riavviato mentre girava).

## 15. La pagina di una prova (`/prova/N`)

Dall'alto in basso.

**Intestazione.** «← Tutte le prove»; il titolo «Prova N: seme S, C classi, con/senza ritardi». Se la prova parte
da una scelta fatta a mano, un avviso giallo: da quale prova e passo (collegamento), fino a quale cella, con quale
voce, e che «impossibile» lì vuol dire solo «nella parte cercata». Poi: l'esito (etichetta), «N nodi, R ritorni
indietro in tutto, S s», e il limite.

**I tasti.**
- **Ferma** (solo mentre gira).
- **Continua per altri 100 nodi / 1.000 nodi** (solo se l'esito è «limite di nodi» e non gira): riprende la ricerca
  (capitolo 9); la pagina si riapre sul passo a cui si era fermata.
- **Rifai**: una prova nuova sullo stesso caso con lo stesso limite.
- **Aggiungi ai notturni**: il caso entra tra i test notturni.

**Messaggi.** Se un orario non c'è già al primo nodo: «Un orario non può esserci, e si vede prima ancora di
cercare», con il controllo. Se l'orario è stato trovato: il riquadro della verifica, con le otto regole e ✓ o ✗.

**La ricerca, passo per passo.**
- Una riga di tasti con lo slider in mezzo: **⏮ Ritorno**, **−100**, **−10**, **◀ Prec.**, lo slider (un passo per
  nodo), **Succ. ▶**, **+10**, **+100**, **Ritorno ⏭**. Le frecce sinistra e destra della tastiera fanno un passo.
  I tasti «Ritorno» saltano al ritorno indietro prima o dopo.
- La riga dei filtri. A sinistra: senza una cella scelta, in quale cella sono soprattutto i ritorni dall'ultimo
  passo avanti, e l'invito a fare clic su una cella; con una cella scelta, «Solo i ritorni di Ven 10, C02: N in
  tutta la ricerca, di cui M dall'ultimo passo avanti», il tasto **Tutti i ritorni** e, se a quel passo la cella è
  decisa e la prova non gira, **Cambia questa scelta…**. A destra: «Solo i ritorni che tornano indietro di almeno
  [N] celle».
- **Passo K di T · chi in quale cella · quante celle decise su quante.**
- «Dal passo J: N celle cambiate» (il confronto è con il passo guardato subito prima).
- Se il passo è un ritorno, una frase su fondo rosso: «**Controllo X · nome** non passa.», la spiegazione in parole
  con i due numeri, e «Dopo, la ricerca torna indietro di N celle» (o «cambia solo questa scelta»). Accanto il tasto
  **Mostra nell'orario** / **Togli i segni**, che resta acceso da un passo all'altro.
- **Il conto**: «Il conto, fatto a Ven alle 09.» e i pezzi da cui vengono i numeri (capitolo 10), in parole; le
  liste (voci, classi) come etichette una accanto all'altra.
- La legenda dei segni, quando sono accesi.
- **L'orario**: una riga per classe, una colonna per ogni ora della settimana, raggruppate per giorno; la colonna
  dei nomi delle classi resta ferma mentre si scorre di lato; tutte le celle larghe uguali. In una cella il docente
  e, sotto e più chiaro, il codocente; «–» per l'ora buca. Colori:
  - fondo rosa: l'ultima scelta;
  - bordo lilla: cella cambiata rispetto al passo guardato prima; fondo lilla e testo sbarrato: cella tolta;
  - con i segni accesi: verde «fatta», giallo «può», rosso «no», lilla «serve» (bordo e fondo chiaro);
  - bordo rosa spesso: la cella scelta con un clic;
  - il nome della classe di cui parla il controllo è segnato in rosso.
  Il riquadro scorre da solo fino all'ultima scelta.
- Le sezioni che si aprono e si chiudono:
  - **Tutti i ritorni, uno per riga** (chiusa; si riempie quando si apre): una tabella con una riga per ritorno,
    nell'ordine in cui sono avvenuti: passo, ultima scelta (chi in quale cella), controllo, per chi, servono,
    possibili, celle indietro. Cento righe per volta, con «Altri 100»; «Dal primo» e «Dal passo che sto guardando»
    scelgono da dove cominciare. Segue la cella scelta e il filtro «almeno N celle». Un clic su una riga la apre:
    sotto compaiono la frase del controllo, i pezzi del suo conto (chiesti in quel momento) e «Vai a questo passo»;
    un altro clic la richiude.
  - **I ritorni, per controllo e per chi** (chiusa; nel titolo c'è già il primo): controllo, docente o classe,
    ritorni, ritorni dall'ultimo passo avanti; dal più grande, al massimo 40 righe.
  - **I controlli, docente per docente** (chiusa): a quel passo, per ogni docente, A («restano R, a scuola e libero
    per L») e B per ogni classe dove ha ancora ore («C02 4 / 3»), con ✓ o ✗; la riga del docente del controllo che
    non passa è segnata. Sotto, le ore buche classe per classe: «restano / posti dove possono andare», e se la
    Buca oggi ha già cominciato o deve cominciare adesso.
  - **I numeri dei ritorni: che cosa vogliono dire** (aperta): una voce per ogni numero della pagina.
- **Il profilo della ricerca**: un disegno largo quanto la pagina, alto 120 punti: da sinistra a destra tutti i
  passi (al massimo 400 tratti), in altezza quante celle sono decise (la più bassa e la più alta di ogni tratto);
  un filo scuro per il passo che si guarda; un clic porta a quel punto.

**Un clic su una cella** la sceglie (un altro la lascia): i tasti «Ritorno» saltano solo ai ritorni arrivati subito
dopo una scelta in quella cella. Il filtro «almeno N celle» vale insieme.

**Cambia questa scelta…** mostra, come tasti, le scelte che la ricerca aveva per quella cella nel punto in cui l'ha
decisa lungo la strada del passo che si guarda, nell'ordine in cui le prova (quella che c'è è spenta). Un clic su
una fa partire una **prova nuova**: l'orario di quel passo fino a quella cella esclusa, la scelta nuova, e da lì la
ricerca, con lo stesso limite di nodi.

**Il caso** (in fondo, due sezioni chiuse): le classi con le loro ore e le loro lezioni; i docenti con le ore,
l'ingresso (se tardi) e le classi.

**Mentre la prova gira** la pagina chiede lo stato ogni secondo e il profilo ogni quattro; lo slider si allunga e,
se è in fondo, segue l'ultimo passo.

## 16. La pagina «Test notturni» (`/notturni`)

- **Il giro.** «Avvia il giro» (spento se non ci sono casi, o se gira un giro o una prova) e «Ferma»; una riga che
  dice del giro in corso (casi fatti su quanti) o dell'ultimo (numero, quando, come è finito, limite, se è partito
  da solo). Le impostazioni: «Ogni caso si ferma dopo N milioni di nodi» (da 0,01 a **100**; in partenza 2),
  «ogni notte il giro parte da solo alle HH:MM» (in partenza spento, 22:00), e «Salva».
- **I cambiamenti.** Una tabella per tipo, solo quelli che ci sono: Allarmi; Non risolto → Risolto; Risolto → Non
  risolto; Stesso esito, meno nodi; Stesso esito, più nodi; Altri cambiamenti. In ogni riga: seme, classi, ritardi,
  l'esito e i nodi di prima (con il giro), quelli di adesso, la differenza di nodi in percento. In cima a ogni
  tabella il tasto **Visto** (o «Visti tutti e N»).
- **I casi.** Un modulo per aggiungerne: semi da… a…, classi da… a…, ritardi (senza e con / senza / con); al massimo
  **2000** per volta; quelli che ci sono già non si ripetono. «Togli i casi scelti», con le caselle. La tabella:
  seme, classi, ritardi, esito, nodi, celle decise, tempo, e «rispetto al giro prima» (uguale, ±N% di nodi, o
  l'esito di prima), e in alto il conto dei casi per esito.

**Il giudice.** Dopo la ricerca, ogni caso è rifatto con il metodo esatto (capitolo 22; al massimo 120 secondi),
e la sua risposta sta in una colonna accanto, «Un orario c'è?»: «sì», «no, è impossibile», «non si sa». È in evidenza
quando la ricerca si è fermata al limite ma un orario c'è, e in rosso quando contraddice la ricerca (trovato da una
parte e impossibile dall'altra): quella contraddizione è un allarme tra i cambiamenti. Non cambia il risultato della
ricerca né i suoi confronti. Se scipy non è installato la colonna resta vuota.

Come funziona un giro: tutti i casi, uno dopo l'altro, ognuno generato e risolto in una cartella provvisoria con il
limite scelto e **senza registrare i passi**; di ogni caso si tengono esito, nodi, secondi, profondità massima,
celle e se l'orario passa la verifica. Il file del giro si riscrive a ogni caso. Un giro finisce «finito»,
«fermato», «interrotto» (il sito si è riavviato) o con un errore. Il giro della notte: un orologio guarda ogni 20
secondi se è l'ora scelta e se oggi non è già partito.

**I cambiamenti: la regola.** Il risultato di un caso è (esito, nodi, limite). L'ultimo risultato si confronta con
il **riferimento**: quello di cui si è presa visione, se era con lo stesso limite; altrimenti il primo che il caso
ha avuto con quel limite. Se è diverso il caso sta tra i cambiamenti finché non si preme «Visto». Il tipo:
- **allarme**: l'orario non passa la verifica, oppure il caso non è riuscito o non si è generato, oppure uno dei
  due esiti è «trovato» e l'altro «impossibile» (uno dei due è sbagliato). Un orario che non passa la verifica si
  mostra anche se è il primo risultato;
- esito diverso: «risolto» se adesso è trovato, «non risolto» se prima era trovato, altrimenti «altro»;
- esito uguale: «meno nodi» o «più nodi».

## 17. Tutti gli indirizzi del sito

| Indirizzo | Metodo | Che cosa fa |
|---|---|---|
| `/` | GET | La pagina delle prove. |
| `/avvia` | POST | Avvia una prova (seme, classi, ritardi, milioni, metodo: `esatto` o `ricerca`). |
| `/prova/N` | GET | La pagina di una prova. `?passo=K` la apre su quel passo. |
| `/prova/N/stato.json` | GET | Lo stato, più `gira` e `passi` (quanti nodi si possono già rivedere). |
| `/prova/N/passo/K` | GET | Il nodo K: `k, profondita, cella, voce, motivo, orario, indietro`. |
| `/prova/N/nodo/K` | GET | Quello che il solver dice del nodo K: segni (con i nomi `fatta, puo, no, serve`), conto, docenti. |
| `/prova/N/ritorno/K/prima` o `/dopo` | GET | Il ritorno indietro prima o dopo il nodo K: `{"k": …}`. `?cella=C` solo quelli della cella; `?almeno=N` solo quelli che tornano indietro di almeno N celle. |
| `/prova/N/ritorni` | GET | I ritorni uno per riga, 100 per volta: `{"ritorni": [{k, profondita, cella, voce, motivo, indietro}, …], "prossimo": K o null}`. `?dopo=K` da dopo quel nodo (0: dall'inizio); `?cella=` e `?almeno=` come sopra. |
| `/prova/N/scelte/K/C` | GET | Le scelte per la cella C lungo la strada del nodo K: `{"scelte": […], "adesso": voce}`. |
| `/prova/N/cambia` | POST | Una prova nuova da una scelta fatta a mano (passo, cella, voce). |
| `/prova/N/continua` | POST | Manda avanti di 100 o 1000 nodi (nodi). |
| `/prova/N/profilo.json` | GET | `{"nodi": …, "fasce": [[bassa, alta], …]}`, al massimo 400 fasce. |
| `/ferma` | POST | Ferma la prova che gira. |
| `/elimina` | POST | Elimina le prove (numeri, separati da virgole). |
| `/notturni` | GET | La pagina dei test notturni. |
| `/notturni/stato.json` | GET | La situazione: impostazioni, giro, casi, cambiamenti. |
| `/notturni/aggiungi` | POST | Aggiunge casi (semi_da, semi_a, classi_da, classi_a, ritardi). |
| `/notturni/da-prova` | POST | Aggiunge il caso di una prova (prova). |
| `/notturni/togli` | POST | Toglie casi (numeri). |
| `/notturni/impostazioni` | POST | Limite e giro della notte (milioni, notte, ora). |
| `/notturni/avvia`, `/notturni/ferma` | POST | Avvia o ferma un giro. |
| `/notturni/visto` | POST | Conferma la presa visione (numeri). |

«Di quante celle torna indietro» un ritorno al nodo K: `profondità di K − profondità di K + 1 + 1`: le scelte che
la ricerca disfa prima di provarne una nuova. 1 è il caso normale. Dell'ultimo nodo non si sa.

Il ritorno «di una cella» è un nodo con un motivo il cui campo cella (l'ultima scelta) è quella cella.

## 18. I limiti e i numeri fissi, in un posto solo

| Che cosa | Valore |
|---|---|
| Giorni, ore al giorno, classi, docenti, voci (massimi) | 7, 10, 120, 250, 4000 |
| Ore di un docente in una classe in un giorno | al massimo 3, di seguito |
| Classi di un docente in un giorno | al massimo 3 |
| Casi casuali | 6 giorni × 6 ore, 5 obbligatorie; 30–36 ore per classe; lezioni da 1 a 6 ore; 5–24 ore per docente; ingresso al più alla quarta ora |
| Tentativi del generatore | 20.000 |
| Classe da sola: celle provate per risposta | 2000 |
| Classe da sola: risposte ricordate | 2^18 |
| Limite di nodi di una prova in tempo reale | da 0,01 a **1000 milioni** |
| Limite di nodi dei test notturni | da 0,01 a 100 milioni (in partenza 2) |
| Tempo del metodo esatto | 300 secondi per una prova; 120 secondi come giudice nei test notturni |
| Continuare una prova | di 100 o di 1000 nodi per volta |
| Spazio su disco | 19 MB per milione di nodi; 500 MB lasciati liberi |
| Istantanee dei passi | ogni 16.384 nodi |
| Blocchi del profilo | 256 nodi; al massimo 400 fasce nel disegno |
| Stato su file | ogni mezzo secondo |
| Casi notturni aggiunti per volta | al massimo 2000 |
| Aggiornamento delle pagine | prova: stato ogni 1 s, profilo ogni 4 s; notturni: ogni 2 s |
| Righe del riepilogo dei ritorni | al massimo 40 |
| Righe per volta nella tabella di tutti i ritorni | 100 |

I numeri dei passi sono a 16 bit: la profondità e il numero di una cella arrivano al più a 7 × 10 × 120 = 8400, il
numero di una voce a 3999.

## 19. La prova automatica e le verifiche da rifare dopo una modifica

`python3 prove/prova.py` compila il solver e risolve **52 casi** con il limite di 200.000 nodi: semi da 0 a 5 ×
classi 3, 5, 8, 12 × senza e con ritardi, più (10, 3, con), (1, 14, con), (2, 20, senza), (3, 30, con). Di ognuno
confronta esito, nodi e (se l'orario c'è) la verifica con `prove/attesi.json`. Con `--aggiorna` riscrive il file:
solo dopo una modifica voluta alla ricerca.

Dopo ogni modifica al solver:
1. `prove/prova.py` deve dire «tutto come atteso» (se la modifica non doveva cambiare la ricerca).
2. Continuare deve dare gli stessi file di una ricerca sola: su qualche caso, spezzare la ricerca in più pezzi con
   `--continua` e confrontare passi, `.ist`, `.prof` e stato (senza i secondi) con quelli di una ricerca unica.
3. Per ogni ritorno, la somma dei pezzi del conto in dettaglio deve dare i due numeri del motivo; la somma dei
   ritorni per cella e quella per motivo devono dare il totale.
4. Se la modifica cambia la ricerca: misurare, sugli stessi casi di prima, quanti orari si trovano e con quanti
   nodi; ogni orario trovato deve passare la verifica; nessun caso con un orario deve diventare «impossibile».

Misure di riferimento (limite 2 milioni di nodi; «prima» è il programma del ramo `stabile/2026-11-18`):

| Casi | Orari trovati, prima | Riscritto (20 nov) | Con la buca in coda (26 nov) |
|---|---|---|---|
| semi 0–19, classi 3–14, senza e con ritardi (480) | 306 | 323 | 327 |
| semi 20–39, classi 3–14 (480) | 310 | 326 | 336 |
| semi 0–5, classi 20–25 (72) | 33 | 44 | 44 |

Mettere la buca in coda ha fatto guadagnare 29 orari e perderne 15 rispetto a subito prima (nei tre gruppi: 9 e 5,
16 e 6, 4 e 4); i nodi in tutto sono scesi da 103 a 75 milioni. I casi impossibili sono gli stessi e nessun orario
trovato fallisce la verifica. Dei 9 casi che solo il programma di prima risolveva, adesso se ne trovano 8 (resta
fuori seme 1, 14 classi, con ritardi), e seme 10, 3 classi, con ritardi si trova in 278 nodi.

Esperimenti misurati e non entrati: le ripartenze a tentativi (330, 341, 48 sulla versione del 20 novembre) e i
«giri» (a ogni giornata finita una ricerca a parte che tiene i giorni dal secondo in poi e butta il primo: 332, 341,
49 sulla versione del 20 novembre).

## 20. Installazione e avvio

Servono Python 3, `gcc`, e i pacchetti di `sito/requirements.txt` (flask, waitress, e scipy per il metodo esatto:
senza scipy il sito funziona lo stesso, ma solo con la ricerca).

```
pip install -r sito/requirements.txt
python3 -m sito avvia --porta 8000            # solo su questo computer
python3 -m sito avvia --host 0.0.0.0 --porta 8000   # visibile da tutta la rete locale (Raspberry)
```

A mano, senza il sito: `make`, poi `./orario genera 10 3 1 > caso.txt` e `./orario risolvi caso.txt --limite
2000000`.

Sul Raspberry (`raspberry@192.168.1.5`, servizio `orario-sito`, repository in `~/orario`, ambiente Python in
`~/orario-venv`), per aggiornare: `cd ~/orario && git pull && sudo systemctl restart orario-sito`, poi Ctrl+F5 nel
browser.

## 21. I file del repository

```
scuola/scuola.h, scuola.c        i dati di una scuola, i limiti di grandezza, la lettura di un caso
generatore/genera.h, genera.c    il generatore dei casi casuali
generatore/nomi.h                i nomi dei docenti dei casi casuali
solver/ricerca.h, ricerca.c      la ricerca: lo stato, le scelte, il nodo, riprendere, partire da una strada
solver/limiti.h, limiti.c        i controlli A, B, C, D, e i conti piu' piccoli che usano
solver/classe_sola.c             il controllo E
verifica/verifica.h, verifica.c  la verifica dell'orario trovato
passi/passi.h, passi.c           i passi su file
spiegazioni/spiega.h, spiega.c   per il sito: i segni, il conto in dettaglio, i conti docente per docente
programma/main.c                 i comandi
programma/stato.h, stato.c       lo stato in JSON
programma/fine.h, fine.c         il file .fine, per continuare una ricerca
esatto/modello.py                il metodo esatto: il caso scritto come conti, e la sua soluzione
esatto/__main__.py               il comando del metodo esatto
sito/__main__.py                 l'avvio
sito/app.py                      le pagine e gli indirizzi
sito/prove.py                    le prove in tempo reale
sito/passi.py                    leggere i passi: un nodo, la strada, i ritorni, il profilo
sito/notturni.py                 i test notturni
sito/solver.py                   compilare e lanciare il solver
sito/archivio.py                 la cartella di lavoro e i file JSON
sito/templates/                  base, indice, prova, notturni (HTML, con il loro JavaScript)
sito/static/stile.css            lo stile
prove/prova.py, attesi.json      la prova automatica
LEGGIMI.md                       la presentazione breve
DESCRIZIONE.md                   questo documento
CLAUDE.md                        le regole di lavoro su questo repository
bf_search/                       SQL di Alb, a parte
```

## 22. Il metodo esatto

Un secondo modo di fare l'orario, accanto alla ricerca (dal 26 novembre 2026). Non riempie le celle una alla volta
tornando indietro: scrive tutto il caso come un sistema di conti con incognite che valgono 0 o 1, e lo dà a un
risolutore generico di programmi lineari interi (HiGHS, dentro la libreria scipy di Python:
`scipy.optimize.milp`). Dà l'orario, oppure dimostra che non esiste.

**Le incognite**, tutte 0 o 1:
- `x[v, g, h]`: la voce v è nell'ora h del giorno g (nella sua classe);
- `b[k, g, h]`: la classe k in quell'ora ha l'ora buca;
- `c[t, k, g, h]`: il docente t comincia a insegnare nella classe k in quell'ora (nell'ora prima non c'era).

**Le regole** (le otto del capitolo 3), ognuna come una somma che deve stare tra due numeri:

| Regola | Come conto |
|---|---|
| Ogni cella ha una cosa sola | per ogni classe e ora: la somma delle x delle voci della classe, più b, fa 1 |
| Ogni voce fa le sue ore | la somma delle sue x nella settimana fa le sue ore |
| Un docente in una classe per volta | per ogni docente e ora: la somma delle x delle sue voci è al massimo 1 |
| Non prima dell'ingresso | quelle x hanno massimo 0 (per una coppia, l'ingresso di chi entra dopo) |
| Al massimo 3 ore al giorno in una classe | per ogni docente, classe e giorno: la somma delle x è al massimo 3 |
| Di seguito | `c[h] ≥ (somma delle x in h) − (somma delle x in h − 1)`, e in un giorno la somma delle c è al massimo 1 |
| Al massimo 3 classi al giorno | per ogni docente e giorno: la somma delle c in tutte le classi è al massimo 3 (in ogni classe comincia al più una volta, quindi gli inizi sono le classi) |
| Prime ore, e dopo la buca solo buche | b ha massimo 0 nelle ore obbligatorie; `b[h + 1] ≥ b[h]` |

Non c'è niente da rendere massimo o minimo: si cerca una risposta qualunque che rispetti tutto. Dalla risposta
all'orario: in ogni cella la voce la cui x vale 1, altrimenti l'ora buca.

**Il comando.** `python3 -m esatto CASO --programma ORARIO [--stato FILE] [--tempo SECONDI]`. Chiede la scuola al
programma in C (`orario scuola`), risolve, e fa ricontrollare l'orario trovato allo stesso programma (`orario
verifica`): così usa gli stessi numeri per docenti, voci e celle, e l'orario è verificato da un programma che con
questo metodo non ha niente a che fare. Scrive lo stato nel formato del capitolo 11, con `"metodo": "esatto"`, `nodi`
0 e senza i conti dei ritorni; gli esiti sono «trovato», «impossibile» (dimostrato) e «limite di tempo».

**Quanto è affidabile.** Confrontato con la ricerca (2 milioni di nodi) sui tre gruppi di casi del capitolo 19,
1.032 casi: nessuna contraddizione. Tutti i 707 casi in cui la ricerca trova un orario li trova anche lui; tutti i
288 che la ricerca dichiara impossibili li dichiara impossibili anche lui; e decide tutti i 37 in cui la ricerca si
ferma al limite: 33 hanno un orario, 4 sono impossibili. Ogni suo orario passa la verifica del programma in C. Tempo
per caso: mediana 0,2 secondi (1 secondo con 20–25 classi), massimo 19.

**Che cosa non dà.** Non ha passi, nodi né ritorni: non c'è niente da rivedere, e non spiega perché un caso è
impossibile. Per questo la ricerca resta, per guardare e capire; il metodo esatto dà il risultato e fa da giudice.

