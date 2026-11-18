"""I test in tempo reale: ognuno in una cartella lavoro/test/<numero> con info.json (i parametri e quando e' partito)
e stato.json (lo stato della ricerca, scritto dal solver), passi.bin (ogni nodo, per lo slider: vedi passi.h).
Un test alla volta; gli altri restano come storico."""
import array
import bisect
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
            j = k + 1 if avanti else min(k, passi) - 1   # l'ultimo passo non e' mai un ritorno
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


_lock_indice = threading.Lock()
_per_cella = {}   # (lavoro, test) -> [quanti ritorni gia' divisi, {cella: array dei suoi passi}]


def _indice_dei_ritorni(lavoro, tid):
    """I ritorni indietro della ricerca, in ordine: (passi, celle), due array lunghi uguali: il ritorno numero i e' al
    passo passi[i - 1], e la ricerca tornava indietro dalla cella celle[i - 1]. Per non rileggere tutti i passi ogni
    volta, l'indice sta accanto ai passi in due file: ritorni.idx (un intero a 64 bit con quanti passi sono gia' stati
    guardati, poi un intero a 32 bit per ritorno) e ritorni.celle (un intero a 16 bit per ritorno). Si allunga da solo
    mentre il test gira: si guardano solo i passi nuovi. Se i due file non vanno d'accordo si rifanno da capo.
    (Un passo e' un ritorno se quello dopo decide una cella che non viene dopo la sua: l'ultimo passo del file si
    giudica quando arriva il successivo.)"""
    d = _cartella(lavoro, tid)
    indice, celle = array.array('I'), array.array('h')
    try:
        with _lock_indice, open(os.path.join(d, 'passi.bin'), 'rb') as f:
            passi = os.fstat(f.fileno()).st_size // 6
            guardati = 0
            try:
                with open(os.path.join(d, 'ritorni.idx'), 'rb') as g, open(os.path.join(d, 'ritorni.celle'), 'rb') as h:
                    guardati = int.from_bytes(g.read(8), 'little')
                    indice.frombytes(g.read())
                    celle.frombytes(h.read())
            except (OSError, ValueError):
                pass
            if len(indice) != len(celle) or guardati > passi:   # file a meta', o passi rifatti: da capo
                indice, celle, guardati = array.array('I'), array.array('h'), 0
            da, gia_guardati = len(indice), guardati
            while guardati < passi - 1:   # i passi da guardati + 1 a passi - 1 si possono giudicare
                a = min(guardati + 65536, passi - 1)
                f.seek(guardati * 6)
                c = array.array('h', f.read((a - guardati + 1) * 6))[0::3]   # le celle dei passi guardati + 1 ... a + 1
                for i in range(len(c) - 1):
                    if c[i + 1] <= c[i]:
                        indice.append(guardati + 1 + i)
                        celle.append(c[i])
                guardati = a
            if guardati != gia_guardati or not os.path.exists(os.path.join(d, 'ritorni.idx')):
                with open(os.path.join(d, 'ritorni.celle'), 'wb' if not da else 'ab') as h:
                    h.write(celle[da:].tobytes())
                with open(os.path.join(d, 'ritorni.idx'), 'r+b' if da else 'wb') as g:
                    if da:
                        g.seek(0, os.SEEK_END)
                    else:
                        g.write(guardati.to_bytes(8, 'little'))
                    g.write(indice[da:].tobytes())
                    g.seek(0)
                    g.write(guardati.to_bytes(8, 'little'))
    except OSError:
        pass
    return indice, celle


def _ritorni_per_cella(lavoro, tid):
    """Per ogni cella da cui la ricerca e' tornata indietro, i passi dei suoi ritorni, in ordine: {cella: array}.
    Si tiene in memoria e si allunga con i ritorni nuovi."""
    indice, celle = _indice_dei_ritorni(lavoro, tid)
    with _lock_indice:
        divisi = _per_cella.setdefault((lavoro, tid), [0, {}])
        if divisi[0] > len(indice):
            divisi[0], divisi[1] = 0, {}
        for i in range(divisi[0], len(indice)):
            divisi[1].setdefault(celle[i], array.array('I')).append(indice[i])
        divisi[0] = len(indice)
        return divisi[1]


def celle_dei_ritorni(lavoro, tid):
    """Le celle da cui la ricerca e' tornata indietro, dalla piu' avanti: [{"cella", "quanti"}, ...]."""
    return [{'cella': c, 'quanti': len(v)} for c, v in sorted(_ritorni_per_cella(lavoro, tid).items(), reverse=True)]


