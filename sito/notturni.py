"""I test notturni: ogni notte gli stessi test, e i loro numeri confrontati con quelli accettati.

Un test notturno e' un caso (dati salvati nel database) con un solver e le opzioni della ricerca. La ricerca si ferma
sempre allo stesso nodo (--ferma-nodi) o prima, se trova l'orario o prova tutto: quindi i numeri (esito, nodi,
backtrack, posizione piu' avanti, primo orario trovato) sono sempre gli stessi finche' la ricerca non cambia. Il tempo
si mostra ma non si confronta (dipende dal computer e dal carico).

Alla prima esecuzione i numeri diventano il riferimento. Dopo, ogni esecuzione e' «uguale» o «diverso» (con le
differenze campo per campo): un cambiamento voluto della ricerca si accetta dalla pagina, e i numeri nuovi diventano il
riferimento.

Un giro esegue tutti i test attivi di una scuola uno dopo l'altro (non in parallelo: i tempi restano confrontabili e il
Raspberry non si satura). Parte da solo ogni notte all'ora impostata (thread di sorveglianza, uno per processo del sito)
o a mano. Un solo giro alla volta.
"""
import datetime
import hashlib
import json
import os
import shutil
import tempfile
import threading
import time
import traceback

from . import db, generatore, solver

CAMPI = ('esito', 'nodi', 'backtrack', 'pos_max', 'orario', 'errore')  # quelli che devono coincidere
NOMI_CAMPI = {'esito': 'risultato', 'nodi': 'nodi', 'backtrack': 'backtrack', 'pos_max': 'posizione massima',
              'orario': 'orario trovato', 'errore': 'errore nei dati'}
FERMA_NODI = 50000000  # default, come nel test in tempo reale: su un Mac fino a circa 40 s per test con main
CASI_DEL_REPOSITORY = os.path.join(solver.RADICE, 'tests', 'casi')

_lock = threading.Lock()
_giro = {'id': None, 'esecuzione': None, 'fermare': False, 'test': None}  # il giro in corso
_sorveglianza_avviata = False


def _json(v):
    return json.dumps(v, ensure_ascii=False)


def impronta_orario(orario):
    """L'impronta del primo orario trovato (la stessa di tests/non_regressione.py)."""
    return hashlib.md5(json.dumps(orario).encode()).hexdigest()[:12] if orario is not None else None


# ---- creare i test ----

def nome_casuale(seme, classi, ritardi, ordine=0):
    nome = 'seme %d, %d classi%s' % (seme, classi, ', ritardi' if ritardi else '')
    return nome + (', ordine %d' % ordine if ordine else '')


def presente(c, scuola_id, solutore, dati, opzioni):
    """L'id del test notturno attivo con gli stessi dati, solver e opzioni, o None."""
    r = c.execute('select id from test_notturni where scuola_id = %s and solutore = %s and dati = %s::jsonb '
                  'and opzioni = %s::jsonb and attivo', (scuola_id, solutore, _json(dati), _json(opzioni))).fetchone()
    return r['id'] if r else None


def da_esecuzione(c, scuola_id, eid, ferma_nodi=None):
    """Il test notturno che si fa da un test in tempo reale (stessi dati e stesso solver; il limite di nodi del test, o
    quello di default), come argomenti di aggiungi, piu' 'presente': l'id se c'e' gia'. None se non ci sono i dati."""
    e = c.execute('select e.caso, e.opzioni, p.dati_scuola, p.opzioni as opzioni_prova from esecuzioni e '
                  'join prove p on p.id = e.prova_id where e.id = %s and p.scuola_id = %s', (eid, scuola_id)).fetchone()
    if not e or not e['dati_scuola']:
        return None
    o = e['opzioni'] or {}
    t = {'nome': e['caso'], 'solutore': o.get('solutore', 'main'),
         'caso': dict((e['opzioni_prova'] or {}).get('caso') or {}, da_test=eid), 'dati': e['dati_scuola'],
         'opzioni': {'orari': o.get('orari', 1), 'ferma_nodi': ferma_nodi or o.get('ferma_nodi') or FERMA_NODI}}
    t['presente'] = presente(c, scuola_id, t['solutore'], t['dati'], t['opzioni'])
    return t


