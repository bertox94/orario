"""Sezione "Test notturni": i test che ogni notte devono dare gli stessi numeri (vedi notturni.py).

La pagina mostra i casi raggruppati (per seme, classi...): ogni caso e' una riga che si apre con una riga per solver, e
ogni giro e' una colonna (il piu' recente a destra), con tempo, esito, nodi e backtrack in verticale, in verde o in
rosso se vanno meglio o peggio dell'esecuzione prima dello stesso test; la × accanto alla data elimina il giro. Il
confronto con il riferimento accettato e' nel segno ≠ e nella pagina del test, con il pulsante per accettarlo. Ogni test
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


def _torna(predefinito='.indice'):
    """Dopo un modulo: alla pagina indicata in «poi» (solo un indirizzo di questo sito), se no a quella predefinita."""
    poi = request.form.get('poi', '')
    return redirect(poi if poi.startswith('/') and not poi.startswith('//') else url_for(predefinito))


def _test(tid):
    t = conn().execute('select t.*, d.dati from test_notturni t join dati_notturni d on d.id = t.dati_id '
                       'where t.id = %s and t.scuola_id = %s', (tid, g.scuola['id'])).fetchone()
    if not t:
        abort(404)
    return t


def celle(dati):
    """Le celle della ricerca (giorni x ore x classi): la posizione massima si legge su questo totale."""
    try:
        n = lambda v: v if isinstance(v, int) else len(v)  # noqa: E731
        return n(dati['giorni']) * n(dati['ore']) * len({l['classe'] for l in dati['lezioni']})
    except (KeyError, TypeError):
        return None


_ULTIME = '''
    select t.id, t.nome, t.solutore, t.caso, t.opzioni, t.attivo, t.riferimento, t.accettato, t.accettato_da,
           (select count(*) from esecuzioni_notturne x where x.test_id = t.id) as quante,
           n.id as nid, n.inizio, n.secondi, n.esito, n.nodi, n.backtrack, n.pos_max, n.orario, n.errore, n.confronto,
           n.differenze, n.giro_id
    from test_notturni t
    left join lateral (select * from esecuzioni_notturne x where x.test_id = t.id order by x.id desc limit 1) n on true
    where t.scuola_id = %s
    order by t.attivo desc, t.nome, t.solutore, t.id'''


# esiti di una ricerca che non e' arrivata a una conclusione (il filtro «nessuno concluso»)
NON_TERMINATI = ('limite di nodi', 'fermato', 'interrotto', 'errore')


# come raggruppare i casi nella pagina (?per=...): il campo del caso, e il titolo del gruppo
RAGGRUPPAMENTI = {
    'seme': ('seme', lambda k: 'seme %s' % k.get('seme')),
    'classi': ('classi', lambda k: '%s classi' % k.get('classi')),
    'ritardi': ('ritardi', lambda k: 'con ritardi' if k.get('ritardi') else 'senza ritardi'),
    'ordine': ('ordine', lambda k: 'ordine delle classi %d%s' % (k.get('ordine', 0), ' (come generato)'
                                                                     if not k.get('ordine') else '')),
}
CAMPI_CASO = ('seme', 'classi', 'ritardi', 'ordine')


def _valore(k, campo):
    if campo == 'ritardi':
        return bool(k.get('ritardi'))
    return k.get(campo, 0)


def _chiave_caso(caso, nome, per):
    """L'ordine dei casi: prima il campo del raggruppamento, poi gli altri (seme, classi, ritardi, ordine); i casi non
    casuali (file, test in tempo reale) in fondo, per nome."""
    k = caso or {}
    if k.get('tipo') != 'casuale':
        return (1, (), nome)
    campi = (per,) + tuple(c for c in CAMPI_CASO if c != per)
    return (0, tuple(_valore(k, c) for c in campi), '')


def _gruppi(casi, per, quali, solver):
    """I casi divisi in gruppi (stesso seme, stesse classi...), ognuno con il conto per solver: quanti trovati su
    quanti test."""
    gruppi = []
    for c in casi:
        k = c['caso']
        titolo = RAGGRUPPAMENTI[per][1](k) if k.get('tipo') == 'casuale' else 'altri casi'
        if not gruppi or gruppi[-1]['titolo'] != titolo:
            gruppi.append({'titolo': titolo, 'casi': [], 'conto': {}})
        gruppi[-1]['casi'].append(c)
    for gr in gruppi:
        for s in solver:
            test = [c[quali][s] for c in gr['casi'] if s in c[quali]]
            gr['conto'][s] = (sum(1 for r in test if r['esito'] == 'trovato'), len(test))
    return gruppi


# ---- la storia: un giro per colonna ----
GIRI_IN_COLONNA = 8   # quanti giri si mostrano (gli ultimi): ogni giro e' una colonna, il piu' recente a destra
RANGO_ESITO = {'trovato': 2, 'nessuna soluzione': 2, 'errore nei dati': 2, 'limite di nodi': 1}


def _confronta(prima, dopo):
    """I numeri di un'esecuzione rispetto a quella prima dello stesso test: {campo: 'meglio' | 'peggio'}. Esito:
    concluso meglio di limite di nodi, meglio di fermato o in errore; nodi, backtrack e tempo: meno e' meglio (il tempo
    solo se cambia di piu' del 20% e di 0,1 s, perche' dipende dal carico del computer)."""
    if not prima:
        return {}
    v = {}
    a, b = RANGO_ESITO.get(prima['esito'], 0), RANGO_ESITO.get(dopo['esito'], 0)
    if a != b:
        v['esito'] = 'meglio' if b > a else 'peggio'
    for campo in ('nodi', 'backtrack'):
        if prima[campo] is not None and dopo[campo] is not None and prima[campo] != dopo[campo]:
            v[campo] = 'meglio' if dopo[campo] < prima[campo] else 'peggio'
    x, y = prima['secondi'], dopo['secondi']
    if x is not None and y is not None and abs(y - x) > max(0.1, 0.2 * x):
        v['secondi'] = 'meglio' if y < x else 'peggio'
    return v


_GIRI_DEI_TEST = """
    select gi.id, gi.inizio, gi.stato, gi.motivo, gi.commit from giri_notturni gi
    where gi.scuola_id = %s and exists (select 1 from esecuzioni_notturne n where n.giro_id = gi.id
                                        and n.test_id = any(%s))
    order by gi.id desc limit %s"""


def storia(c, scuola_id, test_ids):
    """Gli ultimi GIRI_IN_COLONNA giri in cui hanno girato questi test, dal piu' vecchio al piu' recente, e per ogni
    (test, giro) l'esecuzione con il confronto con quella prima dello stesso test (anche di un giro non mostrato)."""
    giri = c.execute(_GIRI_DEI_TEST, (scuola_id, test_ids, GIRI_IN_COLONNA + 1)).fetchall()[::-1]
    if not giri:
        return [], {}
    esec = c.execute('select * from esecuzioni_notturne where giro_id = any(%s) and test_id = any(%s) order by id',
                     ([gi['id'] for gi in giri], test_ids)).fetchall()
    per_test = {}
    for n in esec:
        per_test.setdefault(n['test_id'], []).append(n)
    celle = {}
    for tid, lista in per_test.items():
        prima = None
        for n in lista:
            if n['esito']:
                celle[tid, n['giro_id']] = dict(n, cambia=_confronta(prima, n))
                prima = n
            else:
                celle[tid, n['giro_id']] = dict(n, cambia={})  # in corso
    mostrati = giri[1:] if len(giri) > GIRI_IN_COLONNA else giri  # il primo serve solo per il confronto
    return mostrati, celle


