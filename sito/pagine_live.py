"""Sezione "Test in tempo reale": un test lanciato subito. La pagina del test (pagine_casi) segue la ricerca mentre gira
e mostra i suoi backtrack, con i controlli di ogni docente."""
from flask import Blueprint, current_app, flash, g, jsonify, redirect, render_template, request, url_for

from . import esecutore, modello, notturni, solver
from .db import conn

bp = Blueprint('live', __name__, url_prefix='/live')

FERMA_NODI = 50000000  # limite di nodi predefinito di un test
MAX_SERIE = 500  # test al massimo in una serie


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
                           serie=esecutore.serie(),
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
        # semi, classi e ordini come nei test notturni: un numero o un intervallo col trattino, anche piu' di uno
        try:
            semi = notturni.intervallo(f.get('semi', '') or '1', 0, 10 ** 9)
            classi = notturni.intervallo(f.get('classi', '') or '8', 1, 120)
            ordini = notturni.intervallo(f.get('ordini', '') or '0', 0, 10 ** 6)
        except ValueError:
            flash('Seme, classi e ordine delle classi: un numero o un intervallo col trattino, anche più di uno separati '
                  'da virgole, per esempio «1-6», «8-12» e «0-3» (classi da 1 a 120; ordine 0: come generato).', 'errore')
            return redirect(url_for('.indice'))
        ritardi = {'senza': [False], 'con': [True], 'entrambi': [False, True]}.get(f.get('ritardi'), [False])
        casi = [dict({'tipo': 'casuale', 'seme': s, 'classi': n, 'ritardi': r}, **({'ordine': k} if k else {}))
                for s in semi for n in classi for r in ritardi for k in ordini]
        if len(casi) > MAX_SERIE:
            flash('Troppi test in una volta: %d (al massimo %d).' % (len(casi), MAX_SERIE), 'errore')
            return redirect(url_for('.indice'))
        caso = casi[0] if len(casi) == 1 else None
    # niente limite di tempo: la ricerca si ferma dopo un numero di nodi (sempre allo stesso punto, qualunque sia il
    # computer), quando trova gli orari o prova tutto, oppure col pulsante
    opzioni = {'orari': _intero('orari', 1, 1, 1000),
               'max_backtrack': _intero('max_backtrack', 1000, 0, 100000)}
    ferma_nodi = _intero('ferma_nodi', FERMA_NODI, 0)
    if ferma_nodi:  # 0: senza limite, si ferma col pulsante
        opzioni['ferma_nodi'] = ferma_nodi
    disponibili = list(solver.solutori())
    if f.getlist('solutore') or sorgente != 'esecuzione':  # dal modulo, o «Rifai solo con»: quelli scelti
        solutori = [s for s in disponibili if s in f.getlist('solutore')]
    else:  # «Rifai il test con gli stessi dati»: tutti
        solutori = disponibili
    if not solutori:
        flash('Scegli almeno un solver.', 'errore')
        return redirect(url_for('.indice'))
    if caso is None:  # piu' combinazioni: una serie, un test dopo l'altro
        if esecutore.avvia_serie(_config(), g.scuola['id'], casi, opzioni, solutori):
            flash('Serie avviata: %d test, uno dopo l\'altro (ognuno con %d solver in parallelo).'
                  % (len(casi), len(solutori)), 'ok')
        else:
            flash('C\'è già una serie in corso: aspetta che finisca, o fermala.', 'errore')
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
    # quanti solver di questo test non sono ancora tra i test notturni
    da_aggiungere = [t for t in (notturni.da_esecuzione(conn(), g.scuola['id'], e['id']) for e in esecuzioni)
                     if t and not t['presente']]
    return render_template('live/prova.html', p=p, esecuzioni=esecuzioni, scheda=scheda,
                           descrizioni=solver.DESCRIZIONI, da_aggiungere=len(da_aggiungere))


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


@bp.route('/serie/ferma', methods=['POST'])
def ferma_serie():
    esecutore.ferma_serie()
    flash('Serie fermata: il test in corso si ferma, i successivi non partono.', 'ok')
    return redirect(url_for('.indice'))


@bp.route('/serie/stato')
def stato_serie():
    """Interrogato dalla pagina mentre una serie gira."""
    return jsonify(esecutore.serie())
