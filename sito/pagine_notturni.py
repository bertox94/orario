"""Sezione "Test notturni": i test che ogni notte devono dare gli stessi numeri (vedi notturni.py).

La pagina mostra, per ogni test, l'ultima esecuzione (data e ora, tempo, nodi, backtrack, posizione massima, risultato)
e il confronto con il riferimento accettato; le differenze sono in evidenza, con il pulsante per accettarle. Ogni test
si puo' rifare in tempo reale (stessi dati, stesso solver, stesso limite di nodi: deve dare gli stessi numeri) o
scaricare come JSON per caricarlo nel modulo del test in tempo reale.
"""
import json
import re

from flask import Blueprint, Response, abort, current_app, flash, g, jsonify, redirect, render_template, request, url_for

from . import esecutore, notturni, solver
from .db import conn

bp = Blueprint('notturni', __name__, url_prefix='/notturni')


def _config():
    return current_app.config['SITO']


def _test(tid):
    t = conn().execute('select * from test_notturni where id = %s and scuola_id = %s', (tid, g.scuola['id'])).fetchone()
    if not t:
        abort(404)
    return t


def _intervallo(testo, minimo, massimo):
    """"1-6, 9" -> [1, 2, 3, 4, 5, 6, 9] (ValueError se non si legge)."""
    valori = []
    for parte in re.split(r'[,\s]+', testo.strip()):
        if not parte:
            continue
        a, _, b = parte.partition('-')
        a, b = int(a), int(b or a)
        if not minimo <= a <= b <= massimo:
            raise ValueError(parte)
        valori.extend(range(a, b + 1))
    if not valori:
        raise ValueError('vuoto')
    return sorted(set(valori))


def celle(dati):
    """Le celle della ricerca (giorni x ore x classi): la posizione massima si legge su questo totale."""
    try:
        n = lambda v: v if isinstance(v, int) else len(v)  # noqa: E731
        return n(dati['giorni']) * n(dati['ore']) * len({l['classe'] for l in dati['lezioni']})
    except (KeyError, TypeError):
        return None


_ULTIME = '''
    select t.id, t.nome, t.solutore, t.opzioni, t.attivo, t.riferimento, t.accettato, t.accettato_da,
           (select count(*) from esecuzioni_notturne x where x.test_id = t.id) as quante,
           n.id as nid, n.inizio, n.secondi, n.esito, n.nodi, n.backtrack, n.pos_max, n.orario, n.errore, n.confronto,
           n.differenze, n.giro_id
    from test_notturni t
    left join lateral (select * from esecuzioni_notturne x where x.test_id = t.id order by x.id desc limit 1) n on true
    where t.scuola_id = %s
    order by t.attivo desc, t.nome, t.solutore, t.id'''


@bp.route('/')
def indice():
    righe = conn().execute(_ULTIME, (g.scuola['id'],)).fetchall()
    totali = {t['id']: celle(t['dati']) for t in conn().execute(
        'select id, dati from test_notturni where scuola_id = %s', (g.scuola['id'],))}
    giri = conn().execute('''
        select gi.*, count(n.id) as quanti,
               count(*) filter (where n.confronto in ('diverso', 'accettato')) as diversi,
               count(*) filter (where n.confronto = 'uguale') as uguali,
               count(*) filter (where n.confronto = 'nuovo') as nuovi,
               count(*) filter (where n.confronto = 'errore') as errori,
               sum(n.secondi) as secondi
        from giri_notturni gi left join esecuzioni_notturne n on n.giro_id = gi.id
        where gi.scuola_id = %s group by gi.id order by gi.id desc limit 15''', (g.scuola['id'],)).fetchall()
    diversi = [r for r in righe if r['confronto'] == 'diverso']
    return render_template('notturni/indice.html', righe=righe, totali=totali, giri=giri, diversi=diversi,
                           in_corso=notturni.giro_in_corso(), test_in_corso=notturni.test_in_corso(),
                           ora=notturni.ora_impostata(conn()), nomi_campi=notturni.NOMI_CAMPI,
                           solutori=[(n, solver.DESCRIZIONI.get(n, n)) for n in solver.solutori()],
                           ferma_nodi=notturni.FERMA_NODI)


@bp.route('/stato')
def stato():
    """Per la pagina, mentre un giro e' in corso: si ricarica quando cambia il test in corso."""
    return jsonify({'giro': notturni.giro_in_corso(), 'test': notturni.test_in_corso()})


