"""La ricerca passo passo (-b --passi) registrata in orario.txt.gz nel database, letta a richiesta.

Il formato e' descritto in printer/printer.h. Intestazione: "G giorni ore classi" e "K <classi nell'ordine della
ricerca>". Poi una riga "P pos da s_da ... s_(pos-1)" per ogni nodo della ricerca (un passo: i primi pos slot sono decisi,
scritti come differenze rispetto alla riga prima), seguita eventualmente da "B pos ..." (nessun docente utilizzabile in
quel nodo) con "M d:m ..." (i motivi, docente per docente), oppure da "S m" (nodo scartato dalla potatura, motivo m).
I file registrati da versioni precedenti del solver hanno anche righe "R pos" (ritorno a un nodo): si saltano. Il file si scorre dal principio decompresso a pezzi, senza
tenerlo in memoria.

Il tipo di un passo lo dice il passo successivo, con una regola sola: se sta nello stesso slot o prima, la ricerca e'
tornata indietro e il passo era un blocco; se sta avanti, e' andata avanti (vedi _codice)."""
import zlib

from . import archivio

MAX_BYTE = 4 * 2 ** 30  # oltre questa quantita' di testo decompresso la pagina rinuncia (il file si puo' scaricare)
MAX_BLOCCO = 2000  # passi per richiesta nell'animazione
MAX_VICOLI = 50000  # blocchi elencati al massimo
_totali = {}  # esecuzione -> numero di passi (solo per file finiti)
_vicoli = {}  # esecuzione -> elenco dei blocchi (solo per file finiti)


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


# ---- i passi e il loro tipo ----

def _codice(pos_dopo, pos, ha_b, scarto, slot_dopo_vuoto):
    """Il tipo di un passo, dal passo che lo segue:
        0  normale: la ricerca e' andata avanti
        1  nessun docente utilizzabile ma lo slot e' rimasto vuoto (ora buca) e la ricerca e' andata avanti
        2  vicolo cieco: nessun docente utilizzabile (riga B) e ritorno indietro
        3  nessun docente utilizzabile, esito non registrato (e' l'ultimo passo del file)
        4, 5  ramo scartato dalla potatura (4 + motivo: 0 lezioni della classe, 1 ore dei docenti) e ritorno indietro
        6  ritorno indietro senza motivo registrato (file di un solver piu' vecchio)"""
    if pos_dopo is None:
        return 3 if ha_b else 4 + scarto if scarto in (0, 1) else 0
    if pos_dopo > pos:
        return 1 if ha_b and slot_dopo_vuoto else 0
    if ha_b:
        return 2
    return 4 + scarto if scarto in (0, 1) else 6


TIPO = {0: None, 1: 'vuoto', 2: 'indietro', 3: None, 4: 'scarto', 5: 'scarto', 6: 'scarto'}  # codice -> tipo per il sito
BLOCCHI = (1, 2, 3, 4, 5, 6)  # i codici da elencare tra i blocchi (1 e 3 sono segnalazioni, non ritorni indietro)


def _motivo_scarto(codice):
    return codice - 4 if codice in (4, 5) else -1 if codice == 6 else None


