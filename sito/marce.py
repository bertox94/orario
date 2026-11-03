"""Le marce indietro (-b) e i passi della ricerca (--passi) registrati in orario.txt.gz nel database, letti a richiesta.

Il formato e' descritto in printer/printer.h: una riga "B pos da s_da ... s_(pos-1)" per ogni marcia indietro e, con
--passi, una riga "P pos da ..." (stesso formato, stessa catena di differenze) per ogni nodo della ricerca. Qui si
ricostruisce l'orario parziale di una riga scorrendo il file dal principio (decompresso a pezzi, senza tenerlo in
memoria): le righe B e P condividono la catena, quindi si applicano entrambe qualunque sia quella cercata."""
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


def marcia(url, eid, n, nomi_classi, lettera='B'):
    """La n-esima marcia indietro (da 1; n oltre la fine = l'ultima). Ritorna un dizionario con:
        n, pos (lo slot dove non c'era nessun docente utilizzabile), blocco [g, o, c], orario [g][o][c] = [docente,
        codocente] con -1 negli slot non assegnati, motivi = [[docente, motivo]] (solo se la prova ha usato --motivi).
    Con lettera='P' e' invece l'n-esimo passo della ricerca (--passi): pos e' il numero di slot decisi e blocco lo
    slot che la ricerca sta per decidere; motivi resta None (le righe M seguono solo le righe B). In piu' c'e'
    passaggi [g][o][c]: quante volte la ricerca e' passata da ogni posizione (l'ha decisa) fino a questo passo."""
    giorni = ore = classi = 0
    ordine = []  # k-esima classe dell'ordine della ricerca -> indice nei dati del sito
    slot = []
    pos = visti = 0
    bloccato = False  # solo passi P: il nodo e' un vicolo cieco
    conta = {}  # posizione -> quante volte la ricerca l'ha decisa (una per ogni riga P applicata)
    motivi = None  # con --motivi: [docente, motivo] della marcia indietro in esame (None = non registrati)
    completa = False
    for r in righe(url, eid):
        if completa:
            if lettera == 'B':  # la riga M che segue la marcia scelta
                if r.startswith('M'):
                    motivi = [[int(x) for x in t.split(':')] for t in r.split()[1:] if ':' in t]
                break
            # passo P: se subito dopo c'e' la riga B dello stesso nodo, il passo e' un vicolo cieco (nessun docente
            # utilizzabile in quello slot) e dopo la B ci sono i motivi (riga M)
            if not bloccato and r.startswith('B ') and r.split()[1] == str(pos):
                bloccato = True
                continue
            if bloccato and r.startswith('M'):
                motivi = [[int(x) for x in t.split(':')] for t in r.split()[1:] if ':' in t]
            break
        if r.startswith('G '):
            _, giorni, ore, classi = r.split()[:4]
            giorni, ore, classi = int(giorni), int(ore), int(classi)
        elif r.startswith('K '):
            ordine = [nomi_classi.index(x) if x in nomi_classi else k for k, x in enumerate(r.split()[1:])]
        elif r.startswith('B ') or r.startswith('P '):  # entrambe fanno parte della catena di differenze
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
            if r[0] == 'P' and p > 0:  # a ogni passo la ricerca decide lo slot p - 1
                conta[p - 1] = conta.get(p - 1, 0) + 1
            if r[0] != lettera:
                continue
            pos = p
            visti += 1
            if visti == n:
                completa = True
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
    m = {'n': visti, 'pos': pos, 'slot': giorni * ore * classi, 'blocco': [g, o, ordine[k]], 'orario': griglia, 'motivi': motivi, 'ordine': ordine}
    if lettera == 'P':
        passaggi = [[[0] * classi for _ in range(ore)] for _ in range(giorni)]
        for i, v in conta.items():
            g, resto = divmod(i, ore * classi)
            o, k = divmod(resto, classi)
            passaggi[g][o][ordine[k]] = v
        m['passaggi'] = passaggi
        m['bloccato'] = bloccato
    return m


def totale(url, eid, definitivo, lettera='B'):
    """Quante marce indietro (o, con lettera='P', passi) sono registrate (contate una volta sola per i file finiti)."""
    if (eid, lettera) in _totali:
        return _totali[eid, lettera]
    n = sum(1 for r in righe(url, eid) if r.startswith(lettera + ' '))
    if definitivo:
        _totali[eid, lettera] = n
    return n


_vicoli = {}  # esecuzione -> elenco dei passi che sono vicoli ciechi (solo per file finiti)
MAX_VICOLI = 50000


def vicoli(url, eid, definitivo):
    """I numeri dei passi (da 1) che sono vicoli ciechi: una riga P seguita subito dalla B dello stesso nodo, cioe' nessun
    docente utilizzabile in quello slot. Al massimo MAX_VICOLI; ritorna (elenco, troncato)."""
    if eid in _vicoli:
        return _vicoli[eid]
    n, ultimo, elenco, troncato = 0, None, [], False
    for r in righe(url, eid):
        if r.startswith('P '):
            n += 1
            ultimo = r.split()[1]
            continue
        if r.startswith('B ') and ultimo is not None and r.split()[1] == ultimo:
            if len(elenco) >= MAX_VICOLI:
                troncato = True
                break
            elenco.append(n)
        ultimo = None
    if definitivo:
        _vicoli[eid] = (elenco, troncato)
    return elenco, troncato
