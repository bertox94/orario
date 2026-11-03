"""Sezione "Test notturni": batterie di casi in tre fasi (come tests/casuali.py), programmate o ricorrenti."""
import datetime
import json

from flask import Blueprint, current_app, flash, redirect, render_template, request, url_for

from . import esecutore, solver
from .db import conn

bp = Blueprint('notturni', __name__, url_prefix='/notturni')

# ordine degli esiti per il confronto: piu' alto = meglio
VALORE_ESITO = {'trovato': 3, 'nessuna soluzione': 3, 'impossibile': 3, 'non finito': 1, 'fermato': 1,
                'interrotto (file troppo grande)': 1, 'interrotto (errore di scrittura)': 1, 'interrotta': 0,
                'errore': 0, 'errore nei dati': 0}


def _config():
    return current_app.config['SITO']


def _intero(nome, predefinito, minimo=0, massimo=None):
    try:
        v = int(request.form.get(nome, '') or predefinito)
    except ValueError:
        v = predefinito
    v = max(minimo, v)
    return min(massimo, v) if massimo is not None else v


def _opzioni_dal_modulo():
    f = request.form
    varianti = [v for v in ('normale', 'ritardi') if f.get('variante_' + v)]
    return {'semi_da': _intero('semi_da', 1), 'quanti': _intero('quanti', 20, 0, 10000),
            'classi': _intero('classi', 8, 1, 120), 'varianti': varianti or ['normale'],
            'scuola': bool(f.get('scuola')), 'ordine_classi': _intero('ordine_classi', 0) if f.get('ordine_classi', '').strip().isdigit() else None,
            'fase1': _intero('fase1', 60, 1), 'fase2': _intero('fase2', 600), 'fase3': _intero('fase3', 600),
            'backtrack': bool(f.get('backtrack')), 'spazio_mb': _intero('spazio_mb', _config().spazio_mb, 1),
            'ore_obbligatorie': _intero('ore_obbligatorie', 5, 1, 6), 'ore_prof_classe': _intero('ore_prof_classe', 3, 1, 6),
            'classi_giorno': _intero('classi_giorno', 3, 1, 20)}


def ultime_fasi(prova_id):
    """Per ogni caso della prova, l'esecuzione della fase piu' alta (il suo esito finale), con quante fasi ha fatto."""
    return conn().execute('''
        select distinct on (caso) e.*, (select count(*) from esecuzioni x where x.prova_id = e.prova_id and x.caso = e.caso) as n_fasi
        from esecuzioni e where prova_id = %s order by caso, fase desc''', (prova_id,)).fetchall()


def _ordina_casi(righe):
    """Dati della scuola prima, poi per seme e variante."""
    def chiave(r):
        d = r['caso_dati']
        return (0, 0, 0) if d.get('tipo') != 'casuale' else (1, d.get('seme', 0), int(bool(d.get('ritardi'))))
    return sorted(righe, key=chiave)


@bp.route('/')
def indice():
    c = conn()
    prove = c.execute("select * from prove where tipo = 'notturna' order by coalesce(programmata_per, creata) desc, id desc "
                      "limit 100").fetchall()
    conteggi = {r['prova_id']: r for r in c.execute(
        "select prova_id, count(distinct caso) as casi, count(*) filter (where esito is null) as in_corso "
        "from esecuzioni group by prova_id").fetchall()}
    pianificazioni = c.execute('select * from pianificazioni order by ora, id').fetchall()
    stasera = datetime.datetime.now().replace(hour=22, minute=0, second=0, microsecond=0)
    return render_template('notturni/indice.html', prove=prove, conteggi=conteggi, pianificazioni=pianificazioni,
                           libero_gb=solver.spazio_libero_mb(_config().dati) / 1024, config=_config(),
                           predefinita=stasera.strftime('%Y-%m-%dT%H:%M'),
                           n_classi=c.execute('select count(*) as n from classi').fetchone()['n'])


@bp.route('/nuova', methods=['POST'])
def nuova():
    o = _opzioni_dal_modulo()
    quando = None
    if request.form.get('quando') == 'ora_data' and request.form.get('ora_data'):
        quando = datetime.datetime.fromisoformat(request.form['ora_data']).astimezone()
    if not o['quanti'] and not o['scuola']:
        flash('La prova non ha casi: indica quanti semi o includi i dati della scuola.', 'errore')
        return redirect(url_for('.indice'))
    pid = esecutore.crea_notturna(conn(), o, quando, nome=request.form.get('nome') or None)
    flash('Prova %d %s.' % (pid, 'messa in coda' if quando is None else 'programmata per ' + quando.strftime('%d/%m %H:%M')), 'ok')
    return redirect(url_for('.prova', pid=pid))


