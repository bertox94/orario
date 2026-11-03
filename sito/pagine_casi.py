"""Dettaglio di un'esecuzione (un caso in una fase di una prova): esito, diagnosi, orario, mappa dei blocchi, file.
I file stanno nel database (vedi archivio.py)."""
import json

from flask import Blueprint, Response, abort, current_app, render_template

from . import archivio
from .db import conn

bp = Blueprint('casi', __name__, url_prefix='/casi')

# i file di un'esecuzione che si possono scaricare
FILE = [('risultato.json', 'risultati in JSON (dati, orario, diagnosi)'),
        ('dati.txt', 'descrizione dei dati'),
        ('uscita.txt', 'uscita completa del solver'),
        ('orario.txt', 'orari trovati (testo)'),
        ('orario.txt.gz', 'marce indietro in formato compatto (gzip, vedi printer/printer.h)')]


def _risultato_json(r):
    return json.dumps(r, ensure_ascii=False, indent=1).encode()


@bp.route('/<int:eid>')
def caso(eid):
    e = conn().execute('select e.*, p.tipo, p.nome as prova_nome, p.commit from esecuzioni e '
                       'join prove p on p.id = e.prova_id where e.id = %s', (eid,)).fetchone()
    if not e:
        abort(404)
    fasi = conn().execute('select id, fase, esito, secondi from esecuzioni where prova_id = %s and caso = %s '
                          'order by fase', (e['prova_id'], e['caso'])).fetchall()
    dim = archivio.dimensioni(conn(), eid)
    if e['risultato']:
        dim['risultato.json'] = len(_risultato_json(e['risultato']))
    file = [(n, d, dim[n]) for n, d in FILE if n in dim]
    r = e['risultato'] or {}
    return render_template('casi/caso.html', e=e, fasi=fasi, file=file, r=r)


@bp.route('/<int:eid>/file/<nome>')
def scarica(eid, nome):
    if nome not in dict(FILE):
        abort(404)
    intestazioni = {}
    if nome.endswith(('.gz', '.json')):
        intestazioni['Content-Disposition'] = 'attachment; filename="%d-%s"' % (eid, nome)
    if nome == 'risultato.json':
        e = conn().execute('select risultato from esecuzioni where id = %s', (eid,)).fetchone()
        if not e or not e['risultato']:
            abort(404)
        return Response(_risultato_json(e['risultato']), mimetype='application/json', headers=intestazioni)
    dim = archivio.dimensioni(conn(), eid).get(nome)
    if dim is None:
        abort(404)
    intestazioni['Content-Length'] = str(dim)
    tipo = 'application/gzip' if nome.endswith('.gz') else 'text/plain; charset=utf-8'
    # il flusso legge dal database un pezzo alla volta, con la sua connessione: non tiene in memoria il file
    return Response(archivio.pezzi(current_app.config['SITO'].database_url, eid, nome), mimetype=tipo,
                    headers=intestazioni)