@bp.route('/genera', methods=['POST'])
def genera():
    f = request.form
    try:
        semi = _intervallo(f.get('semi', ''), 0, 10 ** 9)
        classi = _intervallo(f.get('classi', ''), 1, 120)
    except ValueError:
        flash('Semi e classi: numeri o intervalli separati da virgole, per esempio «1-6» e «8, 12, 16» (classi da 1 a '
              '120).', 'errore')
        return redirect(url_for('.indice'))
    ritardi = {'senza': [False], 'con': [True], 'entrambi': [False, True]}.get(f.get('ritardi'), [False])
    solutori = [s for s in solver.solutori() if s in f.getlist('solutore')]
    if not solutori:
        flash('Scegli almeno un solver.', 'errore')
        return redirect(url_for('.indice'))
    if len(semi) * len(classi) * len(ritardi) * len(solutori) > 500:
        flash('Troppi test in una volta (al massimo 500).', 'errore')
        return redirect(url_for('.indice'))
    try:
        ferma_nodi = max(1, int(f.get('ferma_nodi') or notturni.FERMA_NODI))
        orari = min(1000, max(1, int(f.get('orari') or 1)))
    except ValueError:
        flash('Limite di nodi e orari: numeri interi.', 'errore')
        return redirect(url_for('.indice'))
    aggiunti, gia, problemi = notturni.genera(conn(), g.scuola['id'], semi, classi, ritardi, solutori, ferma_nodi,
                                              orari, casi_repository=bool(f.get('repository')))
    flash('Test aggiunti: %d%s.' % (aggiunti, ' (%d c\'erano gia\')' % gia if gia else ''), 'ok')
    for p in problemi[:10]:
        flash('Non generato: %s' % p, 'errore')
    return redirect(url_for('.indice'))


@bp.route('/da-test/<int:eid>', methods=['POST'])
def da_test(eid):
    """Un test in tempo reale diventa un test notturno: stessi dati e stesso solver (limite di nodi dal modulo)."""
    e = conn().execute('select e.caso, e.opzioni, p.dati_scuola, p.opzioni as opzioni_prova from esecuzioni e '
                       'join prove p on p.id = e.prova_id where e.id = %s and p.scuola_id = %s',
                       (eid, g.scuola['id'])).fetchone()
    if not e or not e['dati_scuola']:
        abort(404)
    o = e['opzioni'] or {}
    try:
        ferma_nodi = max(1, int(request.form.get('ferma_nodi') or o.get('ferma_nodi') or notturni.FERMA_NODI))
    except ValueError:
        ferma_nodi = notturni.FERMA_NODI
    caso = dict((e['opzioni_prova'] or {}).get('caso') or {}, da_test=eid)
    nuovo = notturni.aggiungi(conn(), g.scuola['id'], e['caso'], o.get('solutore', 'main'), caso, e['dati_scuola'],
                              {'orari': o.get('orari', 1), 'ferma_nodi': ferma_nodi})
    flash('Aggiunto ai test notturni.' if nuovo else 'Questo test c\'era gia\' tra i test notturni.', 'ok')
    return redirect(url_for('.indice'))


@bp.route('/esegui', methods=['POST'])
def esegui():
    """Un giro adesso: tutti i test attivi, o solo quello indicato (tid) o solo quelli diversi (diversi=1)."""
    soli = None
    if request.form.get('tid'):
        soli = [_test(int(request.form['tid']))['id']]
    elif request.form.get('diversi'):
        soli = [r['id'] for r in conn().execute(_ULTIME, (g.scuola['id'],)) if r['confronto'] == 'diverso']
    gid = notturni.avvia_giro(_config(), g.scuola['id'], 'a mano', soli)
    if gid is None:
        flash('C\'e\' gia\' un giro in corso.' if notturni.giro_in_corso() else 'Nessun test da eseguire.', 'errore')
    return redirect(url_for('.indice'))


@bp.route('/ferma', methods=['POST'])
def ferma():
    notturni.ferma_giro()
    return redirect(url_for('.indice'))


@bp.route('/accetta/<int:nid>', methods=['POST'])
def accetta(nid):
    n = conn().execute('select n.id from esecuzioni_notturne n join test_notturni t on t.id = n.test_id '
                       'where n.id = %s and t.scuola_id = %s', (nid, g.scuola['id'])).fetchone()
    if not n:
        abort(404)
    notturni.accetta(conn(), nid, g.utente['nome'])
    if request.headers.get('X-Gettone'):
        return jsonify({'ok': True})
    return redirect(request.form.get('poi') or url_for('.indice'))


