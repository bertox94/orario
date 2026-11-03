"""Le marce indietro registrate con -b (orario.txt.gz nel database), lette a richiesta per vederle dal sito.

Il formato e' descritto in printer/printer.h: una riga "B pos da s_da ... s_(pos-1)" per ogni marcia indietro, con solo
gli slot cambiati rispetto alla precedente. Qui si ricostruisce l'orario parziale di una marcia scorrendo il file dal
principio (decompresso a pezzi, senza tenerlo in memoria)."""
import zlib

from . import archivio

# oltre questa quantita' di testo decompresso la pagina rinuncia (il file si puo' comunque scaricare)
MAX_BYTE = 400 * 2 ** 20
_totali = {}  # esecuzione -> numero di marce registrate (solo per file finiti)


class FileTroppoGrande(Exception):
    pass


def righe(url, eid):
    """Le righe di testo del file, decompresse a pezzi."""
    d = zlib.decompressobj(16 + zlib.MAX_WBITS)
    resto, letti = b'', 0
    for pezzo in archivio.pezzi(url, eid, 'orario.txt.gz'):
        while pezzo:
            dati = d.decompress(pezzo, 1 << 20)
            pezzo = d.unconsumed_tail
            letti += len(dati)
            if letti > MAX_BYTE:
                raise FileTroppoGrande()
            resto += dati
            *complete, resto = resto.split(b'\n')
            for r in complete:
                yield r.decode('ascii', 'replace')
    if resto:
        yield resto.decode('ascii', 'replace')


def marcia(url, eid, n, nomi_classi):
    """La n-esima marcia indietro (da 1; n oltre la fine = l'ultima). Ritorna un dizionario con:
        n, pos (lo slot dove non c'era nessun docente utilizzabile), blocco [g, o, c], orario [g][o][c] = [docente,
        codocente] con -1 negli slot non assegnati, e, se si e' arrivati alla fine del file, ultima = True."""
    giorni = ore = classi = 0
    ordine = []  # k-esima classe dell'ordine della ricerca -> indice nei dati del sito
    slot = []
    pos = visti = 0
    for r in righe(url, eid):
        if r.startswith('G '):
            _, giorni, ore, classi = r.split()[:4]
            giorni, ore, classi = int(giorni), int(ore), int(classi)
        elif r.startswith('K '):
            ordine = [nomi_classi.index(x) if x in nomi_classi else k for k, x in enumerate(r.split()[1:])]
        elif r.startswith('B '):
            try:
                campi = r.split()
                p, da = int(campi[1]), int(campi[2])
                nuovi = campi[3:]
                if len(nuovi) != p - da:
                    continue
            except (ValueError, IndexError):
                continue
            del slot[da:]
            slot += ['-'] * (da - len(slot))
            slot += nuovi
            pos = p
            visti += 1
            if visti == n:
                break
    if not visti or not classi:
        return None
    griglia = [[[[-1, -1] for _ in range(classi)] for _ in range(ore)] for _ in range(giorni)]
    for i, s in enumerate(slot[:pos]):
        if s == '-':
            continue
        g, resto = divmod(i, ore * classi)
        o, k = divmod(resto, classi)
        a, _, b = s.partition('+')
        griglia[g][o][ordine[k]] = [int(a), int(b) if b else -1]
    g, resto = divmod(pos, ore * classi) if pos < giorni * ore * classi else (giorni - 1, ore * classi - 1)
    o, k = divmod(resto, classi)
    return {'n': visti, 'pos': pos, 'slot': giorni * ore * classi, 'blocco': [g, o, ordine[k]], 'orario': griglia}


def totale(url, eid, definitivo):
    """Quante marce indietro sono registrate (si conta una volta sola per i file finiti)."""
    if eid in _totali:
        return _totali[eid]
    n = sum(1 for r in righe(url, eid) if r.startswith('B '))
    if definitivo:
        _totali[eid] = n
    return n