def _scorri(url, eid, nomi_classi):
    """Scorre il file: prima l'intestazione (giorni, ore, classi, ordine), poi un passo alla volta come
    (n da 1, pos, da, nuovi, codice, motivi). ordine[k] e' l'indice nei dati del sito della k-esima classe dell'ordine
    della ricerca. Il codice di un passo si sa solo leggendo il passo successivo, quindi ogni passo esce quando arriva
    il seguente (l'ultimo alla fine)."""
    giorni = ore = classi = 0
    ordine = []
    pendente = None  # [n, pos, da, nuovi, ha_b, scarto, motivi] del passo in attesa del successivo
    n = 0
    for r in righe(url, eid):
        if r.startswith('P '):
            try:
                campi = r.split()
                pos, da = int(campi[1]), int(campi[2])
                nuovi = campi[3:]
                if len(nuovi) != pos - da:
                    continue
            except (ValueError, IndexError):
                continue
            if pendente:
                # il passo prima ha lasciato vuoto il suo slot se questo e' lo slot dopo e lo riscrive come "-"
                p0 = pendente[1]
                vuoto = pos == p0 + 1 and 0 <= p0 - da < len(nuovi) and nuovi[p0 - da] == '-'
                yield pendente[:4] + [_codice(pos, p0, pendente[4], pendente[5], vuoto), pendente[6]]
            elif n == 0:
                yield giorni, ore, classi, ordine
            n += 1
            pendente = [n, pos, da, nuovi, False, None, None]
        elif pendente and r.startswith('B ') and r.split()[1] == str(pendente[1]):
            pendente[4] = True
        elif pendente and r.startswith('M'):
            pendente[6] = [[int(x) for x in t.split(':')] for t in r.split()[1:] if ':' in t]
        elif pendente and r.startswith('S '):
            try:
                pendente[5] = int(r.split()[1])
            except (ValueError, IndexError):
                pendente[5] = -1
        elif r.startswith('G '):
            try:
                giorni, ore, classi = (int(x) for x in r.split()[1:4])
            except ValueError:
                pass
        elif r.startswith('K '):
            ordine = [nomi_classi.index(x) if x in nomi_classi else k for k, x in enumerate(r.split()[1:])]
    if pendente:
        yield pendente[:4] + [_codice(None, pendente[1], pendente[4], pendente[5], False), pendente[6]]
    elif n == 0:
        yield giorni, ore, classi, ordine


def _applica(slot, da, nuovi):
    """Applica a `slot` le differenze di una riga P: dal `da`-esimo in poi come scritto, prima come erano."""
    del slot[da:]
    slot += ['-'] * (da - len(slot))
    slot += nuovi


def passo(url, eid, n, nomi_classi):
    """L'n-esimo passo della ricerca (da 1; n oltre la fine = l'ultimo). Ritorna un dizionario con:
        n, pos (slot decisi: la ricerca sta per decidere lo slot pos), blocco [g, o, c] di quello slot, ordine (k-esima
        classe della ricerca -> indice nei dati), orario [g][o][c] = [docente, codocente] con -1 negli slot non assegnati,
        passaggi [g][o][c] = quante volte la ricerca ha deciso quella posizione fino a questo passo,
        ripreso (se il passo viene dopo un ritorno indietro: la posizione dove la ricerca e' tornata e ha scelto un altro
        docente, altrimenti None), bloccato (nessun docente utilizzabile in questo nodo) con i motivi [[docente, motivo]] (con --motivi, altrimenti
        None), esito 'vuoto' / 'indietro' / 'scarto' / None (vedi _codice) e scarto (motivo del ramo scartato: 0 lezioni
        della classe, 1 ore dei docenti, -1 non registrato) oppure None."""
    scorri = _scorri(url, eid, nomi_classi)
    giorni, ore, classi, ordine = next(scorri)
    if not classi:
        return None
    slot, conta, ultimo, ripreso = [], {}, None, None
    for passo_ in scorri:
        # dopo un ritorno indietro (codici 2, 4, 5, 6) questo passo riparte da `da`: li' la ricerca ha scelto un altro docente
        ripreso = passo_[2] if ultimo and ultimo[4] in (2, 4, 5, 6) else None
        ultimo = passo_
        _applica(slot, passo_[2], passo_[3])
        conta[passo_[1] - 1] = conta.get(passo_[1] - 1, 0) + 1  # il passo ha deciso lo slot pos - 1
        if passo_[0] == n:
            break
    if ultimo is None:
        return None
    visti, pos, _, _, codice, motivi = ultimo
    griglia = [[[[-1, -1] for _ in range(classi)] for _ in range(ore)] for _ in range(giorni)]
    for i, s in enumerate(slot[:pos]):
        if s == '-':
            continue
        g, resto = divmod(i, ore * classi)
        o, k = divmod(resto, classi)
        a, _, b = s.partition('+')
        griglia[g][o][ordine[k]] = [int(a), int(b) if b else -1]
    passaggi = [[[0] * classi for _ in range(ore)] for _ in range(giorni)]
    for i, v in conta.items():
        g, resto = divmod(i, ore * classi)
        o, k = divmod(resto, classi)
        passaggi[g][o][ordine[k]] = v
    # lo slot che la ricerca sta per decidere (oltre l'ultimo = orario completo: si segna l'ultimo)
    g, resto = divmod(pos, ore * classi) if pos < giorni * ore * classi else (giorni - 1, ore * classi - 1)
    o, k = divmod(resto, classi)
    return {'n': visti, 'pos': pos, 'slot': giorni * ore * classi, 'blocco': [g, o, ordine[k]], 'orario': griglia,
            'ordine': ordine, 'passaggi': passaggi, 'bloccato': codice in (1, 2, 3), 'esito': TIPO[codice],
            'motivi': motivi, 'scarto': _motivo_scarto(codice), 'ripreso': ripreso}


