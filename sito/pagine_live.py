"""Sezione "Test in tempo reale": un caso lanciato subito, seguito mentre gira (progresso e mappa dei blocchi)."""
import os

from flask import Blueprint, current_app, flash, g, jsonify, redirect, render_template, request, url_for

from . import accesso, esecutore
from .db import conn

bp = Blueprint('live', __name__, url_prefix='/live')


def _config():
    return current_app.config['SITO']


def _intero(nome, predefinito, minimo=None, massimo=None):
    try:
        v = int(request.form.get(nome, '') or predefinito)
    except ValueError:
        v = predefinito
    if minimo is not None:
        v = max(minimo, v)
    if massimo is not None:
        v = min(massimo, v)
    return v


@bp.route('/')
def indice():
    prove = conn().execute('''
        select p.*, e.id as eid, e.esito, e.marce, e.secondi, e.diagnosi
        from prove p left join esecuzioni e on e.prova_id = p.id
        where p.tipo = 'live' and p.scuola_id = %s order by p.id desc limit 50''', (g.scuola['id'],)).fetchall()
    n_classi = conn().execute('select count(*) as n from classi where scuola_id = %s', (g.scuola['id'],)).fetchone()['n']
    rilancio = None
    if request.args.get('rilancia'):
        e = conn().execute('select e.caso_dati, e.opzioni from esecuzioni e join prove p on p.id = e.prova_id '
                           'where e.id = %s and p.scuola_id = %s', (request.args['rilancia'], g.scuola['id'])).fetchone()
        rilancio = e
    return render_template('live/indice.html', prove=prove, n_classi=n_classi, rilancio=rilancio,
                           scuola=bool(request.args.get('scuola')))


@bp.route('/avvia', methods=['POST'])
def avvia():
    f = request.form
    if f.get('sorgente') == 'scuola':
        caso = {'tipo': 'scuola'}
    elif f.get('sorgente') == 'file':
        # solo file delle prove del sito (per esempio il risultato.json di un caso da rifare identico)
        file = os.path.realpath(f.get('file', ''))
        radice = os.path.realpath(_config().cartella_prove) + os.sep
        # solo file delle prove di questa scuola: la cartella di una prova si chiama come il suo id
        prima = file[len(radice):].split(os.sep)[0] if file.startswith(radice) else ''
        if not prima.isdigit() or not accesso.prova_della_scuola(int(prima), g.scuola['id']) or not os.path.exists(file):
            flash('File del caso non trovato.', 'errore')
            return redirect(url_for('.indice'))
        caso = {'tipo': 'input', 'file': file, 'nome': f.get('nome') or 'caso riletto'}
    else:
        caso = {'tipo': 'casuale', 'seme': _intero('seme', 1, 0), 'classi': _intero('classi', 8, 1, 120),
                'ritardi': bool(f.get('ritardi'))}
    opzioni = {'tempo': _intero('tempo', 60, 0, 24 * 3600), 'orari': _intero('orari', 1, 1, 1000),
               'motivi': bool(f.get('motivi')), 'backtrack': bool(f.get('backtrack')),
               'max_mb': _intero('max_mb', 200, 1, 100000)}
    # parametri della ricerca: nel JSON delle opzioni solo se diversi dai predefiniti (5, 3, 3)
    for chiave, predefinito, massimo in (('ore_obbligatorie', 5, 6), ('ore_prof_classe', 3, 6), ('classi_giorno', 3, 20)):
        v = _intero(chiave, predefinito, 1, massimo)
        if v != predefinito:
            opzioni[chiave] = v
    if f.get('ordine_classi', '').strip().isdigit():
        opzioni['ordine_classi'] = _intero('ordine_classi', 0, 0)
    if opzioni['tempo'] == 0:
        opzioni.pop('tempo')  # senza limite: si ferma col pulsante
    pid, eid = esecutore.avvia_live(_config(), g.scuola['id'], caso, opzioni)
    if eid is None:
        flash('La prova non e\' partita: guarda il messaggio nella pagina della prova.', 'errore')
    return redirect(url_for('.prova', pid=pid))


@bp.route('/<int:pid>')
def prova(pid):
    p = conn().execute('select * from prove where id = %s', (pid,)).fetchone()
    if not p:
        return redirect(url_for('.indice'))
    e = conn().execute('select * from esecuzioni where prova_id = %s order by id desc limit 1', (pid,)).fetchone()
    if e and e['esito'] is not None:
        return redirect(url_for('casi.caso', eid=e['id']))
    return render_template('live/prova.html', prova=p, esecuzione=e)


@bp.route('/<int:pid>/stato')
def stato(pid):
    """Per la pagina della prova (interrogata ogni secondo): progresso, e la fine."""
    p = conn().execute('select stato, messaggio from prove where id = %s', (pid,)).fetchone()
    e = conn().execute('select id, esito, cartella from esecuzioni where prova_id = %s order by id desc limit 1',
                       (pid,)).fetchone()
    r = {'stato': p['stato'] if p else None, 'messaggio': p['messaggio'] if p else None}
    if e:
        r.update({'eid': e['id'], 'esito': e['esito'], 'in_corso': esecutore.in_corso(e['id']),
                  'progresso': esecutore.progresso(e['id'])})
        if request.args.get('con_dati'):
            r['dati'] = esecutore.vista(e['cartella'])
    return jsonify(r)


@bp.route('/<int:pid>/ferma', methods=['POST'])
def ferma(pid):
    e = conn().execute('select id from esecuzioni where prova_id = %s order by id desc limit 1', (pid,)).fetchone()
    if e:
        esecutore.ferma_esecuzione(e['id'])
    return jsonify({'ok': True}) if request.headers.get('X-Gettone') else redirect(url_for('.prova', pid=pid))


@bp.route('/<int:pid>/elimina', methods=['POST'])
def elimina(pid):
    if esecutore.elimina_prova(_config(), pid):
        flash('Prova eliminata.', 'ok')
    else:
        flash('Una prova in corso non si puo\' eliminare: prima fermala.', 'errore')
    return redirect(url_for('.indice'))
