"""Il sito dell'orario: mostra il lavoro del solver di questo repository (la cartella solver/).

Due parti. Le prove in tempo reale: si sceglie un caso (seme, classi, ritardi, limite di nodi), si guarda la ricerca
mentre va e la si rivede passo per passo. I test notturni: un elenco di casi rieseguiti tutti insieme, per vedere che
cosa cambia quando cambia il solver.

Niente login e niente database: tutto sta in file JSON nella cartella di lavoro (SITO_LAVORO, di default sito/lavoro).

I file:
- __main__.py   l'avvio (python3 -m sito avvia)
- app.py        le pagine e i loro indirizzi
- prove.py      le prove in tempo reale
- passi.py      i passi di una ricerca, letti dal file che scrive il solver
- notturni.py   i test notturni
- solver.py     compilare e lanciare il solver
- archivio.py   la cartella di lavoro e i file JSON
- templates/    le pagine (HTML, con il loro JavaScript in fondo); static/stile.css lo stile
"""
