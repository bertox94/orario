"""Il programma dell'orario scolastico.

Dato l'elenco delle lezioni di una scuola (quale docente, o quale coppia di docenti, fa quante ore in quale classe)
trova un orario settimanale che rispetta le regole, oppure dimostra che non esiste.

- scuola.py    la scuola, le regole dell'orario, e la lettura di un caso da file
- genera.py    i casi casuali, riproducibili da un seme (nomi.py: i nomi dei docenti)
- esatto.py    il metodo: tutto il caso scritto come conti con risposte si'/no, risolti da HiGHS (scipy)
- verifica.py  l'orario trovato ricontrollato da capo, regola per regola, senza usare niente del metodo
- __main__.py  i comandi: python3 -m orario genera | risolvi | verifica

Tutto in Python (dal 26 novembre 2026, richiesta di Alb): il lavoro pesante lo fa il risolutore, che e' compilato;
il resto sono conti piccoli. La ricerca in C che c'era prima (un'ora alla volta, tornando indietro, con i suoi
controlli e i passi da rivedere) e' nel ramo stabile/2026-11-26.
"""
