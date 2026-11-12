"""I test notturni: un elenco di casi (seme, classi, ritardi, ordine delle classi) che si rieseguono tutti insieme, in
un giro, per vedere se i loro numeri cambiano.

Tutto sta in lavoro/notturni/, in file JSON:
- test.json: l'elenco dei test, ognuno con il suo numero;
- impostazioni.json: il limite di nodi, uno solo per tutti i test (lo usa il giro, e si cambia dalla pagina);
- giri/<n>.json: un giro, con il limite usato e il risultato di ogni test (esito, nodi, cella piu' avanti, tempo).

Un giro esegue i test uno dopo l'altro con ./untitled caso (caso.h), in una cartella provvisoria: del test si tengono
solo i numeri, il file dei passi si cancella. Ogni risultato si confronta con l'ultimo dello stesso test fatto con lo
stesso limite: «uguale» o «diverso» (esito, nodi, cella piu' avanti; il tempo no, dipende dal computer). Con un
limite diverso i numeri non si confrontano: «limite cambiato». Un giro alla volta, e mai insieme a un test in tempo
reale.
"""
import json
import os
import shutil
import subprocess
import threading
import time

from . import solver, test_realtime

LIMITE = 10000000   # il limite di nodi finche' non lo si cambia dalla pagina
CONFRONTATI = ('esito', 'nodi', 'pos_max')

_lock = threading.Lock()
_giro = {'id': None, 'processo': None, 'fermare': False}  # il giro in corso


def _cartella(lavoro, *pezzi):
    return os.path.join(lavoro, 'notturni', *pezzi)


def _leggi(percorso, senza):
    try:
        with open(percorso) as f:
            return json.load(f)
    except (OSError, ValueError):
        return senza


def _scrivi(percorso, valore):
    os.makedirs(os.path.dirname(percorso), exist_ok=True)
    with open(percorso + '.tmp', 'w') as f:
        json.dump(valore, f)
    os.replace(percorso + '.tmp', percorso)


# ---- l'elenco dei test e il limite ----

def elenco(lavoro):
    return _leggi(_cartella(lavoro, 'test.json'), [])


def limite(lavoro):
    return _leggi(_cartella(lavoro, 'impostazioni.json'), {}).get('limite', LIMITE)


def cambia_limite(lavoro, nodi):
    _scrivi(_cartella(lavoro, 'impostazioni.json'), {'limite': nodi})


def aggiungi(lavoro, seme, classi, ritardi, ordine):
    """Aggiunge il caso ai test notturni, se non c'e' gia' (seme, classi, ritardi e ordine uguali). Ritorna il test, e
    se e' nuovo."""
    with _lock:
        tutti = elenco(lavoro)
        for t in tutti:
            if (t['seme'], t['classi'], t['ritardi'], t['ordine']) == (seme, classi, ritardi, ordine):
                return t, False
        t = {'id': max((x['id'] for x in tutti), default=0) + 1, 'seme': seme, 'classi': classi, 'ritardi': ritardi,
             'ordine': ordine, 'aggiunto': time.time()}
        _scrivi(_cartella(lavoro, 'test.json'), tutti + [t])
        return t, True


def togli(lavoro, tid):
    with _lock:
        _scrivi(_cartella(lavoro, 'test.json'), [t for t in elenco(lavoro) if t['id'] != tid])


# ---- i giri ----

def _giri(lavoro):
    """I numeri dei giri fatti, in ordine."""
    d = _cartella(lavoro, 'giri')
    return sorted(int(x[:-5]) for x in os.listdir(d) if x.endswith('.json') and x[:-5].isdigit()) \
        if os.path.isdir(d) else []


def giro(lavoro, gid):
    return _leggi(_cartella(lavoro, 'giri', '%d.json' % gid), None)


def ultimo_giro(lavoro):
    numeri = _giri(lavoro)
    return giro(lavoro, numeri[-1]) if numeri else None


def in_corso():
    """Il numero del giro che sta girando (o None)."""
    with _lock:
        return _giro['id']


def _prima(lavoro, tid, nodi, fino_a):
    """L'ultimo risultato del test tid, nei giri prima di fino_a: (risultato, limite di quel giro), con preferenza per
    un giro fatto con il limite `nodi` (quello con cui si confronta). (None, None) se il test non ha mai girato."""
    altro = (None, None)
    for gid in reversed([g for g in _giri(lavoro) if g < fino_a]):
        g = giro(lavoro, gid) or {}
        r = g.get('risultati', {}).get(str(tid))
        if not r:
            continue
        if g.get('limite') == nodi:
            return r, nodi
        if altro[0] is None:
            altro = (r, g.get('limite'))
    return altro


def _confronto(lavoro, tid, nodi, gid, r):
    prima, limite_prima = _prima(lavoro, tid, nodi, gid)
    if prima is None:
        return 'primo'
    if limite_prima != nodi:
        return 'limite cambiato'
    return 'uguale' if all(prima.get(k) == r.get(k) for k in CONFRONTATI) else 'diverso'


