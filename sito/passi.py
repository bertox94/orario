"""I passi di una ricerca, letti dal file che scrive il solver (solver/passi.h): un record di 9 interi a 16 bit per
nodo (profondita, cella, voce, e il motivo per cui da li' non si va avanti: tipo, docente, classe, ora, servono,
possibili), piu' un file di istantanee per ricostruire in fretta l'orario di un nodo qualunque."""
import array
import os
import threading

CAMPI, ISTANTANEA = 9, 16384
RECORD = CAMPI * 2  # byte
MOTIVI = ('', 'ore_docente', 'ore_docente_in_classe', 'ore_classe', 'prime_ore', 'docente_senza_posto',
          'classe_senza_scelte', 'orario_della_classe')


def quanti(file):
    """Quanti nodi ci sono nel file."""
    try:
        return os.path.getsize(file) // RECORD
    except OSError:
        return 0


def _record(f, da, a):
    """I record dei nodi da `da` ad `a` compresi (da 1), tutti di fila in un array."""
    f.seek((da - 1) * RECORD)
    return array.array('h', f.read((a - da + 1) * RECORD))


def passo(file, k, celle):
    """Il nodo k (da 1): {"k", "profondita", "cella", "voce", "motivo" (None, o tipo e numeri), "orario": per ogni
    cella la sua voce, -1 per l'ora buca, None se non e' decisa}. None se il nodo non c'e'.
    Si parte dall'ultima istantanea prima del nodo e si applicano i record fino a k: ognuno sostituisce la scelta
    alla sua profondita e toglie quelle dopo."""
    if not 1 <= k <= quanti(file):
        return None
    lunga = (1 + 2 * celle) * 2
    with open(file, 'rb') as f:
        strada, da = [], 1
        try:
            with open(file + '.ist', 'rb') as g:
                j = min((k - 1) // ISTANTANEA, os.fstat(g.fileno()).st_size // lunga)
                if j:
                    g.seek((j - 1) * lunga)
                    s = array.array('h', g.read(lunga))
                    strada, da = [(s[1 + 2 * i], s[2 + 2 * i]) for i in range(s[0])], j * ISTANTANEA + 1
        except OSError:
            pass
        r = _record(f, da, k)
    for i in range(0, len(r), CAMPI):
        strada[max(r[i] - 1, 0):] = [(r[i + 1], r[i + 2])] if r[i] else []
    profondita, cella, voce, tipo, docente, classe, ora, servono, possibili = r[-CAMPI:]
    orario = [None] * celle
    for c, v in strada:
        orario[c] = v
    motivo = tipo and {'tipo': MOTIVI[tipo], 'docente': docente, 'classe': classe, 'ora': ora, 'servono': servono,
                       'possibili': possibili}
    return {'k': k, 'profondita': profondita, 'cella': cella, 'voce': voce, 'motivo': motivo or None, 'orario': orario}


def punto_morto(file, k, avanti):
    """Il primo nodo dopo k (o prima, se non avanti) da cui la ricerca non va avanti, cioe' con un motivo. None se
    non ce ne sono."""
    n, pezzo = quanti(file), 65536
    with open(file, 'rb') as f:
        j = k + 1 if avanti else k - 1
        while 1 <= j <= n:
            da, a = (j, min(j + pezzo - 1, n)) if avanti else (max(1, j - pezzo + 1), j)
            tipi = _record(f, da, a)[3::CAMPI]
            for i in (range(len(tipi)) if avanti else range(len(tipi) - 1, -1, -1)):
                if tipi[i]:
                    return da + i
            j = a + 1 if avanti else da - 1
    return None


_BLOCCO = 256    # il profilo si tiene a blocchi di tanti nodi: (profondita minima, massima) di ogni blocco intero
_profili, _lock = {}, threading.Lock()


def profilo(file, fasce=400):
    """Come e' andata la ricerca, in al massimo `fasce` tratti uguali: per ognuno la profondita piu' bassa e la piu'
    alta. {"nodi": quanti, "fasce": [[bassa, alta], ...]}. I blocchi gia' guardati si ricordano: mentre la ricerca va
    avanti si leggono solo i nodi nuovi."""
    n = quanti(file)
    with _lock:
        blocchi = _profili.setdefault(file, [])
        if len(blocchi) * _BLOCCO > n:   # il file e' stato rifatto
            blocchi.clear()
        resto = []
        if n:
            with open(file, 'rb') as f:
                while (len(blocchi) + 1) * _BLOCCO <= n:
                    p = _record(f, len(blocchi) * _BLOCCO + 1, (len(blocchi) + 1) * _BLOCCO)[0::CAMPI]
                    blocchi.append((min(p), max(p)))
                if len(blocchi) * _BLOCCO < n:   # l'ultimo blocco, non ancora intero
                    p = _record(f, len(blocchi) * _BLOCCO + 1, n)[0::CAMPI]
                    resto = [(min(p), max(p))]
        tutti = blocchi + resto
    per_fascia = max(1, -(-len(tutti) // fasce))
    return {'nodi': n, 'fasce': [[min(b for b, _ in t), max(a for _, a in t)]
                                 for t in (tutti[i:i + per_fascia] for i in range(0, len(tutti), per_fascia))]}


def dimentica(file):
    with _lock:
        _profili.pop(file, None)
