"""Il sito dell'orario: test in tempo reale con il solver di questo repository.

Una pagina con il modulo per avviare un test (seme, classi, ritardi, limite di nodi) e lo storico dei test; una pagina
per ogni test, che mostra la ricerca mentre gira. Niente login e niente database: ogni test e' una cartella in
SITO_LAVORO (di default sito/lavoro) con info.json (i parametri) e stato.json (scritto dal solver).
"""
