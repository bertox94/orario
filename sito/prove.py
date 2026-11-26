"""Le prove: un caso, e il suo orario. Ognuna sta in lavoro/prove/<numero>/, con info.json (il caso e quando e'
partita), caso.txt (il caso, come lo legge il programma) e stato.json (lo stato, scritto dal programma: orario/).
Una alla volta."""
import os
import shutil
import threading
import time

from . import archivio, solver

TEMPO = 300   # i secondi che il programma ha per un caso
# le richieste arrivano da piu' fili insieme: una alla volta qui (rientrante: chi lo tiene puo' chiamare prova())
_lock = threading.RLock()
_in_corso = {'numero': None, 'processo': None}   # la prova che sta girando e il suo processo


def _cartella(numero=None):
    return archivio.percorso('prove') if numero is None else archivio.percorso('prove', numero)


def in_corso():
    """Il numero della prova che sta girando, o None."""
    with _lock:
        p = _in_corso['processo']
        return _in_corso['numero'] if p is not None and p.poll() is None else None


def avvia(seme, classi, ritardi):
    """Lancia una prova e ritorna il suo numero. ValueError se un'altra sta girando o il caso non si genera."""
    with _lock:
        p = _in_corso['processo']
        if p is not None and p.poll() is None:
            raise ValueError('C\'è già una prova in corso: aspetta che finisca, o fermala.')
        numero = max(archivio.numeri(_cartella()), default=0) + 1   # il numero dopo l'ultimo
        d = _cartella(numero)
        os.makedirs(d)
        try:
            solver.genera(seme, classi, ritardi, os.path.join(d, 'caso.txt'))
        except ValueError:
            shutil.rmtree(d, ignore_errors=True)
            raise
        archivio.scrivi(os.path.join(d, 'info.json'), {'numero': numero, 'seme': seme, 'classi': classi,
                                                      'ritardi': ritardi, 'tempo': TEMPO, 'inizio': time.time()})
        processo = solver.risolvi(os.path.join(d, 'caso.txt'), os.path.join(d, 'stato.json'), TEMPO)
        _in_corso.update(numero=numero, processo=processo)
        return numero


def ferma():
    """Ferma la prova in corso: risulta «fermata»."""
    with _lock:
        p, numero = _in_corso['processo'], _in_corso['numero']
        if p is None or p.poll() is not None:
            return
        info = archivio.leggi(os.path.join(_cartella(numero), 'info.json'), {})
        # si segna che e' stata fermata a mano: senza, risulterebbe «interrotta»
        archivio.scrivi(os.path.join(_cartella(numero), 'info.json'), dict(info, fermata=True))
        p.terminate()


def prova(numero):
    """Una prova: {"info", "stato"}; nello stato anche «gira» (il processo e' vivo). None se non c'e'."""
    d = _cartella(numero)
    info = archivio.leggi(os.path.join(d, 'info.json'))
    if not info:
        return None
    # lo stato lo scrive il programma; se non c'e' ancora, vuoto
    stato = archivio.leggi(os.path.join(d, 'stato.json'), {})
    gira = in_corso() == numero
    if not gira and stato.get('in_corso', 1):   # finita senza dirlo: fermata, o il sito si e' riavviato
        stato = dict(stato, in_corso=0, esito='fermata' if info.get('fermata') else 'interrotta')
    return {'info': info, 'stato': dict(stato, gira=gira)}


def tutte():
    """Tutte le prove, dalla piu' recente."""
    return [p for p in (prova(n) for n in reversed(archivio.numeri(_cartella()))) if p]


def elimina(numero):
    if in_corso() == numero:
        raise ValueError('La prova sta girando: prima fermala.')
    shutil.rmtree(_cartella(numero), ignore_errors=True)