@bp.route('/accetta-tutti', methods=['POST'])
def accetta_tutti():
    """Accetta tutte le ultime esecuzioni diverse (un cambiamento voluto che tocca molti test)."""
    quanti = sum(notturni.accetta(conn(), r['nid'], g.utente['nome'])
                 for r in conn().execute(_ULTIME, (g.scuola['id'],)).fetchall() if r['confronto'] == 'diverso')
    flash('Accettati %d cambiamenti: i numeri nuovi sono il riferimento.' % quanti, 'ok')
    return redirect(url_for('.indice'))


@bp.route('/ora', methods=['POST'])
def ora():
    v = request.form.get('ora', '').strip()
    if v != 'mai' and not re.fullmatch(r'([01]\d|2[0-3]):[0-5]\d', v):
        flash('Ora non valida: per esempio 02:00 (oppure «mai»).', 'errore')
        return redirect(url_for('.indice'))
    conn().execute("insert into impostazioni (nome, valore) values ('ora_test_notturni', %s) "
                   "on conflict (nome) do update set valore = excluded.valore", (v,))
    flash('I test notturni partono %s.' % ('solo a mano' if v == 'mai' else 'ogni notte alle ' + v), 'ok')
    return redirect(url_for('.indice'))


@bp.route('/<int:tid>')
def test(tid):
    t = _test(tid)
    esecuzioni = conn().execute('select n.*, gi.commit, gi.motivo from esecuzioni_notturne n '
                                'join giri_notturni gi on gi.id = n.giro_id where n.test_id = %s order by n.id desc '
                                'limit 200', (tid,)).fetchall()
    return render_template('notturni/test.html', t=t, esecuzioni=esecuzioni, totale=celle(t['dati']),
                           nomi_campi=notturni.NOMI_CAMPI, in_corso=notturni.giro_in_corso(),
                           descrizione=solver.DESCRIZIONI.get(t['solutore'], t['solutore']))


@bp.route('/<int:tid>/ritesta', methods=['POST'])
def ritesta(tid):
    """Lo stesso test in tempo reale: stessi dati, solver, orari e limite di nodi, con i backtrack registrati."""
    t = _test(tid)
    caso = {'tipo': 'input', 'nome': 'notturno: %s' % t['nome'], 'dati': t['dati'], 'notturno': tid}
    opzioni = {'orari': t['opzioni'].get('orari', 1), 'max_backtrack': 1000}
    if t['opzioni'].get('ferma_nodi'):
        opzioni['ferma_nodi'] = t['opzioni']['ferma_nodi']
    try:
        pid, _ = esecutore.avvia_live(_config(), g.scuola['id'], caso, opzioni, [t['solutore']])
    except ValueError as ex:
        flash(str(ex), 'errore')
        return redirect(url_for('.test', tid=tid))
    return redirect(url_for('live.prova', pid=pid))


@bp.route('/<int:tid>/dati.json')
def scarica(tid):
    """I dati del test nel formato di --input: si caricano nel modulo del test in tempo reale («Da un file JSON»)."""
    t = _test(tid)
    nome = re.sub(r'[^a-z0-9]+', '-', ('%s %s' % (t['nome'], t['solutore'])).lower()).strip('-')
    return Response(json.dumps(t['dati'], ensure_ascii=False, indent=1), mimetype='application/json',
                    headers={'Content-Disposition': 'attachment; filename="notturno-%d-%s.json"' % (tid, nome)})


@bp.route('/<int:tid>/attivo', methods=['POST'])
def attivo(tid):
    t = _test(tid)
    conn().execute('update test_notturni set attivo = not attivo where id = %s', (tid,))
    flash('Test %s.' % ('disattivato: non si esegue piu\'' if t['attivo'] else 'riattivato'), 'ok')
    return redirect(request.form.get('poi') or url_for('.indice'))


@bp.route('/<int:tid>/elimina', methods=['POST'])
def elimina(tid):
    _test(tid)
    if notturni.test_in_corso() == tid:
        flash('Il test sta girando: prima ferma il giro.', 'errore')
        return redirect(url_for('.test', tid=tid))
    conn().execute('delete from test_notturni where id = %s', (tid,))
    flash('Test eliminato, con le sue esecuzioni.', 'ok')
    return redirect(url_for('.indice'))
