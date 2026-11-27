# Orario scolastico: descrizione completa

Questo documento descrive tutto il programma e il sito con il dettaglio che serve per rifarli da zero senza avere il
codice: che cosa fanno, con quali dati, con quali regole e numeri, che cosa mostrano, quali comandi e quali indirizzi
hanno. Dove un numero o un ordine conta (perché cambiandolo cambiano i risultati) è scritto.

Stato descritto: `main` al 26 novembre 2026. È tutto in Python.

Indice

1. A che cosa serve, e le parole
2. La scuola e le regole dell'orario
3. Il file di un caso
4. Il generatore dei casi casuali
5. Il metodo: l'orario come sistema di conti
6. La verifica dell'orario trovato
7. I comandi
8. Lo stato (JSON)
9. Il sito: com'è fatto
10. La pagina «Prove»
11. La pagina di una prova
12. La pagina «Test notturni»
13. Tutti gli indirizzi del sito
14. I limiti e i numeri fissi, in un posto solo
15. La prova automatica e le verifiche da rifare dopo una modifica
16. Installazione e avvio
17. I file del repository
18. Che cosa c'era prima, e dov'è
19. Le scuole scritte a mano: le pagine «Scuole»
20. I due metodi insieme: HiGHS e CP-SAT
21. I test notturni su GitHub

---

## 1. A che cosa serve, e le parole

Dato l'elenco delle lezioni di una scuola (quale docente, o quale coppia di docenti, fa quante ore in quale classe),
il **programma** (`orario/`) trova un orario settimanale che rispetta le regole del capitolo 2, oppure dimostra che
non esiste. Lo cerca con due metodi insieme (capitoli 5 e 20).

Il **sito** (`sito/`) non calcola niente: lancia il programma e ne mostra i risultati. Ha due parti: le **prove** (un
caso e il suo orario) e i **test notturni** (un elenco di casi rifatti tutti insieme, per vedere che cosa cambia
quando cambia il programma).

I casi sono **casuali ma riproducibili**: un caso è identificato da tre cose (un seme, il numero di classi, con o
senza ritardi) e le stesse tre danno sempre la stessa scuola.

| Parola | Che cosa vuol dire |
|---|---|
| cella | Un posto dell'orario: una classe, in un'ora, in un giorno. |
| voce | Una lezione di una classe: una materia, fatta da un docente da solo oppure da una coppia (docente e codocente insieme), con le ore che devono fare in una settimana. |
| materia | Quello che si insegna in una lezione. Un docente può insegnare più materie: nella stessa classe sono voci separate. Ogni materia chiede un tipo di aula. |
| aula | Dove si fa una lezione. Ogni aula ha un tipo (normale, laboratorio, palestra…: un nome qualunque); tutte le aule della scuola sono contate, anche quelle normali. |
| ora buca | Una cella senza lezione. |
| ingresso | L'ora (indice, da 0) da cui un docente è a scuola. Prima non può insegnare. Vale per tutti i giorni. |
| uscita | L'ora (indice) da cui un docente non è più a scuola: insegna solo nelle ore prima. Vale per tutti i giorni. Chi resta fino alla fine ha come uscita il numero delle ore. |
| giorno libero | Un giorno in cui il docente non insegna. Ogni docente ne ha uno: un giorno preciso, oppure, quando non è specificato, «uno qualunque» (quale, lo sceglie il programma). |
| ore obbligatorie | Le prime ore di ogni giorno, che in ogni classe hanno sempre lezione. |

Numerazione. Giorni, ore, classi, docenti e voci sono numerati da 0. La cella di (giorno, ora, classe) ha il numero
`(giorno × ore + ora) × classi + classe`: le celle sono in ordine di tempo, e dentro la stessa ora in ordine di
classe. Un orario è un elenco con un numero per cella: il numero di una voce, oppure −1 per l'ora buca. Le aule
dell'orario sono un secondo elenco, anche lui con un numero per cella: l'aula della lezione, oppure −1.

## 2. La scuola e le regole dell'orario

Una scuola ha: i giorni e le ore (con i loro nomi), le classi, i docenti (ognuno con il suo ingresso), le voci, e il
numero di ore obbligatorie.

Limiti di grandezza: al massimo 7 giorni, 10 ore al giorno, 120 classi, 250 docenti, 4000 voci; nomi lunghi al
massimo 47 caratteri.

Le voci sono tenute in ordine di classe (poi di docente, poi di codocente); in una coppia il docente con l'indice
più basso viene per primo, così la stessa coppia nella stessa classe è sempre la stessa voce.

Le regole di un orario valido:

1. **Le ore.** Ogni voce fa esattamente le sue ore, nella sua classe.
2. **Una classe per volta.** Un docente è in una sola classe per volta.
3. **A scuola.** Un docente insegna solo quando è a scuola: non prima della sua ora di ingresso, non dalla sua ora
   di uscita in poi, non nel suo giorno libero. Ogni docente ha un giorno libero (dal 27 novembre 2026, richiesta di Alb): chi
   non ne ha uno preciso ha almeno un giorno senza lezioni, uno qualunque.
4. **In una classe.** In una classe, in un giorno, un docente fa al massimo **3** ore (`ORE_DI_SEGUITO`), e le fa
   di seguito, senza buchi. Si contano insieme tutte le sue voci lì: da solo, in coppia, di tutte le sue materie.