def ritorni(lavoro, tid, cella, i=None, fino_a=None):
    """I ritorni indietro da una cella, contati da 1: {"quanti": quanti sono}, piu' "i" e "k" (il numero di un ritorno
    e il suo passo) se si chiede il ritorno numero i, oppure l'ultimo fino al passo fino_a (se ce n'e' uno)."""
    suoi = _ritorni_per_cella(lavoro, tid).get(cella, ())
    r = {'quanti': len(suoi)}
    if fino_a is not None:
        i = bisect.bisect_right(suoi, fino_a)
    if i is not None and 1 <= i <= len(suoi):
        r.update(i=i, k=suoi[i - 1])
    return r


FASCE, LUNGHI = 400, 12   # in quante parti si divide la ricerca per il profilo, e quanti ostacoli si elencano
SCRITTURA = 600           # di quanti nodi, al massimo, si scrive il percorso di un ostacolo
PERIODO = 12              # la ripetizione piu' lunga che si cerca nel percorso


def ostacoli(lavoro, tid):
    """I backtrack del test (./untitled ostacoli, vedi ostacoli.h): il profilo della ricerca in FASCE parti e i
    LUNGHI ostacoli che hanno richiesto piu' nodi. None se non ci sono passi."""
    return _solver(lavoro, ['ostacoli', 'passi.bin', FASCE, LUNGHI], _cartella(lavoro, tid))


def _raccogli(numeri):
    """Accorcia una fila di numeri raccogliendo i pezzi che si ripetono di seguito: [30, 29, 30, 29, 30, 28] ->
    [30, ([29, 30], 2), 28]. Da sinistra, a ogni punto si prende la ripetizione che copre piu' numeri, con un pezzo
    lungo al massimo PERIODO; se pero' partendo dal numero dopo se ne coprono altrettanti, il numero resta da solo e
    la ripetizione parte dopo (cosi' viene 30 (29 30)^2 e non (30 29)^2 30: prima la cella dell'incaglio, poi i
    giri). I pezzi raccolti non si raccolgono di nuovo."""
    def ripetizione(i):  # (numeri coperti, lunghezza del pezzo, volte) della ripetizione migliore che parte da i
        meglio = (0, 0, 0)
        for p in range(1, min(PERIODO, (len(numeri) - i) // 2) + 1):
            volte = 1
            while numeri[i + volte * p:i + (volte + 1) * p] == numeri[i:i + p]:
                volte += 1
            if volte > 1 and volte * p > meglio[0]:
                meglio = (volte * p, p, volte)
        return meglio

    fuori, i = [], 0
    while i < len(numeri):
        coperti, p, volte = ripetizione(i)
        if coperti and ripetizione(i + 1)[0] < coperti:
            fuori.append((numeri[i:i + p], volte))
            i += coperti
        else:
            fuori.append(numeri[i])
            i += 1
    return fuori


def scrittura(lavoro, tid, dal, al):
    """Il percorso della ricerca dentro un ostacolo, dal nodo `dal` (il primo ritorno) al nodo `al` (quello in cui la
    cella e' superata; None: non ancora), scritto con le celle su cui la ricerca lavora, da 1: la cella dove si
    incaglia, la cella piu' bassa a cui torna per cambiare scelta, poi una dopo l'altra quelle che rimette, fino a
    incagliarsi di nuovo o a passare. Un numero piu' basso del precedente e' un ritorno indietro. Per esempio
    30 29 30 31: si incaglia alla 30, cambia la 29, rimette la 30, arriva alla 31. Si scrivono al massimo SCRITTURA
    nodi. Ritorna {"pezzi": [numero | [[numeri], volte], ...], "tagliata": se l'ostacolo continua oltre}, o None."""
    try:
        with open(os.path.join(_cartella(lavoro, tid), 'passi.bin'), 'rb') as f:
            passi = os.fstat(f.fileno()).st_size // 6
            if not 1 <= dal <= passi:
                return None
            fine = min(al if al else passi, dal + SCRITTURA - 1, passi)
            f.seek((dal - 1) * 6)
            # una in piu' in fondo, se c'e': serve a sapere se l'ultimo nodo e' un ritorno
            pos = array.array('h', f.read((min(fine + 1, passi) - dal + 1) * 6))[0::3]
    except OSError:
        return None
    n = fine - dal + 1
    numeri = [pos[0] + 1]          # il primo nodo e' il ritorno: la cella dove si incaglia
    for i in range(1, n):
        numeri.append(pos[i])      # la cella appena messa (pos - 1, da 1)
        if (i + 1 < len(pos) and pos[i + 1] <= pos[i]) or (al and dal + i == al):
            numeri.append(pos[i] + 1)   # e quella dove si incaglia di nuovo, o dove arriva superato l'ostacolo
    return {'pezzi': [x if isinstance(x, int) else [x[0], x[1]] for x in _raccogli(numeri)],
            'tagliata': fine < (al if al else passi)}


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
