"""Le prove: un caso, e il suo orario. Il caso e' casuale (seme, classi, ritardi) oppure quello di una scuola scritta
a mano (scuole.py). Ognuna sta in lavoro/prove/<numero>/, con info.json (di che caso si tratta e quando e'
partita), caso.txt (il caso, come lo legge il programma) e stato.json (lo stato, scritto dal programma: orario/).
Una alla volta.

Il percorso di una prova:
1. avvia(): si crea la cartella, ci si scrive il caso, si lancia il programma (un processo a parte);
2. il programma scrive stato.json: subito «in corso», poi il risultato;
3. prova(): la pagina chiede lo stato ogni secondo, finche' il processo non e' finito;
4. se il processo finisce senza un risultato la prova risulta «fermata» (da ferma()) o «interrotta»."""
import os
import shutil
import threading
import time

from . import archivio, programma

TEMPO = 300   # i secondi che il programma ha per un caso
# le richieste arrivano da piu' fili insieme: una alla volta qui (rientrante: chi lo tiene puo' chiamare prova())
_lock = threading.RLock()
_in_corso = {'numero': None, 'processo': None}   # la prova che sta girando e il suo processo


def _cartella(numero=None):
    """La cartella di una prova, o quella di tutte."""
    return archivio.percorso('prove') if numero is None else archivio.percorso('prove', numero)


def _file(numero, nome):
    return os.path.join(_cartella(numero), nome)


def in_corso():
    """Il numero della prova che sta girando, o None."""
    with _lock:
        return _in_corso['numero'] if programma.vivo(_in_corso['processo']) else None


def avvia(caso, testo):
    """Lancia una prova sul caso scritto in `testo` e ritorna il suo numero. `caso` dice di che caso si tratta, e
    finisce in info.json: {"seme", "classi", "ritardi"} per un caso casuale, {"scuola", "nome"} per una scuola.
    ValueError se un'altra prova sta girando."""
    with _lock:
        if programma.vivo(_in_corso['processo']):
            raise ValueError('C\'è già una prova in corso: aspetta che finisca, o fermala.')
        # 1. la cartella, con il numero dopo l'ultimo
        numero = max(archivio.numeri(_cartella()), default=0) + 1
        os.makedirs(_cartella(numero))
        # 2. il caso, e di che caso si tratta
        programma.scrivi_caso(testo, _file(numero, 'caso.txt'))
        archivio.scrivi(_file(numero, 'info.json'), dict(caso, numero=numero, tempo=TEMPO, inizio=time.time()))
        # 3. il programma, in un processo a parte: da qui in poi lo stato lo scrive lui
        processo = programma.risolvi(_file(numero, 'caso.txt'), _file(numero, 'stato.json'), TEMPO)
        _in_corso.update(numero=numero, processo=processo)
        return numero


def ferma():
    """Ferma la prova in corso: risulta «fermata»."""
    with _lock:
        if not programma.vivo(_in_corso['processo']):
            return
        numero = _in_corso['numero']
        # si segna che e' stata fermata a mano (senza, risulterebbe «interrotta»), poi si chiude il processo
        info = archivio.leggi(_file(numero, 'info.json'), {})
        archivio.scrivi(_file(numero, 'info.json'), dict(info, fermata=True))
        _in_corso['processo'].terminate()


def prova(numero):
    """Una prova: {"info", "stato"}; nello stato anche «gira» (il processo e' vivo). None se non c'e'."""
    info = archivio.leggi(_file(numero, 'info.json'))
    if not info:
        return None
    # lo stato lo scrive il programma; se non c'e' ancora, vuoto
    stato = archivio.leggi(_file(numero, 'stato.json'), {})
    gira = in_corso() == numero
    if not gira and stato.get('in_corso', 1):   # finita senza dirlo: fermata, o il sito si e' riavviato
        stato = dict(stato, in_corso=0, esito='fermata' if info.get('fermata') else 'interrotta')
    return {'info': info, 'stato': dict(stato, gira=gira)}


def tutte():
    """Tutte le prove, dalla piu' recente."""
    return [p for p in (prova(n) for n in reversed(archivio.numeri(_cartella()))) if p]


def elimina(numero):
    """Butta la cartella della prova. ValueError se sta girando."""
    if in_corso() == numero:
        raise ValueError('La prova sta girando: prima fermala.')
    shutil.rmtree(_cartella(numero), ignore_errors=True)