5. **Le classi.** In un giorno un docente insegna in al massimo **3** classi (`CLASSI_AL_GIORNO`).
6. **La giornata del docente.** In un giorno un docente insegna al massimo **5** ore (`LEZIONI_AL_GIORNO`), in tutte
   le sue classi insieme. Le sue ore buche (ore senza lezione tra la sua prima e la sua ultima lezione del giorno:
   quelle prima della prima e dopo l'ultima non contano) dipendono da quante ore insegna quel giorno: fino a 3 ore,
   nessuna; con 4 ore, nessuna o una; con 5 ore, esattamente una: la pausa è obbligatoria (`ORE_PER_UNA_BUCA` = 4,
   `BUCHE_AL_GIORNO` = 1). Chi entra alle 10, fa due ore di seguito ed esce non ne ha nessuna.
7. **La giornata della classe.** In ogni classe le prime ore di ogni giorno (le obbligatorie) hanno lezione, e dopo
   un'ora buca la classe quel giorno non ha più lezione.
8. **Le aule.** Ogni lezione si fa in un'aula del tipo che la sua materia chiede, e in un'aula c'è una classe per
   volta.

Fino al 27 novembre 2026 erano scritte come tredici regole, una per frase; sono state raggruppate in otto su
richiesta di Alb, senza cambiare che cosa chiedono (gli orari sono gli stessi). Quando sono nate: aule, uscita e
giorno libero il 26 novembre 2026; le ore buche dei docenti il 26 (al massimo 2 al giorno); il massimo di 5 ore al
giorno e il giorno libero per ogni docente il 27. Il 27 novembre le ore buche erano diventate più strette (nessuna con
meno di 4 ore di lezione, al massimo 1 con 4 o 5), ma così circa un caso su nove restava senza risposta, e sono
tornate «al massimo 2 al giorno»; poi, la sera stessa, «al massimo 1 al giorno», con la pausa obbligatoria per chi
fa 5 ore, e infine com'è adesso: nessuna fino a 3 ore, una possibile con 4, obbligatoria con 5 (richieste di Alb).

Una scuola, nel programma e nello stato, è scritta così:

```
{"giorni": [nomi], "ore": [nomi], "classi": [nomi], "obbligatorie": N,
 "ore_di_seguito": 3, "classi_al_giorno": 3, "buche_al_giorno": 1, "ore_per_una_buca": 4,
 "lezioni_al_giorno": 5,
 "docenti": [[nome, ingresso, uscita, giorno libero], ...],
 "materie": [[nome, tipo di aula], ...], "aule": [[nome, tipo], ...],
 "voci": [[classe, docente, codocente o -1, ore, materia], ...]}
```

Il giorno libero è il numero di un giorno, o −2 (uno qualunque). Classi, docenti, materie e aule sono
numerati nell'ordine in cui compaiono nel file del caso. Le voci sono in ordine
di classe, poi di docente, di codocente e di materia.

## 3. Il file di un caso

Un file di testo, una riga per cosa, con i campi separati da TAB. Le righe vuote e quelle che cominciano con `#`
si saltano.

```
giorni	Lun	Mar	Mer	Gio	Ven	Sab
ore	08	09	10	11	12	13
obbligatorie	5
materia	Italiano	normale
materia	Scienze	laboratorio
aula	A01	normale
aula	Lab 1	laboratorio
docente	Sirius	0
docente	Zelda	2	5	Ven
lezione	C01	4	Italiano	Sirius
lezione	C01	2	Scienze	Sirius	Zelda
```

- `giorni` e `ore`: i nomi, nell'ordine.
- `obbligatorie N`: le prime N ore di ogni giorno hanno sempre lezione (ridotto tra 0 e il numero di ore).
- `docente NOME INGRESSO [USCITA [LIBERO]]`: ingresso e uscita sono indici di ore (ingresso 0 = dalla prima; senza
  uscita = fino alla fine); il docente insegna nelle ore da INGRESSO a USCITA − 1, e l'uscita deve venire dopo
  l'ingresso. LIBERO è il nome di un giorno o `qualunque` (se manca: qualunque). Nell'esempio Zelda entra
  alla terza ora, esce dopo la quinta e ha il venerdì libero.
- `materia NOME TIPO`: una materia e il tipo di aula che chiede. `aula NOME TIPO`: un'aula e il suo tipo. Il tipo è un
  nome qualunque: conta solo che sia scritto uguale.
- `lezione CLASSE ORE MATERIA DOCENTE [CODOCENTE]`: la classe nasce la prima volta che compare; la materia e i docenti
  devono essere già stati dichiarati; i due di una coppia devono essere diversi; le ore devono essere più di 0. Più
  righe con la stessa classe, la stessa materia e gli stessi docenti si sommano in una voce sola.

Il file è rifiutato (con la riga dell'errore) se una riga non si capisce o supera i limiti, se mancano giorni, ore
o lezioni, o se per una materia usata in una lezione non c'è nessuna aula del suo tipo.

## 4. Il generatore dei casi casuali

`python3 -m orario genera SEME CLASSI RITARDI` scrive un caso nel formato del capitolo 3. Lo stesso seme, lo stesso numero di
classi e la stessa scelta sui ritardi danno **sempre lo stesso caso**: i test notturni e la prova automatica
rigenerano i casi dai semi, quindi cambiare qualunque cosa qui sotto (anche l'ordine in cui si chiedono i numeri
casuali) cambia tutti i casi e tutti i risultati di riferimento.

La scuola generata: 6 giorni (Lun … Sab), 6 ore (08 … 13), 5 ore obbligatorie. Le classi si chiamano C01, C02, …
I docenti prendono i nomi, nell'ordine, da un elenco fisso di personaggi di fantasia (`orario/nomi.py`, 432 nomi, tutti diversi); oltre la fine dell'elenco il docente numero d si chiama `D` seguito da d + 1 su tre cifre.

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

**Le materie e le aule** si aggiungono sopra al caso, con una serie di numeri casuali a parte (stesso generatore,
seminato con `stato = seme × 0x9E3779B97F4A7C15 + 0x7A5C3E1`): classi, docenti, lezioni e ritardi di ogni seme sono
quelli che erano prima che materie e aule esistessero (controllato su 672 casi). Nell'ordine:
1. per ogni docente una cattedra, a caso tra queste quattordici (alcune hanno due materie): Italiano e Storia (due
   volte nell'elenco, quindi esce il doppio); Matematica e Fisica; Matematica; Inglese; Scienze e Chimica; Scienze
   motorie; Informatica; Diritto ed Economia; Francese; Arte; Religione; Storia e Filosofia; Tecnologia;
2. per ogni lezione, nell'ordine (mescolato) in cui il generatore le tiene, una materia a caso della cattedra del suo
   docente (in coppia: del primo dei due). Le ore da solo e quelle in coppia della stessa lezione hanno la stessa
   materia;
3. le aule: una normale per classe (`A01`, `A02`…); poi i laboratori (`Lab 1`…, per Scienze, Chimica, Informatica,
   Tecnologia) e le palestre (`Palestra 1`…, per Scienze motorie): tanti quante le ore di lezione la settimana che
   li chiedono, divise per 24 e arrotondate in su.

Nel file si scrivono le materie usate (in ordine di nome) e le aule, prima dei docenti. Con le aule così contate
nessuno dei 1.032 casi di prova cambia esito: bastano sempre.

**I giorni liberi e le uscite** si aggiungono allo stesso modo, con un'altra serie di numeri casuali a parte
(`stato = seme × 0x9E3779B97F4A7C15 + 0x51F15E7`): il resto del caso non cambia (controllato su 434 casi). Per ogni
docente, nell'ordine, si chiedono sempre quattro numeri, anche quando non servono: `sorte` tra 1 e 100, `giorno` tra
0 e 5, `sorte dell'uscita` tra 1 e 100, `uscita` tra 4 e 5.
- **L'ingresso anticipato.** Con un giorno libero le ore di un docente devono stare in cinque giorni: chi entra
  tardi viene fatto entrare un'ora prima, finché `ore ≤ min(6 − ingresso, 5) × 5` (in un giorno fa al massimo 5
  ore). Tocca circa un docente su quaranta.
- **Il giorno libero**, in tutti i casi, per tutti i docenti: con `sorte` fino a 25 il giorno preciso `giorno`;
  oltre, uno qualunque.
- **L'uscita**, solo nei casi con i ritardi e solo per chi (dopo l'eventuale anticipo) entra alla prima ora, con
  `sorte dell'uscita` fino a 30: esce dopo la quarta o la quinta ora (`uscita`), ritardata di un'ora alla volta
  finché `ore + 6 > min(uscita, 5) × 5` (6 è il margine). Tutti gli altri restano fino alla fine.

**Quando non si riesce.** Con tante classi i docenti sarebbero più di 250 e il caso non si genera: in pratica fino a
circa 70 classi si riesce, da 75 in su no. Da 84 classi in su è certo (almeno 30 ore per classe vuol dire almeno 3
docenti per classe) e il programma lo dice subito; tra 75 e 83 fa comunque i suoi 20.000 tentativi, e ci mette una
ventina di secondi.

Il generatore è lo stesso che c'era nel programma in C: i due sono stati confrontati su 3.556 casi (semi 0–39 e
altri, da 1 a 70 classi, senza e con ritardi) e danno lo stesso testo, carattere per carattere.