def _esegui_test(lavoro, exe, t, nodi):
    """Un test: il solver in una cartella provvisoria, da cui si leggono i numeri. None se il giro e' stato fermato."""
    d = _cartella(lavoro, 'provvisoria')
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    try:
        p = subprocess.Popen([exe, 'caso', str(t['seme']), str(t['classi']), '1' if t['ritardi'] else '0',
                              str(t['ordine']), str(nodi), 'stato.json', 'passi.bin'], cwd=d,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with _lock:
            _giro['processo'] = p
        p.wait()
        with _lock:
            _giro['processo'] = None
            if _giro['fermare']:
                return None
        s = _leggi(os.path.join(d, 'stato.json'), {})
        if s.get('in_corso', 1):
            return {'esito': 'non riuscito'}
        r = {k: s.get(k) for k in ('esito', 'nodi', 'pos_max', 'celle', 'secondi')}
        if s.get('errore'):
            r['errore'] = s['errore']
        if 'verifica' in s:
            r['verificato'] = bool(s['verifica']['ok'])
        return r
    finally:
        shutil.rmtree(d, ignore_errors=True)


def _esegui_giro(lavoro, gid, nodi, tests):
    percorso = _cartella(lavoro, 'giri', '%d.json' % gid)
    g = giro(lavoro, gid)
    try:
        exe = solver.compila(os.path.join(lavoro, 'bin'))
        for t in tests:
            g['adesso'] = t['id']
            _scrivi(percorso, g)
            r = _esegui_test(lavoro, exe, t, nodi)
            if r is None:
                g['stato'] = 'fermato'
                break
            r['confronto'] = _confronto(lavoro, t['id'], nodi, gid, r)
            g['risultati'][str(t['id'])] = r
        else:
            g['stato'] = 'finito'
    except Exception as e:  # il solver che non compila, il disco pieno...: il giro finisce e lo dice
        g['stato'] = 'errore: %s' % e
    finally:
        g['adesso'] = None
        g['fine'] = time.time()
        _scrivi(percorso, g)
        with _lock:
            _giro.update(id=None, processo=None, fermare=False)


def avvia_giro(lavoro):
    """Lancia un giro su tutti i test dell'elenco e ritorna il suo numero. ValueError se non si puo'."""
    with _lock:
        if _giro['id'] is not None:
            raise ValueError('C\'è già un giro in corso.')
        if test_realtime.in_corso() is not None:
            raise ValueError('Sta girando un test in tempo reale: aspetta che finisca, o fermalo.')
        tests = elenco(lavoro)
        if not tests:
            raise ValueError('Non ci sono test notturni da eseguire.')
        gid = max(_giri(lavoro), default=0) + 1
        nodi = limite(lavoro)
        _scrivi(_cartella(lavoro, 'giri', '%d.json' % gid),
                {'id': gid, 'inizio': time.time(), 'fine': None, 'limite': nodi, 'stato': 'in corso', 'adesso': None,
                 'test': [t['id'] for t in tests], 'risultati': {}})
        _giro.update(id=gid, processo=None, fermare=False)
    threading.Thread(target=_esegui_giro, args=(lavoro, gid, nodi, tests), daemon=True).start()
    return gid


def ferma_giro():
    """Ferma il giro in corso: il test che sta girando si interrompe, quelli gia' fatti restano."""
    with _lock:
        if _giro['id'] is None:
            return
        _giro['fermare'] = True
        if _giro['processo'] is not None:
            _giro['processo'].terminate()


def chiudi_giri_interrotti(lavoro):
    """All'avvio del sito: un giro rimasto «in corso» (il sito si e' fermato mentre girava) risulta «interrotto»."""
    for gid in _giri(lavoro):
        g = giro(lavoro, gid)
        if g and g.get('stato') == 'in corso':
            g.update(stato='interrotto', adesso=None, fine=g.get('fine') or time.time())
            _scrivi(_cartella(lavoro, 'giri', '%d.json' % gid), g)


def situazione(lavoro):
    """Per la pagina: il limite, il giro in corso (o l'ultimo) e i test, ognuno con il suo ultimo risultato."""
    g = ultimo_giro(lavoro)
    nodi = limite(lavoro)
    quale = max(_giri(lavoro), default=0) + 1
    tests = []
    for t in elenco(lavoro):
        r = (g or {}).get('risultati', {}).get(str(t['id']))
        if r is None:  # non e' nell'ultimo giro (aggiunto dopo, o giro fermato prima): l'ultimo risultato che ha
            r, limite_prima = _prima(lavoro, t['id'], nodi, quale)
            if r is not None and limite_prima != nodi:
                r = dict(r, limite=limite_prima)
        tests.append(dict(t, risultato=r))
    return {'limite': nodi, 'giro': g, 'in_corso': in_corso(), 'tests': tests,
            'realtime': test_realtime.in_corso()}
