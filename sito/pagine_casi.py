"""Dettaglio di un'esecuzione (un caso in una fase di una prova): esito, diagnosi, orario, mappa dei blocchi, file."""
import os

from flask import Blueprint, abort, render_template, send_from_directory

from .db import conn

bp = Blueprint('casi', __name__, url_prefix='/casi')

# i file di un'esecuzione che si possono scaricare
FILE = [('risultato.json', 'risultati in JSON (dati, orario, diagnosi)'),
        ('dati.txt', 'descrizione dei dati'),
        ('uscita.txt', 'uscita completa del solver'),
        ('orario.txt', 'orari trovati (testo)'),
        ('orario.txt.gz', 'marce indietro in formato compatto (gzip, vedi printer/printer.h)')]


@bp.route('/<int:eid>')
def caso(eid):
    e = conn().execute('select e.*, p.tipo, p.nome as prova_nome, p.commit from esecuzioni e '
                       'join prove p on p.id = e.prova_id where e.id = %s', (eid,)).fetchone()
    if not e:
        abort(404)
    fasi = conn().execute('select id, fase, esito, secondi from esecuzioni where prova_id = %s and caso = %s '
                          'order by fase', (e['prova_id'], e['caso'])).fetchall()
    file = [(n, d, os.path.getsize(os.path.join(e['cartella'], n)))
            for n, d in FILE if e['cartella'] and os.path.exists(os.path.join(e['cartella'], n))]
    r = e['risultato'] or {}
    return render_template('casi/caso.html', e=e, fasi=fasi, file=file, r=r)


@bp.route('/<int:eid>/file/<nome>')
def scarica(eid, nome):
    if nome not in dict(FILE):
        abort(404)
    e = conn().execute('select cartella from esecuzioni where id = %s', (eid,)).fetchone()
    if not e or not e['cartella']:
        abort(404)
    return send_from_directory(e['cartella'], nome, as_attachment=nome.endswith(('.gz', '.json')))