## 5. Il metodo: l'orario come sistema di conti

Il programma non riempie le celle una alla volta tornando indietro. Scrive tutto il caso come un sistema di conti con
incognite che valgono 0 o 1, e lo dà a un risolutore generico di programmi lineari interi: HiGHS, dentro la libreria
scipy di Python (`scipy.optimize.milp`). Il risolutore è compilato: il lavoro pesante lo fa lui. Dà l'orario, oppure
dimostra che non esiste.

**Le incognite**, tutte 0 o 1:
- `x[v, g, h]`: la voce v è nell'ora h del giorno g (nella sua classe);
- `b[k, g, h]`: la classe k in quell'ora ha l'ora buca;
- `c[t, k, g, h]`: il docente t comincia a insegnare nella classe k in quell'ora (nell'ora prima non c'era). Ce n'è
  una serie per ogni coppia (docente, classe) in cui il docente ha almeno una voce;
- `f[t, g]`: il giorno g è il giorno libero del docente t. Ci sono solo per i docenti che non hanno un giorno libero
  preciso.

**Le regole** (le otto del capitolo 2), ognuna come somme che devono stare tra due numeri:

| Regola | Come conto |
|---|---|
| Ogni cella ha una cosa sola | per ogni classe e ora: la somma delle x delle voci della classe, più b, fa 1 |
| 1. Le ore | la somma delle sue x nella settimana fa le sue ore |
| 2. Una classe per volta | per ogni docente e ora: la somma delle x delle sue voci è al massimo 1 |
| 3. A scuola | le x di una voce hanno massimo 0 prima dell'ingresso dei suoi docenti, dalla loro uscita in poi (per una coppia: l'ingresso di chi entra dopo, l'uscita di chi esce prima) e nel giorno libero preciso di uno di loro. Giorno libero qualunque: la somma delle f del docente nella settimana fa 1 (ne segna uno solo), e per ogni giorno `(lezioni del giorno) + 5 × f[t, g] ≤ 5`: è la riga delle ore di lezione della regola 6, con f in più. (Una riga per giorno invece di una per ora: sono meno righe, e su 480 casi il risolutore non va peggio: 55 al limite di tempo contro 58.) |
| 4. In una classe | **al massimo 3 ore**: per ogni docente, classe e giorno: la somma delle x è al massimo 3. **Di seguito**: `c[h] ≥ (somma delle x in h) − (somma delle x in h − 1)`, e in un giorno la somma delle c è al massimo 1 |
| 5. Le classi | per ogni docente e giorno: la somma delle c in tutte le classi è al massimo 3 (in ogni classe comincia al più una volta, quindi gli inizi sono le classi). Si scrive solo per i docenti che hanno più di 3 classi. |
| 6. La giornata del docente | **al massimo 5 ore di lezione**: per ogni docente e giorno: la somma delle x delle sue voci è al massimo 5 (per chi ha un giorno libero qualunque, più `5 × f`: vedi la regola 3). **Le ore buche**, tre richieste, per ogni docente e giorno. *Mai più di 1*: per ogni coppia di ore a < b con in mezzo più di 1 ora: `(lezioni in mezzo) ≥ (ore in mezzo − 1) × (lezione in a + lezione in b − 1)`. *Un buco solo con almeno 4 lezioni*: per ogni coppia a < b con almeno un'ora in mezzo: `(lezioni nelle ore fuori da a…b) + 3 × (lezioni in mezzo) ≥ 2 × (lezione in a + lezione in b − 1)`, cioè: se ha lezione in a e in b e in mezzo nessuna, quel giorno deve averne almeno altre due. *Con 5 ore la pausa*: in 5 ore di seguito le lezioni sono al massimo 4. «Lezione in h» è la somma delle x delle voci del docente in quell'ora. Con 6 ore sono 18 righe per docente e giorno; non servono incognite in più. Controllato su tutte le giornate possibili (da 4 a 8 ore): le righe accettano esattamente quelle consentite. |
| 7. La giornata della classe | **le prime ore hanno lezione**: b ha massimo 0 nelle ore obbligatorie. **Dopo la buca solo buche**: `b[h + 1] ≥ b[h]` |
| 8. Le aule | per ogni tipo di aula e ogni ora: la somma delle x delle voci che chiedono quel tipo è al massimo il numero delle aule di quel tipo. Si scrive solo se le classi che hanno voci di quel tipo sono più delle aule. |

Non c'è niente da rendere massimo o minimo: si cerca una risposta qualunque che rispetti tutto. Dalla risposta
all'orario: in ogni cella la voce la cui x vale 1, altrimenti l'ora buca.

**I passaggi**, nell'ordine (`orario/esatto.py`, uno per funzione): si dà un posto a ogni incognita (prima le x, poi
le b, le c e le f); si scrive una riga per ogni regola; si segnano le incognite che devono valere 0 (regole 3 e
7); si dà
tutto al risolutore; dalla sua risposta si legge l'orario; a ogni lezione si dà la sua aula.

Le aule non sono incognite: i conti chiedono solo che in ogni ora bastino. Quale aula tocca a ogni lezione si decide
dopo, ora per ora (passaggio 6): la classe resta nell'aula dove era l'ora prima, se è del tipo giusto e libera;
altrimenti va nella «sua» (tra le aule di quel tipo, quella con il numero della classe, ricominciando da capo se
sono meno delle classi), se è libera; altrimenti nella prima libera. Prima si sistemano tutte le classi che restano
dove erano, poi quelle che trovano libera la loro, poi le altre.

**In due tempi.** Le ore buche dei docenti (regola 6) rendono più lento dimostrare che un caso è impossibile. Per
questo il programma chiede due volte: prima **senza le ore buche dei docenti**: se già così un orario non esiste,
non esiste nemmeno con una richiesta in più, ed è la risposta; se l'orario trovato le rispetta per caso, è la
risposta. Altrimenti chiede da capo **con tutte le regole**, nel tempo che resta.

**Quasi tutti i casi hanno una risposta.** Quanto il risolutore ce la fa dipende molto da quanto sono strette le ore
buche dei docenti. Su 480 casi (semi 0–19, da 3 a 14 classi, senza e con ritardi), con 60 secondi l'uno e il solo
HiGHS:

