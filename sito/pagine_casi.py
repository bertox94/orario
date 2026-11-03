"""Dettaglio di un'esecuzione (un caso in una fase di una prova): esito, diagnosi, orario, mappa dei blocchi, file.
I file stanno nel database (vedi archivio.py)."""
import json

from flask import Blueprint, Response, abort, current_app, jsonify, render_template, request

from . import archivio, marce
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


@bp.route('/<int:eid>/marce')
def marce_indietro(eid):
    """Una marcia indietro registrata con -b (?n=, da 1): l'orario parziale in quel momento e lo slot dove si e' bloccato."""
    e = conn().execute('select risultato, esito from esecuzioni where id = %s', (eid,)).fetchone()
    if not e or 'orario.txt.gz' not in archivio.dimensioni(conn(), eid):
        abort(404)
    url = current_app.config['SITO'].database_url
    nomi = [cl['nome'] for cl in ((e['risultato'] or {}).get('dati') or {}).get('classi', [])]
    try:
        if request.args.get('totale'):
            return jsonify({'totale': marce.totale(url, eid, e['esito'] is not None)})
        m = marce.marcia(url, eid, max(1, request.args.get('n', 1, type=int)), nomi)
    except marce.FileTroppoGrande:
        return jsonify({'errore': 'Il file delle marce indietro e\' troppo grande per essere sfogliato qui: scaricalo.'})
    return jsonify(m or {'errore': 'Nessuna marcia indietro registrata.'})
