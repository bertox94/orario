"""Sezione "Test in tempo reale": un test lanciato subito. La pagina del test (pagine_casi) segue la ricerca mentre gira
e mostra i suoi backtrack, con i controlli di ogni docente."""
from flask import Blueprint, current_app, flash, g, jsonify, redirect, render_template, request, url_for

from . import esecutore, modello, solver
from .db import conn

bp = Blueprint('live', __name__, url_prefix='/live')

FERMA_NODI = 50000000  # limite di nodi predefinito di un test


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
    select e.id, e.prova_id, e.caso, coalesce(e.opzioni->>'solutore', 'main') as solutore, e.esito, e.marce, e.diagnosi, coalesce(e.fine, e.inizio) as quando
    from esecuzioni e join prove p on p.id = e.prova_id
    where p.tipo = 'live' and p.scuola_id = %s and e.esito is not null
      and e.esito not in ('trovato', 'nessuna soluzione') and not e.verificato
    order by e.id desc'''


@bp.route('/')
def indice():
    righe = conn().execute('''
        select p.*, e.id as eid, coalesce(e.opzioni->>'solutore', 'main') as solutore, e.esito, e.marce, e.secondi,
            e.diagnosi
        from (select * from prove where tipo = 'live' and scuola_id = %s order by id desc limit 60) p
        left join esecuzioni e on e.prova_id = p.id order by p.id desc, e.id''', (g.scuola['id'],)).fetchall()
    # una riga per test; un test con piu' solver si apre con una riga per solver
    prove = []
    for r in righe:
        if not prove or prove[-1]['id'] != r['id']:
            prove.append(dict(r, solver=[]))
        if r['eid']:
            prove[-1]['solver'].append(r)
    n_classi = conn().execute('select count(*) as n from classi where scuola_id = %s', (g.scuola['id'],)).fetchone()['n']
    da_verificare = conn().execute(_DA_VERIFICARE + ' limit 40', (g.scuola['id'],)).fetchall()
    return render_template('live/indice.html', prove=prove, n_classi=n_classi, da_verificare=da_verificare,
                           solutori=[(n, solver.DESCRIZIONI.get(n, n)) for n in solver.solutori()], ferma_nodi=FERMA_NODI,
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
    # niente limite di tempo: la ricerca si ferma dopo un numero di nodi (sempre allo stesso punto, qualunque sia il
    # computer), quando trova gli orari o prova tutto, oppure col pulsante
    opzioni = {'orari': _intero('orari', 1, 1, 1000),
               'max_backtrack': _intero('max_backtrack', 1000, 0, 100000)}
    ferma_nodi = _intero('ferma_nodi', FERMA_NODI, 0)
    if ferma_nodi:  # 0: senza limite, si ferma col pulsante
        opzioni['ferma_nodi'] = ferma_nodi
    disponibili = list(solver.solutori())
    if f.getlist('solutore'):  # «Rifai il test»: gli stessi solver (dalla pagina di un solver: solo quello)
        solutori = [s for s in disponibili if s in f.getlist('solutore')]
    else:  # dal modulo: tutti tranne quelli esclusi
        esclusi = set(f.getlist('escludi'))
        solutori = [s for s in disponibili if s not in esclusi]
    if not solutori:
        flash('Hai escluso tutti i solver: lasciane almeno uno.', 'errore')
        return redirect(url_for('.indice'))
    try:
        pid, eids = esecutore.avvia_live(_config(), g.scuola['id'], caso, opzioni, solutori)
    except ValueError as ex:  # per esempio il generatore non trova dati validi
        flash(str(ex), 'errore')
        return redirect(url_for('.indice'))
    return redirect(url_for('.prova', pid=pid))


@bp.route('/<int:pid>')
def prova(pid):
    """Un test: con un solo solver la sua pagina; con piu' solver una tab per ognuno (?scheda=id dell'esecuzione da
    aprire), ognuna con la pagina del suo solver, che segue la ricerca mentre gira: le ricerche vanno in parallelo."""
    p = conn().execute('select * from prove where id = %s and scuola_id = %s', (pid, g.scuola['id'])).fetchone()
    esecuzioni = conn().execute("select id, esito, coalesce(opzioni->>'solutore', 'main') as solutore "
                                "from esecuzioni where prova_id = %s order by id", (pid,)).fetchall()
    if not p or not esecuzioni:
        return redirect(url_for('.indice'))
    if len(esecuzioni) == 1:
        return redirect(url_for('casi.caso', eid=esecuzioni[0]['id']))
    scheda = request.args.get('scheda', type=int)
    if scheda not in [e['id'] for e in esecuzioni]:
        scheda = esecuzioni[0]['id']
    return render_template('live/prova.html', p=p, esecuzioni=esecuzioni, scheda=scheda,
                           descrizioni=solver.DESCRIZIONI)


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