| Ore buche dei docenti | Trovati | Impossibili | Al limite di tempo |
|---|---|---|---|
| al massimo 2 al giorno | 328 | 148 | 4 |
| al massimo 1 al giorno | 316 | 148 | 16 |
| al massimo 1 al giorno, obbligatoria con 5 ore di lezione | 290 | 151 | 39 |

La regola di adesso (nessuna fino a 3 ore, una possibile con 4, obbligatoria con 5) non è stata misurata qui: i
numeri li danno i test notturni su GitHub (capitolo 21), con tutti e due i metodi.
| nessuna con meno di 4 ore di lezione, al massimo 1 con 4 o 5 (provata il 27 novembre) | 277 | 148 | 55 |

Con i due metodi insieme (capitolo 20) i casi al limite sono meno: inciampano su casi diversi. Per la versione più
stretta sono state provate altre scritture (un'incognita per ogni giornata consentita; le incognite «ha già
cominciato» e «deve ancora finire»; il giorno libero con una riga per ora): nessuna andava meglio.

**Che cosa fa ogni richiesta** (27 novembre 2026, su 128 casi, togliendone una alla volta; misurato con le ore buche
nella versione stretta e prima che ogni docente avesse un giorno libero). Le regole non si
contraddicono: 87 casi su 128 hanno un orario che le rispetta tutte. E nessuna è di troppo: tolta una richiesta, tra
gli orari trovati quelli che la violano sono: l'ingresso, 62 su 125; l'uscita, 22 su 88; il giorno libero, 88 su 89;
le 3 ore in una classe, 88 su 88; le 3 classi al giorno, 51 su 87; le 5 ore al giorno, 46 su 87; le ore buche dei
docenti, 88 su 88; le prime ore con lezione, 85 su 90; le aule, 40 su 85. L'unica che le altre garantiscono già è
«dopo un'ora buca la classe non ha più lezione», e solo nelle scuole dove le ore non obbligatorie sono al più una
per giorno (come nei casi casuali: 5 obbligatorie su 6): lì l'unica ora buca possibile è l'ultima.

**Come ragiona il risolutore**, in breve. Prima tira tutte le conseguenze obbligate delle regole (chi ha tante ore
quanti posti è fisso; e così via, a catena). Poi fa finta che le risposte possano essere «a metà»: così il problema
diventa un sistema di conti normale, che si risolve tutto insieme; se nemmeno a metà esiste una risposta, il caso è
impossibile. Dove restano risposte a metà ne sceglie una, prova 1 e poi 0, e ricomincia. La differenza con una
ricerca che riempie le celle in ordine è che guarda tutte le regole di tutta la settimana insieme.

**Gli esiti**: «trovato», «impossibile» (dimostrato: un orario non esiste), «limite di tempo» (non ha finito nei
secondi che aveva: non si sa).

**Quanto è affidabile.** È stato confrontato, su 1.032 casi (semi 0–19 e 20–39 con 3–14 classi, semi 0–5 con 20–25
classi, senza e con ritardi), con la ricerca in C che c'era prima (capitolo 18), a cui si davano 2 milioni di nodi:
nessuna contraddizione. Tutti i 707 casi in cui la ricerca trovava un orario li trova anche lui; tutti i 288 che la
ricerca dichiarava impossibili li dichiara impossibili anche lui; e decide tutti i 37 che la ricerca lasciava al
limite: 33 hanno un orario, 4 sono impossibili. Ogni suo orario passa la verifica. Tempo per caso: mediana 0,2
secondi, massimo 19; un caso da 60 classi circa 1 secondo. Il tempo cambia molto da un caso all'altro, e anche con
l'ordine in cui sono scritti i conti.

**Che cosa non dà.** Non spiega perché un caso è impossibile, e non c'è un percorso da rivedere passo per passo.

## 6. La verifica dell'orario trovato

Quando il programma trova un orario lo ricontrolla subito una parte che **non usa niente del metodo**: legge solo la
scuola e le celle, e controlla da capo le otto regole del capitolo 2. Per ogni regola dice se è rispettata e, se no,
il primo punto in cui non lo è (per esempio «Ven 10: Zelda e' in C01 e in C03»). Nello stato finisce sotto
`verifica`; il sito la mostra in un riquadro. Un orario che non passa la verifica è un errore del programma, e nei
test notturni è un allarme.

Come guarda: un giorno alla volta, e dentro il giorno un'ora alla volta e una classe alla volta. Quello che riguarda
una cella (prime ore, ora buca, ingresso e uscita, giorno libero preciso, una classe per volta, voce della classe
giusta, l'aula) si controlla lì; quello che riguarda la giornata di un docente (le ore in una classe e se sono di
seguito, le classi, le ore di lezione, le ore buche) alla fine di ogni giorno; il giorno libero qualunque e le ore di
ogni voce alla fine.

Le regole compaiono nel risultato nell'ordine del capitolo 2, da 1 a 8. Di una regola che chiede più cose si scrive
il primo punto, qualunque delle sue richieste riguardi.

La verifica è la stessa che c'era nel programma in C: le due sono state confrontate su 372 orari, 10 validi e 362
rovinati apposta in vari modi, e danno lo stesso risultato regola per regola, con le stesse frasi.

## 7. I comandi

```
python3 -m orario genera SEME CLASSI RITARDI
python3 -m orario risolvi CASO [--stato FILE] [--tempo SECONDI]
python3 -m orario verifica CASO < ORARIO
```

| Comando | Che cosa fa |
|---|---|
| `genera` | Scrive su stdout un caso casuale (capitolo 4). RITARDI: 1 con, 0 senza. |
| `risolvi` | Cerca un orario per il caso e scrive lo stato (capitolo 8): su stdout alla fine, oppure in FILE, subito «in corso» e poi il risultato; il file si scrive a parte (`FILE.tmp`) e poi si rinomina, così chi legge trova sempre un file intero. `--tempo`: i secondi a disposizione (300 se non c'è). |
| `verifica` | Ricontrolla un orario fatto da altri: su stdin un numero per cella (la voce; −1: ora buca), nell'ordine delle celle, e poi un altro numero per cella (l'aula; −1: ora buca). Scrive `{"orario": [...], "aule": [...], "verifica": {...}}`. |

Escono con 0, o con 2 (e una riga su stderr) se il comando non va, il caso non si legge o non si genera, o quello che
arriva a `verifica` non è un orario di quel caso.

## 8. Lo stato (JSON)

```
{"in_corso": 0 | 1,                    1 finché i metodi non hanno finito tutti
 "esito": "in corso" | "trovato" | "impossibile" | "limite di tempo" | "contraddizione",
 "risolto_da": "highs" | "cpsat",       chi ha dato per primo una risposta certa (se c'è)
 "secondi": S,                          quando è arrivata quella risposta (se non c'è: quando hanno finito tutti)
 "celle": C,
 "scuola": {...},                       come nel capitolo 2
 "metodi": {"highs": {"esito", "secondi", "verificato"}, "cpsat": {...}},   capitolo 20
 "orario": [voce | -1, ...],            solo con «trovato»: un numero per cella
 "aule": [aula | -1, ...],              solo con «trovato»: l'aula di ogni cella
 "verifica": {"ok": 0 | 1, "controlli": [[regola, 0 | 1, dove], ...]}}   solo con «trovato»
```

## 9. Il sito: com'è fatto

- Python con Flask; server waitress (4 fili) se c'è, altrimenti quello di Flask. Niente login, niente database.
- Tutti i dati stanno in file dentro la **cartella di lavoro**: `sito/lavoro/`, o quella della variabile
  `SITO_LAVORO`. I file JSON si scrivono a parte e poi si rinominano.
- Il sito genera i casi da solo (chiamando `orario.genera`) e lancia `python3 -m orario risolvi` in un processo a
  parte, che scrive lo stato in un file: così il sito resta libero e la prova si può fermare.
- Gira **una cosa alla volta**: una prova oppure un giro di test notturni, mai insieme.
- Aspetto: fondo rosa cipria, testo prugna, un accento rosa lampone e il lilla; titoli con le grazie, il nome del
  sito in corsivo; tasti a pillola. In cima a ogni pagina il nome («Orario scolastico») e tre schede: «Prove»,
  «Scuole» e «Test notturni». I messaggi (errori, conferme) compaiono sotto, in un riquadro.
- I numeri sono scritti all'italiana (punti per le migliaia).

La cartella di lavoro:

```
prove/<numero>/info.json        di che caso si tratta (seme, classi, ritardi; oppure scuola e nome), il tempo,
                                quando è partita, se è stata fermata
prove/<numero>/caso.txt         il caso, come lo legge il programma
prove/<numero>/stato.json       lo stato, scritto dal programma
scuole/<numero>.json            una scuola scritta a mano: numero, nome, quando è stata modificata, i dati
notti/casi.json                 i casi dei test notturni
notti/impostazioni.json         il tempo di ogni caso, il giro della notte (acceso, ora), l'ultimo numero dato a un caso
notti/giri/<numero>.json        un giro e i suoi risultati
notti/visti.json                per ogni caso, il risultato di cui si è presa visione
```

## 10. La pagina «Prove» (`/`)

- Un modulo: **Seme** (da 0), **Classi** (da 1 a 120), **Ritardi** (senza / con), e il tasto **Fai l'orario** (spento
  se una prova sta girando). I valori iniziali sono quelli dell'ultima prova.
