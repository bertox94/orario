"""Sezione "Test in tempo reale": un test lanciato subito. La pagina del test (pagine_casi) segue la ricerca mentre gira
e mostra i suoi backtrack, con i controlli di ogni docente."""
from flask import Blueprint, current_app, flash, g, jsonify, redirect, render_template, request, url_for

from . import esecutore, modello
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


# I test che non sono arrivati a una conclusione (fermati, finiti per tempo, in errore...) e non ancora segnati come
# verificati: da rivedere.
_DA_VERIFICARE = '''
    select e.id, e.prova_id, e.caso, e.esito, e.marce, e.diagnosi, coalesce(e.fine, e.inizio) as quando
    from esecuzioni e join prove p on p.id = e.prova_id
    where p.tipo = 'live' and p.scuola_id = %s and e.esito is not null
      and e.esito not in ('trovato', 'nessuna soluzione') and not e.verificato
    order by e.id desc'''


@bp.route('/')
def indice():
    prove = conn().execute('''
        select p.*, e.id as eid, e.esito, e.marce, e.secondi, e.diagnosi
        from prove p left join esecuzioni e on e.prova_id = p.id
        where p.tipo = 'live' and p.scuola_id = %s order by p.id desc limit 50''', (g.scuola['id'],)).fetchall()
    n_classi = conn().execute('select count(*) as n from classi where scuola_id = %s', (g.scuola['id'],)).fetchone()['n']
    da_verificare = conn().execute(_DA_VERIFICARE + ' limit 40', (g.scuola['id'],)).fetchall()
    return render_template('live/indice.html', prove=prove, n_classi=n_classi, da_verificare=da_verificare,
                           preseleziona_scuola=bool(request.args.get('scuola')))


@bp.route('/avvia', methods=['POST'])
def avvia():
    f = request.form
    sorgente = f.get('sorgente')
    if sorgente == 'scuola':
        dati = modello.leggi(conn(), g.scuola['id'])
        if not dati['lezioni']:
            flash('La scuola non ha ancora lezioni: inseriscile nei dati della scuola.', 'errore')
            return redirect(url_for('.indice'))
        caso = {'tipo': 'scuola'}
    elif sorgente == 'file':
        file = request.files.get('file')
        if not (file and file.filename):
            flash('Scegli il file JSON da caricare.', 'errore')
            return redirect(url_for('.indice'))
        dati, nome, errore = esecutore.dati_da_file_caricato(_config(), file)
        if errore:
            flash(errore, 'errore')
            return redirect(url_for('.indice'))
        caso = {'tipo': 'input', 'nome': nome, 'dati': dati}
    elif sorgente == 'esecuzione':
        # rifa' un test con gli stessi dati (salvati nella prova)
        p = conn().execute('select p.dati_scuola, p.nome from prove p join esecuzioni e on e.prova_id = p.id '
                           'where e.id = %s and p.scuola_id = %s', (_intero('eid', 0), g.scuola['id'])).fetchone()
        if not p or not p['dati_scuola']:
            flash('Dati del test non trovati.', 'errore')
            return redirect(url_for('.indice'))
        caso = {'tipo': 'input', 'nome': p['nome'], 'dati': p['dati_scuola']}
    else:
        caso = {'tipo': 'casuale', 'seme': _intero('seme', 1, 0), 'classi': _intero('classi', 8, 1, 120),
                'ritardi': bool(f.get('ritardi'))}
    opzioni = {'tempo': _intero('tempo', 60, 0, 24 * 3600), 'orari': _intero('orari', 1, 1, 1000),
               'max_backtrack': _intero('max_backtrack', 1000, 0, 100000)}
    if opzioni['tempo'] == 0:
        opzioni.pop('tempo')  # senza limite: si ferma col pulsante
    try:
        pid, eid = esecutore.avvia_live(_config(), g.scuola['id'], caso, opzioni)
    except ValueError as ex:  # per esempio il generatore non trova dati validi
        flash(str(ex), 'errore')
        return redirect(url_for('.indice'))
    return redirect(url_for('casi.caso', eid=eid))


@bp.route('/<int:pid>')
def prova(pid):
    """Le pagine vecchie dei test portano alla pagina del test."""
    e = conn().execute('select id from esecuzioni where prova_id = %s order by id desc limit 1', (pid,)).fetchone()
    return redirect(url_for('casi.caso', eid=e['id']) if e else url_for('.indice'))


@bp.route('/verificato/<int:eid>', methods=['POST'])
def verificato(eid):
    """Toglie un test dall'elenco «da verificare» (il test resta)."""
    conn().execute('update esecuzioni set verificato = true where id = %s', (eid,))
    return jsonify({'ok': True}) if request.headers.get('X-Gettone') else redirect(url_for('.indice'))


@bp.route('/verificato-tutti', methods=['POST'])
def verificato_tutti():
    conn().execute('update esecuzioni set verificato = true where id in (select id from (' + _DA_VERIFICARE + ') t)',
                   (g.scuola['id'],))
    return jsonify({'ok': True}) if request.headers.get('X-Gettone') else redirect(url_for('.indice'))


@bp.route('/<int:pid>/elimina', methods=['POST'])
def elimina(pid):
    ok = esecutore.elimina_prova(_config(), pid)
    if request.headers.get('X-Gettone'):  # dalla pagina, senza ricaricarla
        return jsonify({'ok': ok, 'errore': None if ok else 'Un test in corso non si può eliminare: prima fermalo.'})
    if ok:
        flash('Test eliminato.', 'ok')
    else:
        flash('Un test in corso non si puo\' eliminare: prima fermalo.', 'errore')
    return redirect(url_for('.indice'))