def riassunto_giro(c, colonne, celle):
    """Per ogni caso e ogni giro (colonna): quando ha girato, quanti orari trovati su quanti test, e se qualcosa va
    peggio (o solo meglio) dell'esecuzione prima (per la riga del caso)."""
    ids = [r['id'] for r in c['per_solver'].values()]
    righe = []
    for gi in colonne:
        mie = [celle[t, gi['id']] for t in ids if (t, gi['id']) in celle]
        x = {'giro': gi, 'test': len(mie), 'trovati': sum(1 for n in mie if n['esito'] == 'trovato'),
             'quando': min((n['inizio'] for n in mie), default=None),
             'in_corso': any(not n['esito'] for n in mie),
             # un peggioramento qualunque (esito, nodi, backtrack, tempo) di un solver colora di rosso la casella del
             # caso; se non ce ne sono, un miglioramento la colora di verde
             'peggio': any('peggio' in n['cambia'].values() for n in mie)}
        x['meglio'] = not x['peggio'] and any('meglio' in n['cambia'].values() for n in mie)
        righe.append(x if mie else None)
    return righe


def raggruppa(righe, ordine_solver, per='seme'):
    """I test raggruppati per caso (stesso nome): una riga per caso (la data dell'ultima esecuzione, poi per ogni solver
    esito, nodi e durata). Ritorna (solver, gruppi dei casi, test disattivati, riepilogo per solver). Tutti i casi
    stanno nella stessa tabella; 'concluso' dice se almeno un solver e' arrivato a una conclusione (per il filtro)."""
    attivi = [r for r in righe if r['attivo']]
    solver = [s for s in ordine_solver if any(r['solutore'] == s for r in attivi)]
    solver += sorted({r['solutore'] for r in attivi} - set(solver))
    casi = {}
    for r in attivi:
        c = casi.setdefault(r['nome'], {'nome': r['nome'], 'chiave': _chiave_caso(r['caso'], r['nome'], per),
                                        'caso': r['caso'] or {}, 'per_solver': {}})
        c['per_solver'][r['solutore']] = r
    casi = sorted(casi.values(), key=lambda c: c['chiave'])
    aperto = lambda r: not r['esito'] or r['esito'] in NON_TERMINATI  # noqa: E731
    for c in casi:
        c['concluso'] = any(not aperto(r) for r in c['per_solver'].values())
        date = [r['inizio'] for r in c['per_solver'].values() if r['inizio']]
        c['data'] = max(date) if date else None  # l'ultima esecuzione del caso
    gruppi = _gruppi(casi, per, 'per_solver', solver)  # una riga per caso, tutti insieme
    riepilogo = []
    for s in solver:
        mie = [r for r in attivi if r['solutore'] == s]
        conta = lambda f: sum(1 for r in mie if f(r))  # noqa: E731
        riepilogo.append({
            'solver': s, 'test': len(mie),
            'trovato': conta(lambda r: r['esito'] == 'trovato'),
            'nessuna': conta(lambda r: r['esito'] == 'nessuna soluzione'),
            'errore_dati': conta(lambda r: r['esito'] == 'errore nei dati'),
            'non_terminati': conta(lambda r: r['esito'] in NON_TERMINATI),
            'mai': conta(lambda r: not r['esito']),
            'diversi': conta(lambda r: r['confronto'] == 'diverso'),
            'secondi': sum(r['secondi'] or 0 for r in mie),
            # quanti casi risolve solo lui (tra quelli in cui c'e')
            'solo_lui': sum(1 for c in casi if (c['per_solver'].get(s) or {}).get('esito') == 'trovato' and
                            not any(t != s and r['esito'] == 'trovato' for t, r in c['per_solver'].items())),
        })
    return solver, gruppi, [r for r in righe if not r['attivo']], riepilogo


