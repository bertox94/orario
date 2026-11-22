"""I passi di una ricerca, letti dal file che scrive il solver (solver/passi.h): un record di 9 interi a 16 bit per
nodo (profondita, cella, voce, e il motivo per cui da li' non si va avanti: tipo, docente, classe, ora, servono,
possibili), piu' un file di istantanee per ricostruire in fretta l'orario di un nodo qualunque."""
import array
import os
import threading

CAMPI, ISTANTANEA = 9, 16384   # come in solver/passi.h: i campi di un record, e ogni quanti nodi c'e' un'istantanea
RECORD = CAMPI * 2             # i byte di un record
# i nomi dei motivi, nell'ordine dei numeri che scrive il solver (solver/ricerca.h, TipoMotivo): 0 e' «si va avanti»
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


def _strada(file, k, celle):
    """La strada del nodo k (da 1) e il suo record: ([(cella, voce), ...] nell'ordine in cui le celle sono state
    decise, i 9 campi del record). None se il nodo non c'e'.
    Si parte dall'ultima istantanea prima del nodo e si applicano i record fino a k: ognuno sostituisce la scelta
    alla sua profondita e toglie quelle dopo."""
    if not 1 <= k <= quanti(file):
        return None
    lunga = (1 + 2 * celle) * 2   # i byte di un'istantanea: la profondita e una coppia (cella, voce) per cella
    with open(file, 'rb') as f:
        strada, da = [], 1        # senza istantanee si parte dal primo nodo, con la strada vuota
        try:
            with open(file + '.ist', 'rb') as g:
                # l'istantanea numero j e' la strada del nodo j * ISTANTANEA: serve l'ultima prima del nodo k, tra
                # quelle gia' scritte per intero
                j = min((k - 1) // ISTANTANEA, os.fstat(g.fileno()).st_size // lunga)
                if j:
                    g.seek((j - 1) * lunga)
                    s = array.array('h', g.read(lunga))
                    strada, da = [(s[1 + 2 * i], s[2 + 2 * i]) for i in range(s[0])], j * ISTANTANEA + 1
        except OSError:
            pass
        r = _record(f, da, k)
    # ogni record: la sua scelta va al posto numero «profondita» (r[i]) e toglie tutte quelle dopo; la radice
    # (profondita 0) non ha scelta e svuota la strada
    for i in range(0, len(r), CAMPI):
        strada[max(r[i] - 1, 0):] = [(r[i + 1], r[i + 2])] if r[i] else []
    return strada, list(r[-CAMPI:])


def passo(file, k, celle):
    """Il nodo k (da 1): {"k", "profondita", "cella", "voce", "motivo" (None, o tipo e numeri), "orario": per ogni
    cella la sua voce, -1 per l'ora buca, None se non e' decisa, "indietro"}. None se il nodo non c'e'.
    «indietro», per un ritorno: di quante celle la ricerca torna indietro subito dopo, cioe' quante scelte disfa
    prima di provarne una nuova. 1 e' il caso normale (cambia solo l'ultima scelta); di piu' vuol dire che anche le
    celle prima avevano finito le loro scelte. None se non e' un ritorno, o e' l'ultimo nodo che c'e'."""
    x = _strada(file, k, celle)
    if not x:
        return None
    # (i nove campi dell'ultimo record: quelli del nodo k)
    strada, (profondita, cella, voce, tipo, docente, classe, ora, servono, possibili) = x
    orario = [None] * celle
    # dalla strada all'orario: ogni scelta nella sua cella, le altre restano non decise
    for c, v in strada:
        orario[c] = v
    motivo = tipo and {'tipo': MOTIVI[tipo], 'docente': docente, 'classe': classe, 'ora': ora, 'servono': servono,
                       'possibili': possibili}
    indietro = None
    if motivo and k < quanti(file):   # il nodo dopo dice dove la ricerca e' ripartita
        with open(file, 'rb') as f:
            indietro = profondita - _record(f, k + 1, k + 1)[0] + 1
    return {'k': k, 'profondita': profondita, 'cella': cella, 'voce': voce, 'motivo': motivo or None, 'orario': orario,
            'indietro': indietro}


def strada(file, k, celle):
    """La strada del nodo k: [(cella, voce), ...] nell'ordine in cui le celle sono state decise. None se il nodo non
    c'e'."""
    x = _strada(file, k, celle)
    return x[0] if x else None


def strada_per_il_solver(scelte):
    """Una strada come la vogliono i comandi «scelte» e «risolvi --da» del solver: quante scelte, e per ognuna la
    cella e la voce, tutto in numeri."""
    return ' '.join(str(n) for n in [len(scelte)] + [n for scelta in scelte for n in scelta])


def nodo_per_il_solver(file, k, celle):
    """Il nodo k come lo vuole il comando «nodo» del solver (solver/main.c), tutto in numeri: il motivo (tipo 0 se
    da li' si va avanti), quante celle sono decise, e le scelte in ordine. None se il nodo non c'e'."""
    x = _strada(file, k, celle)
    if not x:
        return None
    strada, record = x
    return ' '.join(str(n) for n in record[3:] + [len(strada)] + [n for scelta in strada for n in scelta])


def ritorno(file, k, avanti, cella=None, almeno=1):
    """Il primo ritorno indietro dopo il nodo k (o prima, se non avanti): un nodo da cui la ricerca non va avanti,
    cioe' con un motivo. Con `cella`, solo quelli arrivati subito dopo una scelta in quella cella. Con `almeno`
    piu' di 1, solo quelli dopo cui la ricerca torna indietro di almeno tante celle (vedi `indietro`). None se non
    ce ne sono."""
    n, pezzo = quanti(file), 65536   # si legge a pezzi di tanti nodi, guardando solo i campi che servono
    with open(file, 'rb') as f:
        j = k + 1 if avanti else k - 1
        while 1 <= j <= n:
            da, a = (j, min(j + pezzo - 1, n)) if avanti else (max(1, j - pezzo + 1), j)
            r = _record(f, da, min(a + 1, n))   # un nodo in piu': per sapere di quanto si torna indietro dopo l'ultimo
            m, tipi, profondita = a - da + 1, r[3::CAMPI], r[0::CAMPI]
            # di quante celle si torna indietro dopo il nodo al posto i (dell'ultimo nodo del file non si sa: 0)
            indietro = lambda i: profondita[i] - profondita[i + 1] + 1 if i + 1 < len(profondita) else 0
            if cella is None:
                buoni = [i for i in range(m) if tipi[i] and (almeno <= 1 or indietro(i) >= almeno)]
            else:
                # prima i nodi di quella cella (array.index li cerca in fretta), poi tra quelli chi ha un motivo
                celle_dei_nodi, buoni, i = r[1::CAMPI], [], 0
                while True:
                    try:
                        i = celle_dei_nodi.index(cella, i)
                    except ValueError:
                        break
                    if i < m and tipi[i] and (almeno <= 1 or indietro(i) >= almeno):
                        buoni.append(i)
                    i += 1
            if buoni:
                return da + (buoni[0] if avanti else buoni[-1])
            j = a + 1 if avanti else da - 1
    return None


_BLOCCO = 256    # come PASSI_BLOCCO in solver/passi.h: il profilo e' a blocchi di tanti nodi
_profili, _lock = {}, threading.Lock()   # per ogni file, i blocchi gia' letti: (le piu' basse, le piu' alte)


def profilo(file, fasce=400):
    """Come e' andata la ricerca, in al massimo `fasce` tratti uguali: per ognuno la profondita piu' bassa e la piu'
    alta. {"nodi": quanti, "fasce": [[bassa, alta], ...]}.
    I blocchi vengono dal file del profilo che il solver scrive (FILE.prof); se non c'e' (una prova fatta prima che
    esistesse) si ricavano dai nodi. Quelli gia' letti si ricordano: mentre la ricerca va avanti si legge solo il
    nuovo."""
    n = quanti(file)
    with _lock:
        basse, alte = _profili.setdefault(file, (array.array('h'), array.array('h')))
        if len(basse) * _BLOCCO > n:   # il file e' stato rifatto
            del basse[:], alte[:]
        resto = []
        if n:
            try:
                with open(file + '.prof', 'rb') as g:
                    g.seek(len(basse) * 4)
                    nuovi = array.array('h', g.read((n // _BLOCCO - len(basse)) * 4))
                    # (solo coppie intere: il file puo' essere stato letto mentre il solver scriveva)
                    nuovi = nuovi[:len(nuovi) // 2 * 2]
                    basse.extend(nuovi[0::2])
                    alte.extend(nuovi[1::2])
                dal_profilo = True
            except OSError:
                dal_profilo = False
            with open(file, 'rb') as f:
                while not dal_profilo and (len(basse) + 1) * _BLOCCO <= n:
                    p = _record(f, len(basse) * _BLOCCO + 1, (len(basse) + 1) * _BLOCCO)[0::CAMPI]
                    basse.append(min(p))
                    alte.append(max(p))
                if len(basse) * _BLOCCO < n:   # i nodi dopo l'ultimo blocco scritto
                    p = _record(f, len(basse) * _BLOCCO + 1, n)[0::CAMPI]
                    resto = [[min(p), max(p)]]
        # le fasce: tanti blocchi per fascia (arrotondato in su), e di ogni fascia la piu' bassa e la piu' alta
        per_fascia = max(1, -(-(len(basse) + len(resto)) // fasce))
        tratti = [[min(basse[i:i + per_fascia]), max(alte[i:i + per_fascia])] for i in range(0, len(basse), per_fascia)]
    return {'nodi': n, 'fasce': tratti + resto}


def dimentica(file):
    """Butta via il profilo ricordato per questo file (la prova e' stata eliminata, o il file rifatto)."""
    with _lock:
        _profili.pop(file, None)
