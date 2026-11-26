"""Le prove in tempo reale: una ricerca che si segue mentre va e si rivede passo per passo. Ognuna sta in
lavoro/prove/<numero>/, con info.json (il caso e quando e' partita), caso.txt (il caso, come lo legge il solver),
stato.json (lo stato della ricerca, scritto dal solver) e passi.bin (ogni nodo: passi.py). Una alla volta."""
import os
import shutil
import threading
import time

from . import archivio, passi, solver

TEMPO_ESATTO = 300                  # i secondi che il metodo esatto ha per un caso
BYTE_PER_NODO = 19                  # 18 il record del nodo, il resto istantanee e profilo (passi/passi.h)
SPAZIO_DA_LASCIARE = 500 * 1000000  # i byte che sul disco devono restare liberi comunque
# le richieste arrivano da piu' fili insieme: una alla volta qui (rientrante: chi lo tiene puo' chiamare prova())
_lock = threading.RLock()
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


def _controlla_il_disco(limite):
    """Ogni nodo si scrive su file (BYTE_PER_NODO l'uno, con istantanee e profilo): se il disco non basta per tutti i
    nodi del limite, meglio dirlo subito che riempirlo. ValueError se non basta."""
    os.makedirs(_cartella(), exist_ok=True)
    servono, liberi = limite * BYTE_PER_NODO, shutil.disk_usage(_cartella()).free
    if servono > liberi - SPAZIO_DA_LASCIARE:
        raise ValueError('Per registrare %s nodi servono %.1f GB, e sul disco ce ne sono %.1f liberi: scegli un '
                         'limite più basso, o elimina delle prove.'
                         % ('{:,}'.format(limite).replace(',', '.'), servono / 1e9, liberi / 1e9))


def avvia(seme, classi, ritardi, limite, metodo='ricerca'):
    """Lancia una prova e ritorna il suo numero. `metodo`: 'ricerca' (la ricerca del solver, che si rivede passo
    per passo, fino a `limite` nodi) o 'esatto' (il metodo esatto: solo il risultato, in al massimo TEMPO_ESATTO
    secondi). ValueError se un'altra sta girando o il caso non si genera."""
    with _lock:
        p = _in_corso['processo']
        if p is not None and p.poll() is None:
            raise ValueError('C\'è già una prova in corso: aspetta che finisca, o fermala.')
        if metodo == 'esatto' and not solver.esatto_disponibile():
            raise ValueError('Per il metodo esatto serve scipy: pip install -r sito/requirements.txt')
        if metodo != 'esatto':   # il metodo esatto non registra passi: non occupa disco
            _controlla_il_disco(limite)
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
                                                      'ritardi': ritardi, 'limite': limite, 'inizio': time.time(),
                                                      'metodo': metodo})
        if metodo == 'esatto':
            processo = solver.esatto('caso.txt', 'stato.json', TEMPO_ESATTO, d)
        else:
            processo = solver.risolvi('caso.txt', limite, 'stato.json', 'passi.bin', d)
        _in_corso.update(numero=numero, processo=processo)
        return numero


# ---- cambiare a mano la scelta di una cella ----

def _strada_fino_a(numero, k, cella):
    """Per cambiare la scelta di una cella al passo k: (la prova, le scelte fatte PRIMA di quella cella, la voce che
    c'e' adesso). ValueError se la prova o il passo non ci sono, o a quel passo la cella non e' decisa."""
    p = prova(numero)
    tutta = p and passi.strada(file_dei_passi(numero), k, p['stato'].get('celle', 0))
    posto = next((i for i, (c, _) in enumerate(tutta or []) if c == cella), None)
    if posto is None:
        raise ValueError('A quel passo quella cella non è ancora decisa: non c\'è una scelta da cambiare.')
    return p, tutta[:posto], tutta[posto][1]


def scelte(numero, k, cella):
    """Le scelte che la ricerca aveva per quella cella, nel punto in cui l'ha decisa lungo la strada del passo k:
    {"scelte": [voce | -1, ...] nell'ordine in cui le prova, "adesso": quella che c'e'}. ValueError se non si puo'."""
    _, prima, adesso = _strada_fino_a(numero, k, cella)
    x = solver.scelte(os.path.join(_cartella(numero), 'caso.txt'), passi.strada_per_il_solver(prima))
    if not x or x['cella'] != cella:
        raise ValueError('Non riesco a ritrovare le scelte di quella cella.')
    return {'scelte': x['scelte'], 'adesso': adesso}


