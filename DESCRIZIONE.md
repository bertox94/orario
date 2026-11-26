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

---

## 1. A che cosa serve, e le parole

Dato l'elenco delle lezioni di una scuola (quale docente, o quale coppia di docenti, fa quante ore in quale classe),
il **programma** (`orario/`) trova un orario settimanale che rispetta le regole del capitolo 2, oppure dimostra che
non esiste.

Il **sito** (`sito/`) non calcola niente: lancia il programma e ne mostra i risultati. Ha due parti: le **prove** (un
caso e il suo orario) e i **test notturni** (un elenco di casi rifatti tutti insieme, per vedere che cosa cambia
quando cambia il programma).

I casi sono **casuali ma riproducibili**: un caso è identificato da tre cose (un seme, il numero di classi, con o
senza ritardi) e le stesse tre danno sempre la stessa scuola.

| Parola | Che cosa vuol dire |
|---|---|
| cella | Un posto dell'orario: una classe, in un'ora, in un giorno. |
| voce | Una lezione di una classe: un docente da solo, oppure una coppia (docente e codocente insieme), con le ore che devono fare in una settimana. |
| ora buca | Una cella senza lezione. |
| ingresso | L'ora (indice, da 0) da cui un docente è a scuola. Prima non può insegnare. Vale per tutti i giorni. |
| ore obbligatorie | Le prime ore di ogni giorno, che in ogni classe hanno sempre lezione. |

Numerazione. Giorni, ore, classi, docenti e voci sono numerati da 0. La cella di (giorno, ora, classe) ha il numero
`(giorno × ore + ora) × classi + classe`: le celle sono in ordine di tempo, e dentro la stessa ora in ordine di
classe. Un orario è un elenco con un numero per cella: il numero di una voce, oppure −1 per l'ora buca.

## 2. La scuola e le regole dell'orario

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
4. In una classe, in un giorno, un docente fa al massimo **3** ore (`ORE_DI_SEGUITO`).
5. Le ore di un docente in una classe, in un giorno, sono di seguito (senza buchi).
6. In un giorno un docente insegna in al massimo **3** classi (`CLASSI_AL_GIORNO`).
7. In ogni classe le prime ore di ogni giorno (le obbligatorie) hanno lezione.
8. Dopo un'ora buca, la classe quel giorno non ha più lezione.
9. In un giorno un docente ha al massimo 2 ore buche (`BUCHE_AL_GIORNO`): ore senza lezione tra la sua prima e la
   sua ultima lezione di quel giorno. Le ore prima della prima e dopo l'ultima non contano. (Dal 26 novembre 2026,
   richiesta di Alb.)

Le regole 4 e 5 contano le ore di un docente in una classe sommando tutte le sue voci lì (da solo e in coppia).

Una scuola, nel programma e nello stato, è scritta così:

```
{"giorni": [nomi], "ore": [nomi], "classi": [nomi], "obbligatorie": N,
 "ore_di_seguito": 3, "classi_al_giorno": 3, "buche_al_giorno": 2,
 "docenti": [[nome, ingresso], ...],
 "voci": [[classe, docente, codocente o -1, ore], ...]}
```

Classi e docenti sono numerati nell'ordine in cui compaiono nel file del caso.

## 3. Il file di un caso

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
- `p[t, g, h]`: il docente t quel giorno ha già cominciato (ha lezione in quell'ora o in una prima);
- `d[t, g, h]`: il docente t quel giorno deve ancora finire (ha lezione in quell'ora o in una dopo). Un'ora è
  «dentro la giornata» del docente quando p e d valgono 1 tutti e due: le ore buche sono quelle dentro la giornata
  senza lezione.

**Le regole** (le nove del capitolo 2), ognuna come una somma che deve stare tra due numeri:

| Regola | Come conto |
|---|---|
| Ogni cella ha una cosa sola | per ogni classe e ora: la somma delle x delle voci della classe, più b, fa 1 |
| 1. Ogni voce fa le sue ore | la somma delle sue x nella settimana fa le sue ore |
| 2. Un docente in una classe per volta | per ogni docente e ora: la somma delle x delle sue voci è al massimo 1 |
| 3. Non prima dell'ingresso | quelle x hanno massimo 0 (per una coppia, l'ingresso di chi entra dopo) |
| 4. Al massimo 3 ore al giorno in una classe | per ogni docente, classe e giorno: la somma delle x è al massimo 3 |
| 5. Di seguito | `c[h] ≥ (somma delle x in h) − (somma delle x in h − 1)`, e in un giorno la somma delle c è al massimo 1 |
| 6. Al massimo 3 classi al giorno | per ogni docente e giorno: la somma delle c in tutte le classi è al massimo 3 (in ogni classe comincia al più una volta, quindi gli inizi sono le classi). Si scrive solo per i docenti che hanno più di 3 classi. |
| 7. Le prime ore hanno lezione | b ha massimo 0 nelle ore obbligatorie |
| 8. Dopo la buca solo buche | `b[h + 1] ≥ b[h]` |
| 9. Al massimo 2 ore buche al giorno per un docente | `p[h] ≥ lezione in h`, `p[h] ≥ p[h − 1]`; `d[h] ≥ lezione in h`, `d[h] ≥ d[h + 1]`; in un giorno la somma di `p[h] + d[h] − 1 − lezione in h` è al massimo 2 (e almeno −ore, per i giorni senza lezioni). «Lezione in h» è la somma delle x delle voci del docente in quell'ora. |

Non c'è niente da rendere massimo o minimo: si cerca una risposta qualunque che rispetti tutto. Dalla risposta
all'orario: in ogni cella la voce la cui x vale 1, altrimenti l'ora buca.

**I passaggi**, nell'ordine (`orario/esatto.py`, uno per funzione): si dà un posto a ogni incognita (prima le x, poi
le b, le c, le p e le d); si scrive una riga per ogni regola; si segnano le incognite che devono valere 0 (regole 3 e 7); si dà
tutto al risolutore; dalla sua risposta si legge l'orario.