@bp.route('/')
def indice():
    righe = conn().execute(_ULTIME, (g.scuola['id'],)).fetchall()
    per = request.args.get('per') if request.args.get('per') in RAGGRUPPAMENTI else 'seme'
    solver_colonne, gruppi, disattivati, riepilogo = raggruppa(righe, list(solver.solutori()), per)
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
    colonne, celle_giri = storia(conn(), g.scuola['id'], [r['id'] for r in righe if r['attivo']])
    for gr in gruppi:
        for c in gr['casi']:
            c['giri'] = riassunto_giro(c, colonne, celle_giri)
    return render_template('notturni/indice.html', righe=righe, giri=giri, diversi=diversi,
                           colonne=colonne, celle_giri=celle_giri,
                           solver_colonne=solver_colonne, gruppi=gruppi,
                           disattivati=disattivati, riepilogo=riepilogo, per=per,
                           raggruppamenti=list(RAGGRUPPAMENTI),
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
        semi = notturni.intervallo(f.get('semi', ''), 0, 10 ** 9)
        classi = notturni.intervallo(f.get('classi', ''), 1, 120)
        ordini = notturni.intervallo(f.get('ordini', '') or '0', 0, 10 ** 6)
    except ValueError:
        flash('Semi, classi e ordini delle classi: un numero o un intervallo col trattino, anche più di uno separati da '
              'virgole, per esempio «1-6», «8-16» e «0-3» (classi da 1 a 120; ordine 0: come generato).', 'errore')
        return redirect(url_for('.indice'))
    ritardi = {'senza': [False], 'con': [True], 'entrambi': [False, True]}.get(f.get('ritardi'), [False])
    solutori = [s for s in solver.solutori() if s in f.getlist('solutore')]
    if not solutori:
        flash('Scegli almeno un solver.', 'errore')
        return redirect(url_for('.indice'))
    if len(semi) * len(classi) * len(ritardi) * len(ordini) * len(solutori) > 20000:
        flash('Troppi test in una volta (al massimo 20.000).', 'errore')
        return redirect(url_for('.indice'))
    try:
        ferma_nodi = max(1, int(f.get('ferma_nodi') or notturni.FERMA_NODI))
        orari = min(1000, max(1, int(f.get('orari') or 1)))
    except ValueError:
        flash('Limite di nodi e orari: numeri interi.', 'errore')
        return redirect(url_for('.indice'))
    aggiunti, gia, problemi, ids = notturni.genera(conn(), g.scuola['id'], semi, classi, ritardi, solutori, ferma_nodi,
                                                   orari, casi_repository=bool(f.get('repository')), ordini=ordini)
    flash('Test aggiunti: %d%s.' % (aggiunti, ' (%d c\'era%s già)' % (gia, '' if gia == 1 else 'no') if gia else ''), 'ok')
    for p in problemi[:10]:
        flash('Non generato: %s' % p, 'errore')
    if f.get('esegui') and ids:
        conn().commit()  # il giro legge i test con un'altra connessione: devono esserci gia'
        _avvia(ids)
    return redirect(url_for('.indice'))


def _avvia(soli):
    """Un giro a mano adesso con i test indicati (None: tutti i test attivi)."""
    gid = notturni.avvia_giro(_config(), g.scuola['id'], 'a mano', soli)
    if gid is None:
        flash('C\'e\' gia\' un giro in corso: aspetta che finisca, o fermalo.' if notturni.giro_in_corso()
              else 'Nessun test da eseguire.', 'errore')
    return gid


def _aggiungi_esecuzioni(eids):
    """Le esecuzioni di un test in tempo reale diventano test notturni (quelle che non lo sono gia')."""
    try:
        ferma_nodi = max(1, int(request.form['ferma_nodi'])) if request.form.get('ferma_nodi') else None
    except ValueError:
        ferma_nodi = None
    nuovi = gia = 0
    for eid in eids:
        t = notturni.da_esecuzione(conn(), g.scuola['id'], eid, ferma_nodi)
        if not t:
            continue
        if not t['presente'] and notturni.aggiungi(conn(), g.scuola['id'], t['nome'], t['solutore'], t['caso'],
                                                   t['dati'], t['opzioni'])[1]:
            nuovi += 1
        else:
            gia += 1
    if not nuovi and not gia:
        abort(404)
    if nuovi == 1 and not gia:
        flash('Aggiunto ai test notturni.', 'ok')
    else:
        flash(' '.join(x for x in ('Aggiunti ai test notturni: %d.' % nuovi if nuovi else '',
                                   '%d c\'era%s già.' % (gia, '' if gia == 1 else 'no') if gia else '') if x), 'ok')
    return _torna()


@bp.route('/da-test/<int:eid>', methods=['POST'])
def da_test(eid):
    """Un test in tempo reale diventa un test notturno: stessi dati e stesso solver, con il limite di nodi del test."""
    return _aggiungi_esecuzioni([eid])


@bp.route('/da-prova/<int:pid>', methods=['POST'])
def da_prova(pid):
    """Un test in tempo reale con piu' solver: un test notturno per ogni solver che non c'e' gia'."""
    eids = [r['id'] for r in conn().execute('select e.id from esecuzioni e join prove p on p.id = e.prova_id '
                                            'where p.id = %s and p.scuola_id = %s order by e.id', (pid, g.scuola['id']))]
    return _aggiungi_esecuzioni(eids)


@bp.route('/esegui', methods=['POST'])
def esegui():
    """Un giro adesso: tutti i test attivi, o solo quelli indicati (tid, anche piu' di uno: i solver di un caso) o solo
    quelli diversi (diversi=1)."""
    soli = None
    if request.form.getlist('tid'):
        soli = [_test(int(t))['id'] for t in request.form.getlist('tid')]
    elif request.form.get('diversi'):
        soli = [r['id'] for r in conn().execute(_ULTIME, (g.scuola['id'],)) if r['confronto'] == 'diverso']
    _avvia(soli)
    return _torna()


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
    return _torna()


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
    return _torna()


@bp.route('/<int:tid>/elimina', methods=['POST'])
def elimina(tid):
    _test(tid)
    if notturni.test_in_corso() == tid:
        flash('Il test sta girando: prima ferma il giro.', 'errore')
        return redirect(url_for('.test', tid=tid))
    conn().execute('delete from test_notturni where id = %s', (tid,))
    notturni.pulisci_dati(conn(), g.scuola['id'])
    flash('Test eliminato, con le sue esecuzioni.', 'ok')
    return redirect(url_for('.indice'))


@bp.route('/togli', methods=['POST'])
def togli():
    """Elimina i test scelti nella tabella (per ogni caso spuntato i suoi test, uno per solver), con le esecuzioni: i
    giri dopo eseguono solo quelli rimasti. Il test che sta girando resta."""
    ids = set()
    for v in request.form.getlist('tid'):
        ids.update(int(x) for x in v.split(',') if x.strip().isdigit())
    in_corso = notturni.test_in_corso()
    restano = {in_corso} & ids
    ids -= restano
    n = conn().execute('delete from test_notturni where scuola_id = %s and id = any(%s)',
                       (g.scuola['id'], sorted(ids))).rowcount if ids else 0
    notturni.pulisci_dati(conn(), g.scuola['id'])
    flash('Eliminat%s %d test.' % ('o' if n == 1 else 'i', n), 'ok')
    if restano:
        flash('Un test sta girando e non è stato eliminato: fermalo o aspetta la fine del giro.', 'errore')
    return redirect(url_for('.indice'))


@bp.route('/giro/<int:gid>/elimina', methods=['POST'])
def elimina_giro(gid):
    """Elimina un giro (una sessione di test notturni) con tutte le sue esecuzioni: la sua colonna sparisce. I
    riferimenti accettati dei test restano."""
    gi = conn().execute('select * from giri_notturni where id = %s and scuola_id = %s', (gid, g.scuola['id'])).fetchone()
    if not gi:
        abort(404)
    if notturni.giro_in_corso() == gid:
        flash('Il giro sta girando: prima fermalo.', 'errore')
        return redirect(url_for('.indice'))
    conn().execute('delete from giri_notturni where id = %s', (gid,))
    flash('Giro del %s eliminato, con le sue esecuzioni.' % gi['inizio'].astimezone().strftime('%d/%m/%Y %H:%M'), 'ok')
    return _torna()