def aggiungi(c, scuola_id, nome, solutore, caso, dati, opzioni):
    """Aggiunge un test (se non c'e' gia' lo stesso: stessi dati, solver e opzioni). Ritorna True se e' nuovo."""
    if presente(c, scuola_id, solutore, dati, opzioni):
        return False
    c.execute('insert into test_notturni (scuola_id, nome, solutore, caso, dati, opzioni) values (%s, %s, %s, %s, %s, %s)',
              (scuola_id, nome, solutore, _json(caso), _json(dati), _json(opzioni)))
    return True


def genera(c, scuola_id, semi, classi, ritardi, solutori, ferma_nodi, orari=1, casi_repository=False, ordini=(0,)):
    """I test di tutte le combinazioni (seme x classi x ritardi x ordine delle classi) per ogni solver, piu' (se
    chiesto) i casi di tests/casi. Ritorna (aggiunti, gia' presenti, [messaggi per i casi che il generatore non sa
    fare])."""
    opzioni = {'orari': orari, 'ferma_nodi': ferma_nodi}
    casi, problemi = [], []
    for seme in semi:
        for n in classi:
            for r in ritardi:
                try:
                    base = generatore.genera(seme, n, r)
                except ValueError as ex:
                    problemi.append('%s: %s' % (nome_casuale(seme, n, r), ex))
                    continue
                for k in ordini:
                    caso = {'tipo': 'casuale', 'seme': seme, 'classi': n, 'ritardi': r}
                    if k:
                        caso['ordine'] = k
                    casi.append((nome_casuale(seme, n, r, k), caso, generatore.riordina_classi(base, k)))
    if casi_repository and os.path.isdir(CASI_DEL_REPOSITORY):
        for f in sorted(os.listdir(CASI_DEL_REPOSITORY)):
            if f.endswith('.json'):
                with open(os.path.join(CASI_DEL_REPOSITORY, f)) as fh:
                    casi.append(('tests/casi/' + f[:-5], {'tipo': 'file', 'nome': 'tests/casi/' + f}, json.load(fh)))
    aggiunti = gia = 0
    for nome, caso, dati in casi:
        for s in solutori:
            if aggiungi(c, scuola_id, nome, s, caso, dati, opzioni):
                aggiunti += 1
            else:
                gia += 1
    return aggiunti, gia, problemi


# ---- eseguire un test ----

def esegui_test(config, test):
    """Il solver sul test, senza registrare backtrack ne' nodi. Ritorna i numeri della ricerca (vedi CAMPI) e i
    secondi. Si ferma prima se il giro viene fermato (esito 'fermato')."""
    cartella = tempfile.mkdtemp(prefix='n-', dir=_cartella(config))
    try:
        exe = solver.compila(config.cartella_bin, test['solutore'])
        opzioni = {'solutore': test['solutore'], 'orari': test['opzioni'].get('orari', 1), 'max_backtrack': 0,
                   'max_nodi': 0, 'ferma_nodi': test['opzioni'].get('ferma_nodi')}
        e = solver.Esecuzione(exe, test['dati'], opzioni, cartella)
        with _lock:
            _giro['esecuzione'] = e
            fermare = _giro['fermare']
        if fermare:
            e.ferma()
        try:
            r = e.attendi()
        finally:
            with _lock:
                _giro['esecuzione'] = None
        fine = r['fine'] or {}
        return {'esito': r['esito'], 'nodi': r['nodi'], 'backtrack': r['backtrack'] if r['fine'] else None,
                'pos_max': fine.get('pos_max'), 'orario': impronta_orario(r['orari'][0] if r['orari'] else None),
                'errore': r['errore_dati'], 'secondi': r['secondi']}
    finally:
        shutil.rmtree(cartella, ignore_errors=True)