**In due tempi.** La regola 9 rende molto più lento dimostrare che un caso è impossibile (con tutte le regole insieme
alcuni casi impossibili passavano da pochi secondi a più di 200). Per questo il programma chiede due volte:
prima **senza la regola 9**: se già così un orario non esiste, non esiste nemmeno con una regola in più, ed è la
risposta; se l'orario trovato rispetta per caso anche la regola 9, è la risposta. Altrimenti chiede da capo **con
tutte le regole**, nel tempo che resta. Sui 1.032 casi: tutti i 740 che avevano un orario ne hanno uno anche con la
regola 9 (solo in 20 lo rispettava già quello trovato senza); mediana 0,5 secondi, massimo 14.

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
scuola e le celle, e controlla da capo le nove regole del capitolo 2. Per ogni regola dice se è rispettata e, se no,
il primo punto in cui non lo è (per esempio «Ven 10: Zelda e' in C01 e in C03»). Nello stato finisce sotto
`verifica`; il sito la mostra in un riquadro. Un orario che non passa la verifica è un errore del programma, e nei
test notturni è un allarme.

Come guarda: un giorno alla volta, e dentro il giorno un'ora alla volta e una classe alla volta. Le regole che
riguardano una cella (prime ore, ora buca, ingresso, una classe per volta, voce della classe giusta) si controllano
lì; quelle sulla giornata di un docente (ore al giorno, di seguito, classi al giorno) alla fine di ogni giorno; le
ore di ogni voce alla fine.

Le regole, nell'ordine in cui compaiono nel risultato: le ore di ogni voce; una classe per volta; l'ingresso; il
massimo di ore al giorno in una classe; di seguito; il massimo di classi al giorno; le prime ore; dopo la buca;
le ore buche di un docente.

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
| `verifica` | Ricontrolla un orario fatto da altri: su stdin un numero per cella (−1: ora buca), nell'ordine delle celle. Scrive `{"orario": [...], "verifica": {...}}`. |

Escono con 0, o con 2 (e una riga su stderr) se il comando non va, il caso non si legge o non si genera, o quello che
arriva a `verifica` non è un orario di quel caso.

## 8. Lo stato (JSON)

```
{"in_corso": 0 | 1,
 "esito": "in corso" | "trovato" | "impossibile" | "limite di tempo",
 "secondi": S,                          da quando il caso è stato letto
 "celle": C,
 "scuola": {...},                       come nel capitolo 2
 "orario": [voce | -1, ...],            solo con «trovato»: un numero per cella
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
  sito in corsivo; tasti a pillola. In cima a ogni pagina il nome («Orario scolastico») e due schede: «Prove» e
  «Test notturni». I messaggi (errori, conferme) compaiono sotto, in un riquadro.
- I numeri sono scritti all'italiana (punti per le migliaia).

La cartella di lavoro:

```
prove/<numero>/info.json        il caso (seme, classi, ritardi), il tempo, quando è partita, se è stata fermata
prove/<numero>/caso.txt         il caso, come lo legge il programma
prove/<numero>/stato.json       lo stato, scritto dal programma
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
  partita, seme, classi, ritardi, esito (etichetta colorata; «non valido» se l'orario non passa la verifica), tempo.
- **Elimina le prove scelte**: le caselle, una in cima per tutte; il tasto dice quante sono e chiede conferma. La
  prova che sta girando non si può scegliere.

Avviare una prova: si genera il caso (se non si riesce, un messaggio), si lancia il programma con 300 secondi, e si
passa alla pagina della prova. Il numero di una prova è il più alto esistente più uno.

Gli esiti che una prova può mostrare: quelli del programma («in corso», «trovato», «impossibile», «limite di
tempo»), più «fermata» (fermata a mano) e «interrotta» (il sito si è riavviato mentre girava, o il programma non è
partito, per esempio perché manca scipy).

## 11. La pagina di una prova (`/prova/N`)

Dall'alto in basso.

- «← Tutte le prove»; il titolo «Prova N: seme S, C classi, con/senza ritardi»; l'esito (etichetta) e il tempo.
- I tasti: **Ferma** (solo mentre gira), **Rifai** (una prova nuova sullo stesso caso), **Aggiungi ai notturni** (il
  caso entra tra i test notturni).
- Un messaggio quando un orario non c'è: «Un orario non esiste: il programma lo dimostra…» per «impossibile»; che
  non si sa, per «limite di tempo»; che cosa è successo, per «fermata» e «interrotta».
- Il riquadro della **verifica**, quando l'orario c'è: «verificato» o «non valido», e le nove regole con ✓ o ✗ e, per
  quelle non rispettate, dove.
- **L'orario**, con due tasti per scegliere come guardarlo (lo stesso orario; si parte da «Per classe»):
  - **Per classe**: una riga per classe. In una cella il docente e, sotto e più chiaro, il codocente; «–» per l'ora
    buca.
  - **Per docente**: una riga per docente. In una cella la classe dove insegna e, sotto e più chiaro, «con» e il
    nome dell'altro se sono in due; vuota se è libero; grigie le ore prima del suo ingresso.

  In tutti e due una colonna per ogni ora della settimana, raggruppate per giorno; la colonna dei nomi resta ferma
  mentre si scorre di lato; tutte le celle larghe uguali.
- **Il caso** (due sezioni chiuse): le classi con le loro ore e le loro lezioni; i docenti con le ore, l'ingresso (se
  tardi) e le classi.

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
| Lunghezza di un nome | al massimo 47 byte |
| Ore di un docente in una classe in un giorno | al massimo 3, di seguito |
| Classi di un docente in un giorno | al massimo 3 |
| Ore buche di un docente in un giorno | al massimo 2 |
| Casi casuali | 6 giorni × 6 ore, 5 obbligatorie; 30–36 ore per classe; lezioni da 1 a 6 ore; 5–24 ore per docente; ingresso al più alla quarta ora |
| Tentativi del generatore | 20.000 |
| Classi di un caso casuale | in pratica fino a circa 70; da 84 in su mai |
| Tempo di una prova | 300 secondi |
| Tempo di un caso dei test notturni | da 1 a 3600 secondi (in partenza 120) |
| Casi notturni aggiunti per volta | al massimo 2000 |
| Aggiornamento delle pagine | prova: ogni 1 s; notturni: ogni 2 s |

Il programma non ha un limite di «nodi»: l'unico limite è il tempo.

## 15. La prova automatica e le verifiche da rifare dopo una modifica

`python3 prove/prova.py` genera **54 casi** e ne cerca l'orario: semi da 0 a 5 × classi 3, 5, 8, 12 × senza e con
ritardi, più (10, 3, con), (37, 3, con), (4, 7, con), (1, 14, con), (2, 20, senza), (3, 30, con). Di ognuno confronta
con `prove/attesi.json` tre cose: l'impronta del caso generato (i primi 12 caratteri dello SHA-256 del testo:
se cambia, è cambiato il generatore), l'esito, e se l'orario passa la verifica. Oggi: 41 trovati, 13 impossibili. I
tempi non si confrontano. Con `--aggiorna` riscrive il file: solo dopo una modifica voluta.

