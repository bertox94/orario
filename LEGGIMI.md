# Orario scolastico

Il programma dell'orario e il suo sito. Dal 26 novembre 2026 è tutto in Python: la ricerca in C che c'era prima è nel
ramo `stabile/2026-11-26`, intatta.

La descrizione completa, con il dettaglio per rifare tutto da zero (regole, generatore, metodo, verifica, comandi,
ogni pagina del sito, i limiti), è in `DESCRIZIONE.md`.

## Che cosa c'è

- `orario/`: il programma.
  - `scuola.py`: i dati (giorni, ore, classi, docenti, voci), le regole e la lettura di un caso da file;
  - `genera.py`, `nomi.py`: i casi casuali, riproducibili da un seme;
  - `esatto.py`: il primo metodo. Tutto il caso scritto come conti con risposte sì/no e dato a un risolutore
    generico (HiGHS, dentro scipy): dà l'orario, oppure dimostra che non esiste;
  - `cpsat.py`: il secondo metodo, lo stesso caso per CP-SAT (OR-Tools);
  - `insieme.py`: i due metodi insieme: basta che uno risponda, e di ognuno si vede esito e tempo;
  - `verifica.py`: l'orario trovato, ricontrollato da capo senza usare il metodo;
  - `__main__.py`: i comandi.
- `sito/`: il sito (Flask), con le prove e i test notturni. Non calcola niente: lancia il programma e mostra.
- `prove/prova.py`: la prova automatica.

## Usarlo a mano

```
pip install -r sito/requirements.txt
python3 -m orario genera 10 3 1 > caso.txt     # seme 10, 3 classi, con ritardi
python3 -m orario risolvi caso.txt             # stampa lo stato, in JSON
```

Un caso è un file di testo, una riga per cosa, con i campi separati da TAB:

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

`obbligatorie`: le prime ore di ogni giorno, che hanno sempre lezione. `docente`: il nome, l'ora (da 0) da cui è a
scuola e, se servono, l'ora da cui non c'è più e il giorno libero (un giorno, o `qualunque`: ogni docente ne ha uno). `materia` e `aula`: il nome e il tipo di aula (una materia si fa nelle aule del suo tipo). `lezione`: la classe,
le ore in una settimana, la materia, il docente e, se sono in due, il codocente. Più righe con la stessa classe, la
stessa materia e gli stessi docenti si sommano.

## Le regole dell'orario

1. **Le ore.** Ogni voce fa esattamente le sue ore.
2. **Una classe per volta.** Un docente è in una classe per volta.
3. **A scuola.** Un docente insegna solo quando è a scuola: dopo il suo ingresso, prima della sua uscita, non nel
   suo giorno libero. Ogni docente ne ha uno: un giorno preciso, o uno qualunque scelto dal programma.
4. **In una classe.** Un docente fa al massimo 3 ore al giorno nella stessa classe, e le fa di seguito.
5. **Le classi.** Un docente va al massimo in 3 classi al giorno.
6. **La giornata del docente.** Al massimo 5 ore di lezione al giorno. Ore buche (tra la sua prima e la sua ultima
   lezione del giorno): al massimo 2.
7. **La giornata della classe.** Le prime ore di ogni giorno hanno lezione, e dopo un'ora buca la classe ha solo ore
   buche.
8. **Le aule.** Ogni lezione è in un'aula del tipo che la sua materia chiede, e in un'aula c'è una classe per volta.

## Il sito

```
python3 -m sito avvia --porta 8000                  # solo su questo computer
python3 -m sito avvia --host 0.0.0.0 --porta 8000   # visibile dalla rete locale (Raspberry)
```

- **Prove**: seme, classi, ritardi, «Fai l'orario». La pagina della prova mostra l'esito (trovato, impossibile,
  limite di tempo), il tempo, la verifica regola per regola, l'orario e il caso.
- **Scuole**: i dati di una scuola vera, scritti a mano (docenti con entrata, uscita e giorno libero; lezioni;
  materie; aule; la settimana), e «Fai l'orario».
- **Test notturni**: un elenco di casi rifatti tutti insieme, a mano o ogni notte; mostra che cosa è cambiato
  rispetto al giro prima.

I dati del sito sono file in `sito/lavoro/` (o nella cartella della variabile `SITO_LAVORO`).

## La prova automatica

`python3 prove/prova.py`: 54 casi, di cui confronta il caso generato, l'esito e la verifica con `prove/attesi.json`.
Deve finire con «tutto come atteso».
