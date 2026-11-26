"""Il metodo esatto: l'orario cercato scrivendo tutto il caso come un sistema di conti con incognite 0/1, risolto da un
risolutore generico (HiGHS, dentro scipy).

E' un altro modo di fare l'orario, accanto alla ricerca di solver/: non riempie le celle una alla volta tornando
indietro, ma guarda tutte le regole di tutta la settimana insieme. Dice anche quando un orario non esiste.

- modello.py   il caso scritto come conti, e la sua soluzione
- __main__.py  il comando: python3 -m esatto CASO --programma ORARIO [--stato FILE] [--tempo SECONDI]

La scuola (con i docenti e le voci numerati) e la verifica dell'orario trovato le chiede al programma in C (i comandi
«scuola» e «verifica» di programma/main.c): cosi' parla con gli stessi numeri, e l'orario e' ricontrollato da un
programma che con questo metodo non ha niente a che fare.
"""
