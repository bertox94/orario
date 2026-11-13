"""I test in tempo reale: ognuno in una cartella lavoro/test/<numero> con info.json (i parametri e quando e' partito)
e stato.json (lo stato della ricerca, scritto dal solver), passi.bin (ogni nodo, per lo slider: vedi passi.h).
Un test alla volta; gli altri restano come storico."""
import array
import json
import os
import shutil
import subprocess
import threading
import time

from . import solver

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


def avvia(lavoro, seme, classi, ritardi, ordine, milioni):
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
                                               'ordine': ordine, 'limite': limite, 'inizio': time.time()})
        _in_corso['processo'] = subprocess.Popen(
            [exe, 'caso', str(seme), str(classi), '1' if ritardi else '0', str(ordine), str(limite), 'stato.json',
             'passi.bin'],
            cwd=d,
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
    try:  # quanti nodi si possono gia' rivedere: 6 byte l'uno nel file dei passi
        stato['passi'] = os.path.getsize(os.path.join(d, 'passi.bin')) // 6
    except OSError:
        stato['passi'] = 0
    return {'info': info, 'stato': stato}


def _solver(lavoro, argomenti, cartella=None):
    """Il JSON che il solver stampa con questi argomenti (i dati di un caso, un passo), o None."""
    r = subprocess.run([solver.compila(os.path.join(lavoro, 'bin'))] + [str(a) for a in argomenti], cwd=cartella,
                       capture_output=True, text=True, timeout=20)
    try:
        return json.loads(r.stdout) if r.returncode == 0 else None
    except ValueError:
        return None


def _caso(info):
    """Gli argomenti che dicono al solver di che caso si tratta: seme, classi, ritardi e ordine delle classi (0 nei
    test fatti prima che ci fosse)."""
    return [info['seme'], info['classi'], 1 if info['ritardi'] else 0, info.get('ordine', 0)]


def dati(lavoro, info):
    """I dati del caso (./untitled dati): giorni, ore, lezioni e ore di ingresso."""
    return _solver(lavoro, ['dati'] + _caso(info))


def ritorno(lavoro, tid, k, avanti):
    """Il primo passo dopo k (avanti) o prima di k dove la ricerca torna indietro, cioe' dove il passo dopo decide una
    cella che non viene dopo (vedi passi.h: tre interi a 16 bit per passo, il primo e' la cella). None se non c'e'.
    Legge il file dei passi a pezzi, dal punto di partenza in poi (o indietro)."""
    PEZZO = 65536  # passi letti per volta
    try:
        with open(os.path.join(_cartella(lavoro, tid), 'passi.bin'), 'rb') as f:
            passi = os.fstat(f.fileno()).st_size // 6

            def celle(da, a):  # la cella dei passi da `da` ad `a` compresi (da 1)
                f.seek((da - 1) * 6)
                return array.array('h', f.read((a - da + 1) * 6))[0::3]

            # il passo j e' un ritorno se la cella del passo j + 1 non viene dopo la sua: l'ultimo passo non lo e'
            j = k + 1 if avanti else k - 1
            while 1 <= j < passi:
                da, a = (j, min(j + PEZZO, passi)) if avanti else (max(1, j - PEZZO + 1), j + 1)
                c = celle(da, a)
                for i in (range(len(c) - 1) if avanti else range(len(c) - 2, -1, -1)):
                    if c[i + 1] <= c[i]:
                        return da + i
                j = a if avanti else da - 1
    except OSError:
        pass
    return None


def passo(lavoro, tid, k):
    """Il nodo k della ricerca (./untitled passo, vedi passo.h): l'orario fin li' e i controlli della cella."""
    info = _leggi(os.path.join(_cartella(lavoro, tid), 'info.json'))
    return info and _solver(lavoro, ['passo'] + _caso(info) + ['passi.bin', k], _cartella(lavoro, tid))


def storico(lavoro):
    """Tutti i test, dal piu' recente."""
    d = _cartella(lavoro)
    ids = sorted((int(x) for x in os.listdir(d) if x.isdigit()), reverse=True) if os.path.isdir(d) else []
    return [t for t in (test(lavoro, i) for i in ids) if t]


def elimina(lavoro, tid):
    if in_corso() == tid:
        raise ValueError('Il test sta girando: prima fermalo.')
    shutil.rmtree(_cartella(lavoro, tid), ignore_errors=True)
