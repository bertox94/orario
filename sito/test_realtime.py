"""I test in tempo reale: ognuno in una cartella lavoro/test/<numero> con info.json (i parametri e quando e' partito)
e stato.json (lo stato della ricerca, scritto dal solver), passi.bin (ogni nodo, per lo slider: vedi passi.py).
Un test alla volta; gli altri restano come storico."""
import json
import os
import shutil
import subprocess
import threading
import time

from . import passi, solver

_lock = threading.Lock()
_in_corso = {'id': None, 'processo': None}


def _cartella(lavoro, tid=None):
    d = os.path.join(lavoro, 'test')
    return d if tid is None else os.path.join(d, str(tid))


def _leggi(percorso):
    try:
        with open(percorso) as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def _scrivi(percorso, valore):
    with open(percorso + '.tmp', 'w') as f:
        json.dump(valore, f)
    os.replace(percorso + '.tmp', percorso)


def in_corso():
    """L'id del test che gira (o None)."""
    with _lock:
        p = _in_corso['processo']
        return _in_corso['id'] if p is not None and p.poll() is None else None


def avvia(lavoro, seme, classi, ritardi, milioni):
    """Lancia un test e ritorna il suo id. ValueError se un altro test sta girando."""
    with _lock:
        p = _in_corso['processo']
        if p is not None and p.poll() is None:
            raise ValueError('C\'è già un test in corso: aspetta che finisca, o fermalo.')
        exe = solver.compila(os.path.join(lavoro, 'bin'))
        os.makedirs(_cartella(lavoro), exist_ok=True)
        ids = [int(x) for x in os.listdir(_cartella(lavoro)) if x.isdigit()]
        tid = max(ids, default=0) + 1
        d = _cartella(lavoro, tid)
        os.makedirs(d)
        limite = int(round(milioni * 1000000))
        _scrivi(os.path.join(d, 'info.json'), {'id': tid, 'seme': seme, 'classi': classi, 'ritardi': ritardi,
                                               'limite': limite, 'inizio': time.time()})
        _in_corso['processo'] = subprocess.Popen(
            [exe, 'caso', str(seme), str(classi), '1' if ritardi else '0', str(limite), 'stato.json', 'passi.bin'], cwd=d,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        _in_corso['id'] = tid
        return tid


def ferma(lavoro):
    """Ferma il test in corso: risulta «fermato» (con lo stato dell'ultimo mezzo secondo)."""
    with _lock:
        p, tid = _in_corso['processo'], _in_corso['id']
        if p is None or p.poll() is not None:
            return
        info = _leggi(os.path.join(_cartella(lavoro, tid), 'info.json')) or {}
        info['fermato'] = True
        _scrivi(os.path.join(_cartella(lavoro, tid), 'info.json'), info)
        p.terminate()


def test(lavoro, tid):
    """Un test: info e stato (con «gira»: se il processo e' vivo). None se non c'e'."""
    d = _cartella(lavoro, tid)
    info = _leggi(os.path.join(d, 'info.json'))
    if not info:
        return None
    stato = _leggi(os.path.join(d, 'stato.json')) or {}
    gira = in_corso() == tid
    if not gira and stato.get('in_corso', 1):  # finito senza scrivere la fine: fermato, o il sito si e' riavviato
        stato = dict(stato, esito='fermato' if info.get('fermato') else 'interrotto', in_corso=0)
    stato['gira'] = gira
    stato['passi'] = passi.conta(os.path.join(d, 'passi.bin'))[0]
    return {'info': info, 'stato': stato}


def percorso_passi(lavoro, tid):
    return os.path.join(_cartella(lavoro, tid), 'passi.bin')


def storico(lavoro):
    """Tutti i test, dal piu' recente."""
    d = _cartella(lavoro)
    ids = sorted((int(x) for x in os.listdir(d) if x.isdigit()), reverse=True) if os.path.isdir(d) else []
    return [t for t in (test(lavoro, i) for i in ids) if t]


def elimina(lavoro, tid):
    if in_corso() == tid:
        raise ValueError('Il test sta girando: prima fermalo.')
    shutil.rmtree(_cartella(lavoro, tid), ignore_errors=True)
