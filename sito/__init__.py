"""Il sito dell'orario: fa girare il programma (la cartella orario/) e ne mostra i risultati.

Tre parti. Le prove: si sceglie un caso (seme, classi, ritardi) e si guarda il suo orario, con la verifica, oppure
che un orario non esiste. Le scuole: i dati di una scuola vera, scritti a mano, e il suo orario. I test notturni: un elenco di casi rifatti tutti insieme, per vedere che cosa cambia quando
cambia il programma.

Niente login e niente database: tutto sta in file JSON nella cartella di lavoro (SITO_LAVORO, di default sito/lavoro).

I file:
- __main__.py   l'avvio (python3 -m sito avvia)
- app.py        le pagine e i loro indirizzi
- prove.py      le prove
- scuole.py     le scuole scritte a mano: i dati di una scuola vera, da cui far fare l'orario
- notturni.py   i test notturni
- programma.py  generare un caso e lanciare il programma
- archivio.py   la cartella di lavoro e i file JSON
- templates/    le pagine (HTML, con il loro JavaScript in fondo); static/stile.css lo stile
"""
