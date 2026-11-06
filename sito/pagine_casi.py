"""La pagina di un test: mentre gira (progresso, backtrack man mano che arrivano) e dopo (esito, orario trovato,
backtrack). Ogni backtrack si apre con l'orario fino a quel punto e i controlli di main per ogni docente."""
import json

from flask import Blueprint, Response, abort, jsonify, render_template, request, stream_with_context

from . import esecutore, notturni
from .db import conn

bp = Blueprint('casi', __name__, url_prefix='/casi')


def _esecuzione(eid):
    e = conn().execute('select e.*, p.nome as prova_nome, p.commit, p.stato as stato_prova, p.messaggio, p.scuola_id, '
                       'p.opzioni as opzioni_prova from esecuzioni e join prove p on p.id = e.prova_id '
                       'where e.id = %s', (eid,)).fetchone()
    if not e:
        abort(404)
    return e


@bp.route('/<int:eid>')
def caso(eid):
    e = _esecuzione(eid)
    n = conn().execute('select count(*) as n from backtrack where esecuzione_id = %s', (eid,)).fetchone()['n']
    da_aggiungere = notturni.da_esecuzione(conn(), e['scuola_id'], eid) if e.get('scuola_id') else None
    return render_template('casi/caso.html', e=e, r=e['risultato'] or {}, n_registrati=n,
                           in_corso=esecutore.in_corso(eid), notturno=_confronto_notturno(e),
                           nei_notturni=da_aggiungere and da_aggiungere['presente'], si_aggiunge=bool(da_aggiungere))


def _confronto_notturno(e):
    """Un test rifatto da un test notturno («Ritesta in tempo reale»): il suo riferimento e, a test finito, il confronto
    con i numeri di questa esecuzione (devono coincidere)."""
    tid = (e['caso_dati'] or {}).get('notturno')
    t = tid and conn().execute('select id, nome, riferimento from test_notturni where id = %s', (tid,)).fetchone()
    if not t or not t['riferimento']:
        return None
    r = e['risultato'] or {}
    if e['esito'] is None or not r.get('fine') and e['esito'] != 'errore nei dati':
        return {'test': t, 'differenze': None}  # in corso, o fermato prima della fine: niente confronto
    trovato = {'esito': e['esito'], 'nodi': r.get('nodi'), 'backtrack': r.get('backtrack'), 'pos_max': r.get('pos_max'),
               'orario': notturni.impronta_orario(r['orari'][0] if r.get('orari') else None),
               'errore': r.get('errore_dati')}
    if e['esito'] == 'errore nei dati':
        trovato.update(nodi=None, backtrack=None, pos_max=None)
    return {'test': t, 'differenze': notturni.confronta(t['riferimento'], trovato), 'nomi': notturni.NOMI_CAMPI}


@bp.route('/<int:eid>/stato')
def stato(eid):
    """Interrogato ogni secondo dalla pagina mentre il test gira."""
    e = conn().execute('select esito, risultato is not null and risultato ? %s as con_dati from esecuzioni '
                       'where id = %s', ('dati', eid)).fetchone()
    n = conn().execute('select count(*) as n from backtrack where esecuzione_id = %s', (eid,)).fetchone()['n']
    return jsonify({'esito': e['esito'], 'in_corso': esecutore.in_corso(eid), 'progresso': esecutore.progresso(eid),
                    'registrati': n, 'con_dati': e['con_dati']})


@bp.route('/<int:eid>/dati')
def dati(eid):
    """I nomi di giorni, ore, classi e docenti e le cattedre (il record "dati" della traccia)."""
    e = conn().execute('select risultato from esecuzioni where id = %s', (eid,)).fetchone()
    d = (e['risultato'] or {}).get('dati') if e else None
    return jsonify(d or {})


@bp.route('/<int:eid>/backtrack')
def elenco_backtrack(eid):
    """I backtrack registrati, in breve (?da=N per avere solo quelli dopo il numero N)."""
    da = request.args.get('da', 0, type=int)
    # giro: solo giorni-liberi (l'ordine delle celle cambia a ogni giro), null per gli altri solver
    righe = conn().execute("select n, nodo, pos, giorno, ora, classe, buca_dopo, riassunto, "
                           "(record->>'giro')::int as giro from backtrack "
                           "where esecuzione_id = %s and n > %s order by n limit 5000", (eid, da)).fetchall()
    return jsonify([dict(r) for r in righe])


@bp.route('/<int:eid>/backtrack/<int:n>')
def un_backtrack(eid, n):
    r = conn().execute('select record from backtrack where esecuzione_id = %s and n = %s', (eid, n)).fetchone()
    if not r:
        abort(404)
    return jsonify(r['record'])


@bp.route('/<int:eid>/nodi')
def nodi(eid):
    """I nodi della ricerca da ?da=N ad ?a=M (al massimo 2 milioni): i pezzi registrati che li coprono, e l'ultimo nodo
    registrato. Per scorrere la ricerca passo passo."""
    da = request.args.get('da', 1, type=int)
    a = min(request.args.get('a', da, type=int), da + 2000000)
    pezzi = conn().execute('select da, passi from nodi where esecuzione_id = %s and da <= %s and da + quanti > %s '
                           'order by da', (eid, a, da)).fetchall()
    ultimo = conn().execute('select max(da + quanti) - 1 as u from nodi where esecuzione_id = %s',
                            (eid,)).fetchone()['u']
    return jsonify({'pezzi': [[p['da'], p['passi']] for p in pezzi], 'ultimo': ultimo})


@bp.route('/<int:eid>/ferma', methods=['POST'])
def ferma(eid):
    esecutore.ferma_esecuzione(eid)
    return jsonify({'ok': True})


@bp.route('/<int:eid>/dati.json')
def scarica_dati(eid):
    """I dati del test nel formato di --input: si possono ricaricare in un test o importare nei dati della scuola."""
    p = conn().execute('select p.dati_scuola from prove p join esecuzioni e on e.prova_id = p.id where e.id = %s',
                       (eid,)).fetchone()
    if not p or not p['dati_scuola']:
        abort(404)
    return Response(json.dumps(p['dati_scuola'], ensure_ascii=False, indent=1), mimetype='application/json',
                    headers={'Content-Disposition': 'attachment; filename="test-%d-dati.json"' % eid})


@bp.route('/<int:eid>/backtrack.json')
def scarica_backtrack(eid):
    """Tutti i backtrack registrati, come nella traccia del solver (un JSON per riga)."""
    def righe():
        ultimo = 0
        while True:  # a blocchi: non si tiene tutto in memoria
            blocco = conn().execute('select n, record from backtrack where esecuzione_id = %s and n > %s order by n '
                                    'limit 200', (eid, ultimo)).fetchall()
            if not blocco:
                return
            for r in blocco:
                yield json.dumps(r['record'], ensure_ascii=False) + '\n'
            ultimo = blocco[-1]['n']
    return Response(stream_with_context(righe()), mimetype='application/x-ndjson',
                    headers={'Content-Disposition': 'attachment; filename="test-%d-backtrack.jsonl"' % eid})