def confronta(riferimento, trovato):
    """Le differenze tra il riferimento accettato e un'esecuzione: [{campo, atteso, trovato}]."""
    return [{'campo': k, 'atteso': riferimento.get(k), 'trovato': trovato.get(k)} for k in CAMPI
            if riferimento.get(k) != trovato.get(k)]


def _cartella(config):
    d = os.path.join(config.lavoro, 'notturni')
    os.makedirs(d, exist_ok=True)
    return d


# ---- un giro ----

def giro_in_corso():
    with _lock:
        return _giro['id']


def test_in_corso():
    """L'id del test che sta girando adesso (o None)."""
    with _lock:
        return _giro['test']


def avvia_giro(config, scuola_id, motivo, soli=None):
    """Crea un giro e lo esegue in un thread. soli: gli id dei test da eseguire (default: tutti gli attivi).
    Ritorna l'id del giro, o None se ce n'e' gia' uno in corso (o non c'e' nessun test)."""
    with _lock:
        if _giro['id'] is not None:
            return None
        _giro.update(id=-1, fermare=False, test=None)  # occupato finche' il giro non e' nel database
    try:
        with db.connetti(config.database_url) as c:
            q = 'select id from test_notturni where scuola_id = %s and attivo'
            ids = [r['id'] for r in c.execute(q + ' order by id', (scuola_id,))]
            if soli is not None:
                ids = [i for i in ids if i in set(soli)]
            if not ids:
                with _lock:
                    _giro['id'] = None
                return None
            gid = c.execute("insert into giri_notturni (scuola_id, stato, motivo, commit) values (%s, 'in corso', %s, %s) "
                            "returning id", (scuola_id, motivo, solver.commit_attuale())).fetchone()['id']
            c.commit()
    except Exception:
        with _lock:
            _giro['id'] = None
        raise
    with _lock:
        _giro['id'] = gid
    threading.Thread(target=_esegui_giro, args=(config, gid, ids), daemon=True, name='notturni-%d' % gid).start()
    return gid


def ferma_giro():
    with _lock:
        if _giro['id'] is None:
            return False
        _giro['fermare'] = True
        e = _giro['esecuzione']
    if e:
        e.ferma()
    return True


def _esegui_giro(config, gid, ids):
    stato = 'finito'
    try:
        for tid in ids:
            with _lock:
                if _giro['fermare']:
                    stato = 'fermato'
                    break
                _giro['test'] = tid
            with db.connetti(config.database_url) as c:
                test = c.execute('select * from test_notturni where id = %s', (tid,)).fetchone()
                if not test:
                    continue
                nid = c.execute('insert into esecuzioni_notturne (giro_id, test_id) values (%s, %s) returning id',
                                (gid, tid)).fetchone()['id']
                c.commit()
            try:
                r = esegui_test(config, test)
            except Exception as ex:  # il test risulta in errore, il giro continua
                traceback.print_exc()
                r = {'esito': 'errore', 'errore': str(ex), 'secondi': None}
            _registra(config, nid, test, r)
    except Exception:
        traceback.print_exc()
        stato = 'interrotto'
    finally:
        with db.connetti(config.database_url) as c:
            c.execute('update giri_notturni set stato = %s, fine = now() where id = %s', (stato, gid))
            c.commit()
        with _lock:
            _giro.update(id=None, esecuzione=None, fermare=False, test=None)