- Una nota: che cosa decide il seme; che il programma dà l'orario o dimostra che non esiste, di solito in pochi
  secondi (al massimo 300); una prova alla volta; il collegamento alla prova che sta girando.
- **Le prove fatte**: una tabella, dalla più recente: casella per sceglierla, numero (collegamento), quando è
  partita, il caso («seme S, C classi, con/senza ritardi», oppure il nome della scuola, che porta ai suoi dati), esito (etichetta colorata; «non valido» se l'orario non passa la verifica), tempo.
- **Elimina le prove scelte**: le caselle, una in cima per tutte; il tasto dice quante sono e chiede conferma. La
  prova che sta girando non si può scegliere.

Avviare una prova: si genera il caso (se non si riesce, un messaggio), si lancia il programma con 300 secondi, e si
passa alla pagina della prova. Il numero di una prova è il più alto esistente più uno.

Gli esiti che una prova può mostrare: quelli del programma («in corso», «trovato», «impossibile», «limite di
tempo»), più «fermata» (fermata a mano) e «interrotta» (il sito si è riavviato mentre girava, o il programma non è
partito, per esempio perché manca scipy).

## 11. La pagina di una prova (`/prova/N`)

Dall'alto in basso.

- «← Tutte le prove»; il titolo «Prova N: seme S, C classi, con/senza ritardi», oppure «Prova N: nome della
  scuola»; l'esito (etichetta) e il tempo.
- I tasti: **Ferma** (solo mentre gira), **Rifai** (una prova nuova sullo stesso caso), **Aggiungi ai notturni** (il
  caso entra tra i test notturni). Per una prova su una scuola: «Rifai» fa una prova nuova con i dati che la scuola
  ha adesso, al posto di «Aggiungi ai notturni» c'è il collegamento «I dati della scuola», e se la scuola è stata
  eliminata non c'è nessuno dei due.
- Un messaggio quando un orario non c'è: «Un orario non esiste: il programma lo dimostra…» per «impossibile»; che
  non si sa, per «limite di tempo»; che cosa è successo, per «fermata» e «interrotta».
- Il riquadro della **verifica**, quando l'orario c'è: «verificato» o «non valido», e le otto regole con ✓ o ✗ e, per
  quelle non rispettate, dove.
- **L'orario**, con due tasti per scegliere come guardarlo (lo stesso orario; si parte da «Per classe»):
  - **Per classe**: una riga per classe. In una cella tre righe: la materia; chi la insegna (in due: docente e
    codocente, separati da una virgola); più chiara, l'aula. «–» per l'ora buca.
  - **Per docente**: una riga per docente. In una cella tre righe: la classe dove insegna e, se sono in due, «con» e
    il nome dell'altro; la materia; più chiara, l'aula. Vuota se è libero; grigie le ore in cui non è a scuola
    (prima dell'ingresso, dall'uscita in poi, nel suo giorno libero preciso).

  In tutti e due una colonna per ogni ora della settimana, raggruppate per giorno; la colonna dei nomi resta ferma
  mentre si scorre di lato; tutte le celle larghe uguali.
- **Il caso** (tre sezioni chiuse): le classi con le loro ore e le loro lezioni (materia, docenti, ore); i docenti
  con le ore, quando sono a scuola (solo se non è «sempre»: «dalle 10», «fino alle 12», «libero Ven» per chi ha un giorno
  libero preciso), le materie e le classi; le aule con il tipo e le materie che lo chiedono.
  Nelle prove fatte prima che ci fossero materie e aule queste mancano, e la sezione delle aule non si vede.

Mentre la prova gira la pagina chiede lo stato ogni secondo.

## 12. La pagina «Test notturni» (`/notturni`)

- **Il giro.** «Avvia il giro» (spento se non ci sono casi, o se gira un giro o una prova) e «Ferma»; una riga che
  dice del giro in corso (casi fatti su quanti) o dell'ultimo (numero, quando, come è finito, il tempo per caso, se è
  partito da solo). Le impostazioni: «Ogni caso ha al massimo N secondi» (da 1 a 3600; in partenza 120), «ogni notte
  il giro parte da solo alle HH:MM» (in partenza spento, 22:00), e «Salva».
- **I cambiamenti.** Una tabella per tipo, solo quelli che ci sono: Allarmi; Non risolto → Risolto; Risolto → Non
  risolto; Altri cambiamenti. In ogni riga: seme, classi, ritardi, l'esito di prima (con il giro) e quello di adesso.
  In cima a ogni tabella il tasto **Visto** (o «Visti tutti e N»).