def cambia(numero, k, cella, voce):
    """Una prova nuova sullo stesso caso, che parte dall'orario del passo k fino a quella cella (esclusa), con in
    quella cella la voce scelta a mano, e da li' cerca, con lo stesso limite di nodi. Ritorna il numero della prova
    nuova. ValueError se non si puo'."""
    with _lock:
        p = _in_corso['processo']
        if p is not None and p.poll() is None:
            raise ValueError('C\'è già una prova in corso: aspetta che finisca, o fermala.')
        vecchia, prima, _ = _strada_fino_a(numero, k, cella)
        if voce not in scelte(numero, k, cella)['scelte']:
            raise ValueError('Quella non è una delle scelte possibili per quella cella.')
        info = vecchia['info']
        _controlla_il_disco(info['limite'])
        nuova = max(archivio.numeri(_cartella()), default=0) + 1
        d = _cartella(nuova)
        os.makedirs(d)
        # lo stesso caso della prova di partenza, e la strada: le scelte fino a quella cella, poi quella fatta a mano
        shutil.copy(os.path.join(_cartella(numero), 'caso.txt'), os.path.join(d, 'caso.txt'))
        with open(os.path.join(d, 'strada.txt'), 'w') as f:
            f.write(passi.strada_per_il_solver(prima + [(cella, voce)]) + '\n')
        passi.dimentica(file_dei_passi(nuova))
        archivio.scrivi(os.path.join(d, 'info.json'),
                        dict(info, numero=nuova, inizio=time.time(), fermata=False,
                             a_mano={'prova': numero, 'passo': k, 'cella': cella, 'voce': voce}))
        processo = solver.risolvi('caso.txt', info['limite'], 'stato.json', 'passi.bin', d, da='strada.txt')
        _in_corso.update(numero=nuova, processo=processo)
        return nuova


def continua(numero, in_piu):
    """Manda avanti di altri `in_piu` nodi una prova che il limite di nodi ha fermato: riprende dal punto esatto in
    cui era, e i passi nuovi si aggiungono a quelli che ci sono. Ritorna quanti nodi aveva. ValueError se non si
    puo'."""
    with _lock:
        p = _in_corso['processo']
        if p is not None and p.poll() is None:
            raise ValueError('C\'è già una prova in corso: aspetta che finisca, o fermala.')
        d = _cartella(numero)
        info, stato = archivio.leggi(os.path.join(d, 'info.json')), archivio.leggi(os.path.join(d, 'stato.json'), {})
        if not info or stato.get('esito') != 'limite di nodi' or info.get('fermata'):
            raise ValueError('Si può continuare solo una prova che si è fermata al limite di nodi.')
        if not os.path.exists(file_dei_passi(numero) + '.fine'):   # la scrive il solver da quando sa continuare
            raise ValueError('Questa prova è stata fatta prima che si potesse continuare: rifalla.')
        if in_piu * BYTE_PER_NODO > shutil.disk_usage(d).free - SPAZIO_DA_LASCIARE:
            raise ValueError('Sul disco non c\'è abbastanza spazio per continuare.')
        # il limite nuovo: i nodi fatti piu' quelli chiesti (il solver vuole il totale)
        limite = stato['nodi'] + in_piu
        archivio.scrivi(os.path.join(d, 'info.json'), dict(info, limite=limite))
        processo = solver.risolvi('caso.txt', limite, 'stato.json', 'passi.bin', d, continua=True)
        _in_corso.update(numero=numero, processo=processo)
        return stato['nodi']


def ferma():
    """Ferma la prova in corso: risulta «fermata», con lo stato dell'ultimo mezzo secondo."""
    with _lock:
        p, numero = _in_corso['processo'], _in_corso['numero']
        if p is None or p.poll() is not None:
            return
        info = archivio.leggi(os.path.join(_cartella(numero), 'info.json'), {})
        # si segna che e' stata fermata a mano: senza, risulterebbe «interrotta»
        archivio.scrivi(os.path.join(_cartella(numero), 'info.json'), dict(info, fermata=True))
        p.terminate()


def prova(numero):
    """Una prova: {"info", "stato"}; nello stato anche «gira» (il processo e' vivo) e «passi» (quanti nodi si
    possono gia' rivedere). None se non c'e'."""
    d = _cartella(numero)
    info = archivio.leggi(os.path.join(d, 'info.json'))
    if not info:
        return None
    # lo stato lo scrive il solver; se non c'e' ancora, vuoto
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
