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
    bloccato = False  # solo passi P: nessun docente utilizzabile in questo nodo (c'e' la riga B)
    scarto = None  # solo passi P: il nodo e' un ramo scartato dalla potatura (c'e' la riga S): 0 lezioni della classe, 1 ore dei docenti
    esito = None  # se bloccato: 'vuoto' (lo slot resta vuoto e la ricerca prosegue) o 'indietro' (vicolo cieco), None = non si sa
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
                continue
            if bloccato and r.startswith('P '):  # il passo dopo dice com'e' andata
                esito = _esito_dopo_blocco(pos, r)
            elif not bloccato and r.startswith('S '):  # ramo scartato dalla potatura: la ricerca e' tornata indietro subito
                try:
                    scarto = int(r.split()[1])
                except (ValueError, IndexError):
                    scarto = -1
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
        m['esito'] = esito
        m['scarto'] = scarto
    return m


def totale(url, eid, definitivo, lettera='B'):
    """Quante marce indietro (o, con lettera='P', passi) sono registrate (contate una volta sola per i file finiti)."""
    if (eid, lettera) in _totali:
        return _totali[eid, lettera]
    n = sum(1 for r in righe(url, eid) if r.startswith(lettera + ' '))
    if definitivo:
        _totali[eid, lettera] = n
    return n


def _esito_dopo_blocco(pos, riga_p):
    """Il nodo pos non aveva docenti utilizzabili. Guardando il passo successivo (riga P): se e' il nodo pos + 1 con lo slot
    pos vuoto ("-") la ricerca e' andata avanti lasciando l'ora vuota ('vuoto'); se e' un nodo con pos uguale o minore la
    ricerca e' tornata indietro ('indietro': un vero vicolo cieco). Altrimenti None."""
    try:
        c = riga_p.split()
        p, da = int(c[1]), int(c[2])
        if p == pos + 1 and 0 <= pos - da < len(c) - 3 and c[3 + pos - da] == '-':
            return 'vuoto'
        if p <= pos:
            return 'indietro'
    except (ValueError, IndexError):
        pass
    return None


MAX_BLOCCO = 2000


def blocco_passi(url, eid, da, k, nomi_classi):
    """Per l'animazione: i passi da `da` (da 1) a da + k - 1 come sole differenze, cosi' il browser li applica uno dopo
    l'altro senza una richiesta per passo. Ritorna:
        ordine   k-esima classe dell'ordine della ricerca -> indice nei dati del sito
        iniziale {slot: i primi slot decisi prima del passo `da` ("i", "i+j" o "-"), conta: [[posizione, volte]]}
        passi    [[pos, da, [slot], codice]] come le righe P (pos slot decisi; gli slot dal `da`-esimo in poi come scritto,
                 gli altri come nel passo prima); codice 0 normale, 1 nessun docente utilizzabile e slot lasciato vuoto,
                 2 nessun docente utilizzabile e ricerca tornata indietro (vicolo cieco), 3 nessun docente utilizzabile,
                 esito non registrato, 4 e 5 ramo scartato dalla potatura (4 + il motivo: 0 lezioni della classe, 1 ore
                 dei docenti): la ricerca e' tornata indietro subito
        finito   True se dopo questi non ce ne sono altri."""
    da, k = max(1, da), max(1, min(k, MAX_BLOCCO))
    ordine, slot, conta = [], [], {}
    n, iniziale, passi, pendente, dopo_p, finito = 0, None, [], None, False, True
    for r in righe(url, eid):
        if r.startswith('K '):
            ordine = [nomi_classi.index(x) if x in nomi_classi else i for i, x in enumerate(r.split()[1:])]
            dopo_p = False
            continue
        if r.startswith('S '):  # ramo scartato: il passo appena letto
            if dopo_p and passi and n >= da:
                try:
                    passi[-1][3] = 4 + int(r.split()[1])
                except (ValueError, IndexError):
                    pass
            dopo_p = False
            continue
        if not (r.startswith('P ') or r.startswith('B ')):
            dopo_p = False
            continue
        try:
            campi = r.split()
            p, d = int(campi[1]), int(campi[2])
            nuovi = campi[3:]
            if len(nuovi) != p - d:
                continue
        except (ValueError, IndexError):
            continue
        if r[0] == 'P':
            if pendente is not None:  # il passo dopo un blocco dice com'e' andata
                e = _esito_dopo_blocco(passi[pendente][0], r)
                passi[pendente][3] = 1 if e == 'vuoto' else 2 if e == 'indietro' else 3
                pendente = None
            if len(passi) >= k:
                finito = False
                break
            if n + 1 == da:
                iniziale = {'slot': list(slot), 'conta': [[i, v] for i, v in conta.items()]}
            n += 1
            conta[p - 1] = conta.get(p - 1, 0) + 1
            if n >= da:
                passi.append([p, d, nuovi, 0])
            dopo_p = True
        else:  # B: lo stesso nodo del passo appena letto (nessun docente utilizzabile)
            if dopo_p and passi and n >= da and p == passi[-1][0]:
                passi[-1][3] = 3
                pendente = len(passi) - 1
            dopo_p = False
        del slot[d:]
        slot += ['-'] * (d - len(slot))
        slot += nuovi
    if iniziale is None:
        iniziale = {'slot': list(slot), 'conta': [[i, v] for i, v in conta.items()]}
    return {'da': da, 'ordine': ordine, 'iniziale': iniziale, 'passi': passi, 'finito': finito}


_vicoli = {}  # esecuzione -> elenco dei passi senza docenti utilizzabili (solo per file finiti)
MAX_VICOLI = 50000


def vicoli(url, eid, definitivo):
    """I passi (da 1) che sono blocchi della ricerca. Ritorna ([[passo, tipo, motivo]], troncato) con tipo:
        'indietro'  nessun docente utilizzabile e la ricerca e' tornata indietro (vicolo cieco)
        'scarto'    ramo scartato dalla potatura (motivo 0 lezioni della classe, 1 ore dei docenti): ritorno indietro subito
        'vuoto'     nessun docente utilizzabile ma l'ora e' rimasta vuota (ora buca) e la ricerca e' andata avanti: non e'
                    un ritorno indietro, si segnala ma non si conta come blocco
        None        nessun docente utilizzabile, esito non registrato (per esempio e' l'ultimo passo registrato)
    Al massimo MAX_VICOLI."""
    if eid in _vicoli:
        return _vicoli[eid]
    n, ultimo, in_attesa, elenco, troncato = 0, None, None, [], False
    for r in righe(url, eid):
        if r.startswith('P '):
            if in_attesa:  # il passo dopo un blocco: dice com'e' andata
                elenco.append([in_attesa[0], _esito_dopo_blocco(in_attesa[1], r), None])
                in_attesa = None
            n += 1
            ultimo = r.split()[1]
            continue
        if r.startswith('S ') and ultimo is not None:
            if len(elenco) >= MAX_VICOLI:
                troncato = True
                break
            try:
                elenco.append([n, 'scarto', int(r.split()[1])])
            except (ValueError, IndexError):
                pass
        elif r.startswith('B ') and ultimo is not None and r.split()[1] == ultimo:
            if len(elenco) >= MAX_VICOLI:
                troncato = True
                break
            in_attesa = (n, int(ultimo))
        ultimo = None
    if in_attesa:
        elenco.append([in_attesa[0], None, None])
    if definitivo:
        _vicoli[eid] = (elenco, troncato)
    return elenco, troncato
