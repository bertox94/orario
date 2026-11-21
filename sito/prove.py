"""Le prove in tempo reale: una ricerca che si segue mentre va e si rivede passo per passo. Ognuna sta in
lavoro/prove/<numero>/, con info.json (il caso e quando e' partita), caso.txt (il caso, come lo legge il solver),
stato.json (lo stato della ricerca, scritto dal solver) e passi.bin (ogni nodo: passi.py). Una alla volta."""
import os
import shutil
import threading
import time

from . import archivio, passi, solver

BYTE_PER_NODO = 19                  # 18 il record del nodo, il resto istantanee e profilo (solver/passi.h)
SPAZIO_DA_LASCIARE = 500 * 1000000  # i byte che sul disco devono restare liberi comunque
_lock = threading.Lock()                         # le richieste arrivano da piu' fili insieme: una alla volta qui
_in_corso = {'numero': None, 'processo': None}   # la prova che sta girando e il processo del solver


def _cartella(numero=None):
    return archivio.percorso('prove') if numero is None else archivio.percorso('prove', numero)


def file_dei_passi(numero):
    return os.path.join(_cartella(numero), 'passi.bin')


def in_corso():
    """Il numero della prova che sta girando, o None."""
    with _lock:
        p = _in_corso['processo']
        return _in_corso['numero'] if p is not None and p.poll() is None else None


def avvia(seme, classi, ritardi, limite):
    """Lancia una prova e ritorna il suo numero. ValueError se un'altra sta girando o il caso non si genera."""
    with _lock:
        p = _in_corso['processo']
        if p is not None and p.poll() is None:
            raise ValueError('C\'è già una prova in corso: aspetta che finisca, o fermala.')
        # Ogni nodo si scrive su file (BYTE_PER_NODO l'uno, con istantanee e profilo): se il disco non basta per
        # tutti i nodi del limite, meglio dirlo subito che riempirlo.
        os.makedirs(_cartella(), exist_ok=True)
        servono, liberi = limite * BYTE_PER_NODO, shutil.disk_usage(_cartella()).free
        if servono > liberi - SPAZIO_DA_LASCIARE:
            raise ValueError('Per registrare %s nodi servono %.1f GB, e sul disco ce ne sono %.1f liberi: scegli un '
                             'limite più basso, o elimina delle prove.'
                             % ('{:,}'.format(limite).replace(',', '.'), servono / 1e9, liberi / 1e9))
        numero = max(archivio.numeri(_cartella()), default=0) + 1   # il numero dopo l'ultimo
        d = _cartella(numero)
        os.makedirs(d)
        try:
            solver.genera(seme, classi, ritardi, os.path.join(d, 'caso.txt'))
        except ValueError:
            shutil.rmtree(d, ignore_errors=True)
            raise
        passi.dimentica(file_dei_passi(numero))   # il numero di una prova eliminata si riusa
        archivio.scrivi(os.path.join(d, 'info.json'), {'numero': numero, 'seme': seme, 'classi': classi,
                                                      'ritardi': ritardi, 'limite': limite, 'inizio': time.time()})
        processo = solver.risolvi('caso.txt', limite, 'stato.json', 'passi.bin', d)
        _in_corso.update(numero=numero, processo=processo)
        return numero


def ferma():
    """Ferma la prova in corso: risulta «fermata», con lo stato dell'ultimo mezzo secondo."""
    with _lock:
        p, numero = _in_corso['processo'], _in_corso['numero']
        if p is None or p.poll() is not None:
            return
        info = archivio.leggi(os.path.join(_cartella(numero), 'info.json'), {})
        archivio.scrivi(os.path.join(_cartella(numero), 'info.json'), dict(info, fermata=True))
        p.terminate()


def prova(numero):
    """Una prova: {"info", "stato"}; nello stato anche «gira» (il processo e' vivo) e «passi» (quanti nodi si
    possono gia' rivedere). None se non c'e'."""
    d = _cartella(numero)
    info = archivio.leggi(os.path.join(d, 'info.json'))
    if not info:
        return None
    stato = archivio.leggi(os.path.join(d, 'stato.json'), {})
    gira = in_corso() == numero
    if not gira and stato.get('in_corso', 1):   # finita senza dirlo: fermata, o il sito si e' riavviato
        stato = dict(stato, in_corso=0, esito='fermata' if info.get('fermata') else 'interrotta')
    return {'info': info, 'stato': dict(stato, gira=gira, passi=passi.quanti(file_dei_passi(numero)))}


def nodo(numero, k):
    """Quello che il solver dice del nodo k (solver.nodo): i conti di ogni docente e, se da li' la ricerca non va
    avanti, il conto che non torna in dettaglio e le sue celle. None se la prova o il nodo non ci sono."""
    p = prova(numero)
    x = p and passi.nodo_per_il_solver(file_dei_passi(numero), k, p['stato'].get('celle', 0))
    return solver.nodo(os.path.join(_cartella(numero), 'caso.txt'), x) if x else None


def tutte():
    """Tutte le prove, dalla piu' recente."""
    return [p for p in (prova(n) for n in reversed(archivio.numeri(_cartella()))) if p]


def elimina(numero):
    if in_corso() == numero:
        raise ValueError('La prova sta girando: prima fermala.')
    shutil.rmtree(_cartella(numero), ignore_errors=True)
    passi.dimentica(file_dei_passi(numero))