- **I casi.** Un modulo per aggiungerne: semi da… a…, classi da… a…, ritardi (senza e con / senza / con); al massimo
  2000 per volta; quelli che ci sono già non si ripetono. «Togli i casi scelti», con le caselle. La tabella: seme,
  classi, ritardi, esito, tempo, e «rispetto al giro prima» (uguale, o l'esito di prima), e in alto il conto dei casi
  per esito.

Come funziona un giro: tutti i casi, uno dopo l'altro, ognuno generato e risolto in una cartella provvisoria con il
tempo scelto; di ogni caso si tengono esito, secondi, celle e se l'orario passa la verifica. Il file del giro si
riscrive a ogni caso. Un giro finisce «finito», «fermato», «interrotto» (il sito si è riavviato) o con un errore. Il
giro della notte: un orologio guarda ogni 20 secondi se è l'ora scelta e se oggi non è già partito.

**I cambiamenti: la regola.** Il risultato di un caso è (esito, tempo a disposizione). L'ultimo risultato si confronta
con il **riferimento**: quello di cui si è presa visione, se era con lo stesso tempo; altrimenti il primo che il caso
ha avuto con quel tempo. Se è diverso il caso sta tra i cambiamenti finché non si preme «Visto». Il tipo:
- **allarme**: l'orario non passa la verifica, oppure il caso non è riuscito o non si è generato, oppure uno dei due
  esiti è «trovato» e l'altro «impossibile» (uno dei due è sbagliato). Un orario che non passa la verifica si mostra
  anche se è il primo risultato;
- «risolto» se adesso è trovato, «non risolto» se prima era trovato, altrimenti «altro».

I tempi non si confrontano: cambiano da una volta all'altra.

## 13. Tutti gli indirizzi del sito

| Indirizzo | Metodo | Che cosa fa |
|---|---|---|
| `/` | GET | La pagina delle prove. |
| `/avvia` | POST | Avvia una prova (seme, classi, ritardi). |
| `/prova/N` | GET | La pagina di una prova. |
| `/prova/N/stato.json` | GET | Lo stato (capitolo 8), più `gira`. |
| `/ferma` | POST | Ferma la prova che gira. |
| `/elimina` | POST | Elimina le prove (numeri, separati da virgole). |
| `/scuole` | GET | L'elenco delle scuole e il modulo per crearne una. |
| `/scuole/nuova` | POST | Crea una scuola (nome, partenza: `vuota` o `casuale`; con `casuale` anche seme, classi, ritardi). |
| `/scuola/N` | GET | La pagina dei dati di una scuola. |
| `/scuola/N/salva` | POST | Salva: il corpo è JSON `{"nome", "dati"}`; risponde `{"problemi": [...]}`, o 400 con `{"errore"}`. |
| `/scuola/N/orario` | POST | Fa partire una prova con i dati salvati (se non hanno problemi). |
| `/scuola/N/elimina` | POST | Elimina la scuola (le prove già fatte restano). |
| `/notturni` | GET | La pagina dei test notturni. |
| `/notturni/stato.json` | GET | La situazione: impostazioni, giro, casi, cambiamenti. |
| `/notturni/aggiungi` | POST | Aggiunge casi (semi_da, semi_a, classi_da, classi_a, ritardi). |
| `/notturni/da-prova` | POST | Aggiunge il caso di una prova (prova). |
| `/notturni/togli` | POST | Toglie casi (numeri). |
| `/notturni/impostazioni` | POST | Tempo e giro della notte (secondi, notte, ora). |
| `/notturni/avvia`, `/notturni/ferma` | POST | Avvia o ferma un giro. |
| `/notturni/visto` | POST | Conferma la presa visione (numeri). |

## 14. I limiti e i numeri fissi, in un posto solo

| Che cosa | Valore |
|---|---|
| Giorni, ore al giorno, classi, docenti, voci (massimi) | 7, 10, 120, 250, 4000 |
| Materie, aule (massimi) | 60, 300 |
| Lunghezza di un nome | al massimo 47 byte |
| Ore di un docente in una classe in un giorno | al massimo 3, di seguito |
| Classi di un docente in un giorno | al massimo 3 |
| Laboratori e palestre dei casi casuali | uno ogni 24 ore di lezione la settimana che li chiedono |
| Ore buche di un docente in un giorno | fino a 3 ore di lezione nessuna; con 4, nessuna o una; con 5, esattamente una |
| Ore di lezione di un docente in un giorno | al massimo 5 |
| Casi casuali | 6 giorni × 6 ore, 5 obbligatorie; 30–36 ore per classe; lezioni da 1 a 6 ore; 5–24 ore per docente; ingresso al più alla quarta ora |
| Tentativi del generatore | 20.000 |
| Classi di un caso casuale | in pratica fino a circa 70; da 84 in su mai |
| Tempo di una prova | 300 secondi |
| Tempo di un caso dei test notturni | da 1 a 3600 secondi (in partenza 120) |
| Casi notturni aggiunti per volta | al massimo 2000 |
| Aggiornamento delle pagine | prova: ogni 1 s; notturni: ogni 2 s |

Il programma non ha un limite di «nodi»: l'unico limite è il tempo.

## 15. La prova automatica e le verifiche da rifare dopo una modifica

`python3 prove/prova.py` genera **54 casi** e ne cerca l'orario, con 60 secondi l'uno: semi da 0 a 5 × classi 3, 5,
8, 12 × senza e con ritardi, più (10, 3, con), (37, 3, con), (4, 7, con), (1, 14, con), (2, 20, senza), (3, 30,
con). Di ognuno confronta con `prove/attesi.json` tre cose: l'impronta del caso generato (i primi 12 caratteri dello
SHA-256 del testo: se cambia, è cambiato il generatore), l'esito, e se l'orario passa la verifica. «Limite di tempo»
non è una risposta (dipende dal computer): un caso che finisce al limite, nella prova o nei risultati attesi, si
confronta solo per l'impronta, e alla fine si dice quanti sono. I tempi non si confrontano. Con `--aggiorna`
riscrive il file: solo dopo una modifica voluta.

Dopo ogni modifica al programma:
1. `prove/prova.py` deve dire «tutto come atteso» (ci mette qualche minuto).
2. Se si tocca `genera.py`: i casi non devono cambiare (le impronte in `attesi.json`), a meno che non sia voluto. Se
   è voluto, cambiano tutti i casi di tutti i semi, e i risultati dei test notturni non sono più confrontabili.
3. Se si tocca `esatto.py`: ogni orario trovato deve passare la verifica, e nessun caso deve passare da «trovato» a
   «impossibile» o il contrario (sono risposte certe: una delle due sarebbe sbagliata). Va rifatto su tanti casi,
   non solo sui 54.
