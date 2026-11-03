"""Accesso a Postgres e migrazioni.

Le migrazioni sono i file sito/migrazioni/NNN_nome.sql, applicati in ordine una volta sola (la tabella
migrazioni_applicate ricorda quali). Per cambiare il database si aggiunge un file nuovo, non si modificano i vecchi:
cosi' il database sul Pi si aggiorna al prossimo avvio senza perdere i dati.
"""
import os

import psycopg
from flask import current_app, g
from psycopg.rows import dict_row

CARTELLA_MIGRAZIONI = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'migrazioni')


def connetti(url):
    return psycopg.connect(url, row_factory=dict_row, autocommit=False)


def conn():
    """La connessione della richiesta corrente (una per richiesta, chiusa alla fine)."""
    if 'db' not in g:
        g.db = connetti(current_app.config['SITO'].database_url)
    return g.db


def chiudi(_eccezione=None):
    c = g.pop('db', None)
    if c is not None:
        if _eccezione is None:
            c.commit()
        else:
            c.rollback()
        c.close()


def migra(url):
    """Applica le migrazioni mancanti. Ritorna i nomi di quelle applicate."""
    applicate = []
    with connetti(url) as c:
        c.execute('create table if not exists migrazioni_applicate (nome text primary key, '
                  'quando timestamptz not null default now())')
        fatte = {r['nome'] for r in c.execute('select nome from migrazioni_applicate')}
        for nome in sorted(os.listdir(CARTELLA_MIGRAZIONI)):
            if not nome.endswith('.sql') or nome in fatte:
                continue
            c.execute(open(os.path.join(CARTELLA_MIGRAZIONI, nome)).read())
            c.execute('insert into migrazioni_applicate (nome) values (%s)', (nome,))
            applicate.append(nome)
        c.commit()
    return applicate