def totale(url, eid, definitivo):
    """Quanti passi sono registrati (contati una volta sola per i file finiti)."""
    if eid in _totali:
        return _totali[eid]
    n = sum(1 for r in righe(url, eid) if r.startswith('P '))
    if definitivo:
        _totali[eid] = n
    return n


def blocco_passi(url, eid, da, k, nomi_classi):
    """Per l'animazione: i passi da `da` (da 1) a da + k - 1 come sole differenze, cosi' il browser li applica uno dopo
    l'altro senza una richiesta per passo. Ritorna:
        ordine   k-esima classe dell'ordine della ricerca -> indice nei dati del sito
        iniziale {slot: i primi slot decisi prima del passo `da` ("i", "i+j" o "-"), conta: [[posizione, volte]]}
        passi    [[pos, da, [slot], codice]] come le righe P, con il codice di _codice
        finito   True se dopo questi non ce ne sono altri."""
    da, k = max(1, da), max(1, min(k, MAX_BLOCCO))
    scorri = _scorri(url, eid, nomi_classi)
    ordine = next(scorri)[3]
    slot, conta, passi, iniziale, finito = [], {}, [], None, True
    for n, pos, d, nuovi, codice, _ in scorri:
        if n == da:
            iniziale = {'slot': list(slot), 'conta': [[i, v] for i, v in conta.items()]}
        if n >= da:
            if len(passi) >= k:
                finito = False
                break
            passi.append([pos, d, nuovi, codice])
        conta[pos - 1] = conta.get(pos - 1, 0) + 1
        _applica(slot, d, nuovi)
    if iniziale is None:
        iniziale = {'slot': list(slot), 'conta': [[i, v] for i, v in conta.items()]}
    return {'da': da, 'ordine': ordine, 'iniziale': iniziale, 'passi': passi, 'finito': finito}


def vicoli(url, eid, definitivo):
    """I passi (da 1) che sono blocchi, o da segnalare. Ritorna ([[passo, tipo, motivo]], troncato) con tipo:
        'indietro'  vicolo cieco: nessun docente utilizzabile e la ricerca e' tornata indietro
        'scarto'    ramo scartato dalla potatura (motivo 0 lezioni della classe, 1 ore dei docenti, -1 non registrato)
        'vuoto'     nessun docente utilizzabile ma l'ora e' rimasta vuota (ora buca) e la ricerca e' andata avanti:
                    si segnala ma non e' un blocco
        None        nessun docente utilizzabile, esito non registrato (e' l'ultimo passo registrato)
    Al massimo MAX_VICOLI."""
    if eid in _vicoli:
        return _vicoli[eid]
    scorri = _scorri(url, eid, [])
    next(scorri)
    elenco, troncato = [], False
    for n, _, _, _, codice, _ in scorri:
        if codice not in BLOCCHI:
            continue
        if len(elenco) >= MAX_VICOLI:
            troncato = True
            break
        elenco.append([n, TIPO[codice], _motivo_scarto(codice)])
    if definitivo:
        _vicoli[eid] = (elenco, troncato)
    return elenco, troncato