@bp.route('/pianificazioni', methods=['POST'])
def nuova_pianificazione():
    giorni = [int(g) for g in request.form.getlist('giorni')]
    if not giorni:
        flash('Scegli almeno un giorno della settimana.', 'errore')
        return redirect(url_for('.indice'))
    ora = datetime.time.fromisoformat(request.form.get('ora') or '22:00')
    conn().execute('insert into pianificazioni (nome, ora, giorni, opzioni) values (%s, %s, %s, %s)',
                   (request.form.get('nome') or 'Notturna', ora, giorni,
                    json.dumps(_opzioni_dal_modulo())))
    flash('Pianificazione aggiunta.', 'ok')
    return redirect(url_for('.indice'))


@bp.route('/pianificazioni/<int:pid>/attiva', methods=['POST'])
def attiva_pianificazione(pid):
    conn().execute('update pianificazioni set attiva = not attiva where id = %s', (pid,))
    return redirect(url_for('.indice'))


@bp.route('/pianificazioni/<int:pid>/elimina', methods=['POST'])
def elimina_pianificazione(pid):
    conn().execute('delete from pianificazioni where id = %s', (pid,))
    return redirect(url_for('.indice'))


@bp.route('/<int:pid>')
def prova(pid):
    p = conn().execute('select * from prove where id = %s', (pid,)).fetchone()
    if not p or p['tipo'] != 'notturna':
        return redirect(url_for('.indice'))
    casi = _ordina_casi(ultime_fasi(pid))
    esiti = {}
    for r in casi:
        k = r['esito'] or 'in corso'
        esiti[k] = esiti.get(k, 0) + 1
    totale = len(esecutore.casi_della_notturna(p['opzioni'])) + (1 if p['opzioni'].get('scuola') else 0)
    altre = conn().execute("select id, nome, creata from prove where tipo = 'notturna' and id <> %s and stato = 'finita' "
                           "order by id desc limit 30", (pid,)).fetchall()
    return render_template('notturni/prova.html', prova=p, casi=casi, esiti=esiti, totale=totale, altre=altre)


@bp.route('/<int:pid>/annulla', methods=['POST'])
def annulla(pid):
    esecutore.annulla_notturna(_config(), pid)
    flash('Prova annullata (i casi in corso si fermano in modo ordinato).', 'ok')
    return redirect(url_for('.prova', pid=pid))


@bp.route('/<int:pid>/elimina', methods=['POST'])
def elimina(pid):
    if esecutore.elimina_prova(_config(), pid):
        flash('Prova eliminata, con i suoi file.', 'ok')
        return redirect(url_for('.indice'))
    flash('Una prova in corso non si puo\' eliminare: prima annullala.', 'errore')
    return redirect(url_for('.prova', pid=pid))


@bp.route('/confronta')
def confronta():
    """Confronto tra due prove: per ogni caso l'esito finale nell'una e nell'altra."""
    a, b = request.args.get('a', type=int), request.args.get('b', type=int)
    pa = conn().execute('select * from prove where id = %s', (a,)).fetchone()
    pb = conn().execute('select * from prove where id = %s', (b,)).fetchone()
    if not pa or not pb:
        flash('Scegli due prove da confrontare.', 'errore')
        return redirect(url_for('.indice'))
    ra = {r['caso']: r for r in ultime_fasi(a)}
    rb = {r['caso']: r for r in ultime_fasi(b)}
    righe, conta = [], {'migliorati': 0, 'peggiorati': 0, 'uguali': 0, 'solo in una': 0}
    for nome in set(ra) | set(rb):
        x, y = ra.get(nome), rb.get(nome)
        if not x or not y:
            giudizio = 'solo in una'
        else:
            vx, vy = VALORE_ESITO.get(x['esito'], 0), VALORE_ESITO.get(y['esito'], 0)
            if vy > vx:
                giudizio = 'migliorati'
            elif vy < vx:
                giudizio = 'peggiorati'
            elif x['esito'] == 'trovato' and y['esito'] == 'trovato' and x['secondi'] and y['secondi'] \
                    and abs(y['secondi'] - x['secondi']) > max(1.0, 0.25 * x['secondi']):
                giudizio = 'migliorati' if y['secondi'] < x['secondi'] else 'peggiorati'
            else:
                giudizio = 'uguali'
        conta[giudizio] += 1
        righe.append({'caso': nome, 'a': x, 'b': y, 'giudizio': giudizio})
    return render_template('notturni/confronta.html', pa=pa, pb=pb, righe=_ordina_righe(righe, ra, rb), conta=conta)


def _ordina_righe(righe, ra, rb):
    def chiave(r):
        d = (ra.get(r['caso']) or rb.get(r['caso']))['caso_dati']
        return (0, 0, 0) if d.get('tipo') != 'casuale' else (1, d.get('seme', 0), int(bool(d.get('ritardi'))))
    return sorted(righe, key=chiave)