def _registra(config, nid, test, r):
    """Scrive l'esecuzione e il confronto: senza riferimento i numeri diventano il riferimento (confronto 'nuovo')."""
    if r['esito'] in ('fermato', 'errore') or r['esito'] is None:
        confronto, diff = 'errore', None  # un test fermato a mano o in errore non dice niente sulla ricerca
    elif test['riferimento'] is None:
        confronto, diff = 'nuovo', None
    else:
        diff = confronta(test['riferimento'], r)
        confronto = 'diverso' if diff else 'uguale'
    with db.connetti(config.database_url) as c:
        c.execute('update esecuzioni_notturne set secondi = %s, esito = %s, nodi = %s, backtrack = %s, pos_max = %s, '
                  'orario = %s, errore = %s, confronto = %s, differenze = %s where id = %s',
                  (r.get('secondi'), r['esito'], r.get('nodi'), r.get('backtrack'), r.get('pos_max'), r.get('orario'),
                   r.get('errore'), confronto, _json(diff) if diff else None, nid))
        if confronto == 'nuovo':
            c.execute("update test_notturni set riferimento = %s, accettato = now(), accettato_da = 'prima esecuzione' "
                      "where id = %s", (_json(valori(r)), test['id']))
        c.commit()


def valori(r):
    """I campi che devono coincidere, da un'esecuzione (dict o riga del database)."""
    return {k: r.get(k) for k in CAMPI}


def accetta(c, nid, chi):
    """Accetta i numeri di un'esecuzione come nuovo riferimento del suo test."""
    n = c.execute('select * from esecuzioni_notturne where id = %s', (nid,)).fetchone()
    if not n or n['confronto'] not in ('diverso', 'accettato'):
        return False
    c.execute('update test_notturni set riferimento = %s, accettato = now(), accettato_da = %s where id = %s',
              (_json(valori(n)), chi, n['test_id']))
    c.execute("update esecuzioni_notturne set confronto = 'accettato' where id = %s", (nid,))
    return True


# ---- di notte ----

def avvia_sorveglianza(config):
    """Il thread che ogni minuto guarda se e' l'ora dei test notturni (una volta per processo del sito)."""
    global _sorveglianza_avviata
    if _sorveglianza_avviata:
        return
    _sorveglianza_avviata = True
    with db.connetti(config.database_url) as c:  # giri lasciati a meta' da un riavvio del sito
        c.execute("update esecuzioni_notturne set esito = 'interrotto', confronto = 'errore' where esito is null")
        c.execute("update giri_notturni set stato = 'interrotto', fine = now() where stato = 'in corso'")
        c.commit()
    threading.Thread(target=_sorveglia, args=(config,), daemon=True, name='notturni-sorveglianza').start()


def ora_impostata(c):
    r = c.execute("select valore from impostazioni where nome = 'ora_test_notturni'").fetchone()
    return (r['valore'] if r else '') or '02:00'


def _sorveglia(config):
    while True:
        try:
            _forse_avvia(config)
        except Exception:  # per esempio il database non raggiungibile: si riprova al minuto dopo
            traceback.print_exc()
        time.sleep(60)


def _forse_avvia(config, adesso=None):
    """Dall'ora impostata fino a sei ore dopo, per ogni scuola con test attivi e senza un giro di notte oggi: un giro.
    Le scuole vanno una dopo l'altra (un giro alla volta)."""
    adesso = adesso or datetime.datetime.now().astimezone()
    with db.connetti(config.database_url) as c:
        valore = ora_impostata(c)
        if valore == 'mai':
            return
        hh, mm = (int(x) for x in valore.split(':'))
        partenza = adesso.replace(hour=hh, minute=mm, second=0, microsecond=0)
        if not partenza <= adesso < partenza + datetime.timedelta(hours=6):
            return
        scuole = [r['scuola_id'] for r in c.execute(
            "select distinct t.scuola_id from test_notturni t where t.attivo and not exists ("
            "  select 1 from giri_notturni g where g.scuola_id = t.scuola_id and g.motivo = 'notte' and g.inizio >= %s)"
            " order by 1", (partenza,))]
    for sid in scuole:
        if giro_in_corso() is None:
            avvia_giro(config, sid, 'notte')
        return  # una scuola per volta: la prossima al controllo dopo la fine di questo giro