Dopo ogni modifica al programma:
1. `prove/prova.py` deve dire «tutto come atteso».
2. Se si tocca `genera.py`: i casi non devono cambiare (le impronte in `attesi.json`), a meno che non sia voluto. Se
   è voluto, cambiano tutti i casi di tutti i semi, e i risultati dei test notturni non sono più confrontabili.
3. Se si tocca `esatto.py`: ogni orario trovato deve passare la verifica, e nessun caso deve passare da «trovato» a
   «impossibile» o il contrario (sono risposte certe: una delle due sarebbe sbagliata). Va rifatto su tanti casi,
   non solo sui 54.
4. Se si tocca `verifica.py`: deve continuare a riconoscere gli orari rovinati (scambiare due celle, mettere una
   buca, una voce di un'altra classe…), regola per regola.

## 16. Installazione e avvio

Servono Python 3 e i pacchetti di `sito/requirements.txt`: flask, waitress, scipy. Non serve un compilatore.

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
orario/esatto.py                 il metodo: il caso scritto come conti, e la sua soluzione
orario/verifica.py               la verifica dell'orario trovato
orario/__main__.py               i comandi
sito/__main__.py                 l'avvio del sito
sito/app.py                      le pagine e gli indirizzi
sito/prove.py                    le prove
sito/notturni.py                 i test notturni
sito/programma.py                generare un caso e lanciare il programma
sito/archivio.py                 la cartella di lavoro e i file JSON
sito/templates/                  base, indice, prova, notturni (HTML, con il loro JavaScript)
sito/static/stile.css            lo stile
prove/prova.py, attesi.json      la prova automatica
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