4. Se si tocca `verifica.py`: deve continuare a riconoscere gli orari rovinati (scambiare due celle, mettere una
   buca, una voce di un'altra classe…), regola per regola.

## 16. Installazione e avvio

Servono Python 3 e i pacchetti di `sito/requirements.txt`: flask, waitress, scipy, ortools. Non serve un
compilatore. Senza ortools il programma va lo stesso, con il solo HiGHS (CP-SAT risulta «non installato»).

```
pip install -r sito/requirements.txt
python3 -m sito avvia --porta 8000                  # solo su questo computer
python3 -m sito avvia --host 0.0.0.0 --porta 8000   # visibile da tutta la rete locale (Raspberry)
```

A mano, senza il sito: `python3 -m orario genera 10 3 1 > caso.txt`, poi `python3 -m orario risolvi caso.txt`.

Sul Raspberry (`raspberry@192.168.1.5`, servizio `orario-sito`, repository in `~/orario`, ambiente Python in
`~/orario-venv`), per aggiornare:
`cd ~/orario && git pull && ~/orario-venv/bin/pip install -r sito/requirements.txt && sudo systemctl restart orario-sito`,
poi Ctrl+F5 nel browser.

## 17. I file del repository

```
orario/__init__.py               che cos'è il programma
orario/scuola.py                 la scuola, le regole, i limiti di grandezza, la lettura di un caso
orario/genera.py                 il generatore dei casi casuali
orario/nomi.py                   i nomi dei docenti dei casi casuali
orario/esatto.py                 il primo metodo (HiGHS): il caso scritto come conti, e la sua soluzione
orario/cpsat.py                  il secondo metodo (CP-SAT di OR-Tools)
orario/insieme.py                i due metodi insieme, ognuno in un processo suo
orario/verifica.py               la verifica dell'orario trovato
orario/__main__.py               i comandi
sito/__main__.py                 l'avvio del sito
sito/app.py                      le pagine e gli indirizzi
sito/prove.py                    le prove
sito/scuole.py                   le scuole scritte a mano
sito/notturni.py                 i test notturni
sito/programma.py                generare un caso e lanciare il programma
sito/archivio.py                 la cartella di lavoro e i file JSON
sito/templates/                  base, indice, prova, scuole, scuola, notturni (HTML, con il loro JavaScript)
sito/static/stile.css            lo stile
prove/prova.py, attesi.json      la prova automatica
prove/notturno.py                i test notturni fuori dal sito (li lancia GitHub ogni notte)
.github/workflows/notturni.yml   quando e come GitHub li lancia
LEGGIMI.md                       la presentazione breve
DESCRIZIONE.md                   questo documento
CLAUDE.md                        le regole di lavoro su questo repository
bf_search/                       SQL di Alb, a parte
```

## 18. Che cosa c'era prima, e dov'è

Fino al 26 novembre 2026 il programma era in C e faceva l'orario con una **ricerca**: riempiva le celle un'ora alla
volta, la classe con meno scelte per prima, e tornava indietro quando un controllo non passava (sette controlli, da A
a G). Il sito registrava ogni passo e lo faceva rivedere: l'orario a ogni passo, il controllo che non passava con i
pezzi del suo conto e le celle colorate, i ritorni indietro contati per cella e per colpevole, il profilo della
ricerca, la possibilità di continuarla o di cambiare una scelta a mano.

Con il limite di 2 milioni di nodi la ricerca risolveva 707 casi su 1.032 e ne lasciava 37 senza risposta; il metodo
di adesso li decide tutti (capitolo 5). Per questo, su richiesta di Alb, `main` è passato al metodo esatto e tutto il
resto è stato riscritto in Python.

Dove sono le versioni di prima:

| Ramo | Che cosa c'è |
|---|---|
| `stabile/2026-11-26` | L'ultima versione con la ricerca in C: tutto quello che è descritto qui sopra, più il metodo esatto accanto. Ha il suo `DESCRIZIONE.md` completo. |
| `stabile/2026-11-21` | La ricerca in C com'era il 21 novembre. |
| `stabile/2026-11-18` e precedenti | Il programma ancora prima: un'altra ricerca in C, con le «potature», e il sito con Postgres. |

## 19. Le scuole scritte a mano: le pagine «Scuole»

Oltre ai casi casuali il sito tiene i dati di scuole vere, scritti a mano, e ne fa fare l'orario.

**I dati di una scuola** (`sito/scuole.py`; un file JSON per scuola), con i nomi scritti per esteso:

```
{"giorni": [nomi], "ore": [nomi], "obbligatorie": N,
 "materie": [[nome, tipo di aula], ...], "aule": [[nome, tipo], ...],
 "docenti": [[nome, ingresso, uscita, giorno libero], ...],
 "lezioni": [[classe, materia, docente, codocente o "", ore], ...]}
```

Ingresso e uscita sono indici di ore; il giorno libero è il nome di un giorno o `qualunque`. Le classi non
hanno un elenco: sono quelle che compaiono nelle lezioni.

**L'elenco** (`/scuole`). Un modulo per una scuola nuova: il nome (se manca: «Scuola N») e da che cosa si parte: una
scuola vuota (sei giorni da Lun a Sab, sei ore da 08 a 13, 5 prime ore con lezione, nient'altro) oppure un caso
casuale (seme, classi, ritardi: i suoi dati diventano quelli della scuola). Sotto, le scuole dalla più recente: nome
(collegamento), classi, docenti, lezioni, quando è stata modificata, «Elimina» (chiede conferma).

**La pagina di una scuola** (`/scuola/N`). In alto il nome (un campo), **Salva**, **Fai l'orario**, e lo stato
(«Modifiche non salvate.», «Salvato.»). Sotto, se i dati hanno problemi, il loro elenco. Poi cinque tasti che
scelgono la sezione, una alla volta (si parte da «Docenti»):

- **Docenti**: una riga per docente: nome; «Entra» (dalle…, una scelta per ora); «Esce» (dopo le…, oppure «alla
  fine»); «Giorno libero» (uno qualunque, o un giorno: ogni docente ne ha uno); le sue ore di lezione, contate dalle lezioni. Se le
  ore sono più di quelle che può fare, `min(uscita − ingresso, 5) × (giorni − 1)`, si vedono come «22 su 20».
- **Lezioni**: in cima la scelta della classe (o tutte) e le ore della classe scelta, con un avviso se non stanno tra
  `prime ore × giorni` e `ore × giorni`. Una riga per lezione: classe (un campo: scrivendo un nome nuovo nasce una
  classe), materia, docente, codocente (o «–»), ore.
- **Materie**: nome e tipo di aula. **Aule**: nome e tipo. Il tipo è un campo libero che suggerisce i tipi già
  scritti e i tre soliti (normale, laboratorio, palestra).
- **Settimana**: i giorni e le ore, con i nomi separati da virgole, e le prime ore con lezione.

Ogni sezione ha «Aggiungi…» in fondo e «Togli» su ogni riga. Cambiare il nome di un docente o di una materia lo
cambia anche nelle lezioni; togliere un docente o una materia che ha lezioni toglie anche quelle, dopo una conferma.
Se cambiano le ore, chi usciva alla fine esce ancora alla fine; un giorno libero che non c'è più diventa «uno qualunque».

La pagina tiene i dati e li manda tutti insieme con «Salva». Si salva sempre, anche con i dati incompleti. «Fai
l'orario» prima salva, e fa partire una prova solo se non ci sono problemi. Lasciando la pagina con modifiche non
salvate il browser chiede conferma.

**I problemi** (al massimo 30 per volta), con parole: manca un giorno o un'ora; un nome vuoto o ripetuto; una
materia che chiede un tipo di aula che non c'è; l'uscita non dopo l'entrata; un giorno libero che non è un giorno;
una lezione senza classe, materia o docente, con il codocente uguale al docente, o con meno di un'ora; nessuna
lezione. Se di questi non ce n'è nessuno il programma prova a leggere il caso (capitolo 3), e l'eventuale rifiuto
(i limiti di grandezza) è l'ultimo problema.

**Dall'elenco all'orario**: i dati si scrivono come file di un caso (capitolo 3) e parte una prova come le altre
(capitoli 10 e 11); in `info.json` c'è `{"scuola": N, "nome": ...}` al posto di seme, classi e ritardi.

## 20. I due metodi insieme: HiGHS e CP-SAT

Dal 27 novembre 2026 (richiesta di Alb) l'orario lo cercano due metodi, nello stesso momento.

- **HiGHS** (`orario/esatto.py`, capitolo 5): fa i conti su tutta la settimana insieme.
- **CP-SAT** (`orario/cpsat.py`), il risolutore di OR-Tools: è una ricerca che fissa delle scelte, ne tira le
  conseguenze e, quando arriva a una contraddizione, ricava quali poche scelte l'hanno causata e non le rifà più
  insieme. Le regole sono le stesse otto, scritte in modo più diretto: «esattamente uno» (la cella), «al massimo
  uno» (il docente in un'ora), «se questo allora quello» (dopo la buca), e la giornata del docente come **tabella**:
  l'elenco delle giornate consentite (con 6 ore: al massimo 5 lezioni; nessuna ora buca fino a 3 lezioni, al massimo una con 4, la pausa con 5), e la sua deve essere una di
  quelle. Le incognite che non possono valere 1 (prima dell'ingresso, dopo l'uscita, nel giorno libero preciso, l'ora
  buca nelle prime ore) non vengono nemmeno create. Usa fino a quattro processori, lasciandone uno a HiGHS.

**Perché tutti e due.** Nessuno dei due è meglio dell'altro in tutto, e inciampano su casi diversi. Sui 480 casi
(semi 0–19, da 3 a 14 classi), con 60 secondi: HiGHS risponde in 476 (misurato su una macchina più veloce del
Raspberry), CP-SAT in 472 (misurato sul Raspberry), almeno uno dei due in 479. Nessuna risposta dell'uno contraddice
l'altro, e ogni orario passa la verifica.

**Come girano** (`orario/insieme.py`). Un processo per metodo, ognuno con tutto il tempo. Basta che uno dia una
risposta certa («trovato» o «impossibile») e il caso è deciso; l'altro continua fino alla sua risposta o al suo
tempo, così di ogni caso si vede sempre come sono andati tutti e due. Con `--stato FILE` lo stato si riscrive a ogni
risposta: la prima si vede subito, e `in_corso` resta 1 finché non hanno finito entrambi. Ogni orario trovato si
ricontrolla con la verifica, di chiunque sia; nello stato va il primo.

Per ogni metodo nello stato: `esito` («in corso», «trovato», «impossibile», «limite di tempo», «non installato» se
manca il suo pacchetto, «errore: …»), `secondi` (da quando sono partiti), `verificato` (se ha trovato un orario).

**L'esito del caso**: «trovato» se uno ha trovato; «impossibile» se uno lo ha dimostrato; «limite di tempo» se
nessuno ha risposto; **«contraddizione»** se uno ha trovato un orario valido e l'altro dice che non esiste: uno dei
due sbaglia, ed è un allarme.

**Nel sito.** Nella pagina di una prova, sotto l'esito, una riga per metodo: esito, tempo, e «ha risposto per
primo». Nei test notturni due colonne in più, HiGHS e CP-SAT, con esito e tempo di ognuno, e in alto il conto: in
quanti casi ognuno ha dato una risposta certa, e in quanti è stato l'unico. Una contraddizione va tra gli allarmi,
anche se è il primo risultato del caso. «Ferma» chiude tutti e due i processi.

**La prova automatica** (`prove/prova.py`) usa i due metodi insieme e alla fine dice in quanti casi ognuno ha dato
una risposta certa (o che non è installato).

Come è stato provato: CP-SAT sul Raspberry, dal ramo `prova/cp-sat` (480 casi). Il modello, con un finto risolutore
che controlla solo le regole: su 3.600 orari rovinati dice la stessa cosa della verifica. Il girare insieme (prima
risposta, contraddizione, arresto), con un finto OR-Tools a cui risponde HiGHS.

## 21. I test notturni su GitHub

Oltre ai test notturni del sito (capitolo 12), che girano sul Raspberry, ogni notte gli stessi casi girano sulle
macchine di GitHub (GitHub Actions: gratis per i repository pubblici), anche a Raspberry spento. (Dal 27 novembre
2026, richiesta di Alb.)

**Che cosa fa** (`.github/workflows/notturni.yml`). Ogni notte all'1:23 UTC, oppure a mano dal tasto «Run workflow»
della scheda «Actions» (scegliendo semi, classi e secondi per caso): quattro macchine installano scipy e ortools e
fanno girare ognuna un quarto dei casi; una quinta mette insieme i risultati e scrive il riepilogo. Di norma: semi
0–19, classi 3–14, senza e con ritardi (480 casi), 60 secondi per caso.

**Lo script** (`prove/notturno.py`). Per ogni caso lancia i due metodi insieme (`orario/insieme.py`) e tiene:
l'esito, i secondi, chi ha risposto per primo, se l'orario passa la verifica e, per ogni metodo, esito, secondi e
verifica. `--parti N --parte I` prende un caso ogni N, partendo dall'I-esimo. `--riepilogo FILE…` mette insieme i
risultati e scrive in Markdown: quanti casi per esito; per ogni metodo in quanti casi ha dato una risposta certa, in
quanti è stato l'unico, quanti ne ha lasciati al limite e i secondi in tutto; gli allarmi; i casi senza risposta; i
casi in cui ha risposto uno solo dei due.

**Dove si legge.** Nella scheda «Actions» del repository, aprendo l'ultima esecuzione di «Test notturni»: il
riepilogo è nella pagina, e in fondo si scaricano `risultati.json` e `riepilogo.md`.

**Gli allarmi.** Un orario che non passa la verifica, o i due metodi che si contraddicono: l'esecuzione risulta
fallita, e GitHub avvisa per posta.

Questi risultati non compaiono nella pagina «Test notturni» del sito, che legge i file del Raspberry. GitHub
sospende le esecuzioni a orario dopo 60 giorni senza attività nel repository: basta riattivarle dalla scheda
«Actions».

