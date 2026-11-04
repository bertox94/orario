"""La ricerca passo passo (-b --passi) registrata in orario.txt.gz nel database, letta a richiesta.

Il formato e' descritto in printer/printer.h: una riga "P pos da s_da ... s_(pos-1)" per ogni nodo della ricerca (l'orario
parziale come differenze rispetto alla riga prima), "S m" dopo la P di un nodo scartato dalla potatura, "B pos da ..."
(stessa catena di differenze) per ogni marcia indietro e "M d:m ..." con i motivi di scarto dei docenti. Qui si
ricostruisce l'orario parziale di un passo scorrendo il file dal principio (decompresso a pezzi, senza tenerlo in
memoria): le righe B e P condividono la catena, quindi si applicano entrambe."""
import zlib

from . import archivio

# oltre questa quantita' di testo decompresso la pagina rinuncia (il file si puo' comunque scaricare)
MAX_BYTE = 400 * 2 ** 20
_totali = {}  # esecuzione -> numero di passi registrati (solo per file finiti)


class FileTroppoGrande(Exception):
    pass


def righe(url, eid):
    """Le righe di testo del file, decompresse a pezzi. Per un'esecuzione in corso legge quello che il solver ha scritto
    finora (senza l'ultima riga, che puo' essere a meta')."""
    from . import esecutore  # importato qui: esecutore importa solver e modello
    in_corso = esecutore.pezzi_in_corso(eid)
    d = zlib.decompressobj(16 + zlib.MAX_WBITS)
    resto, letti = b'', 0
    for pezzo in (in_corso if in_corso is not None else archivio.pezzi(url, eid, 'orario.txt.gz')):
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
    if resto and in_corso is None:
        yield resto.decode('ascii', 'replace')


def passo(url, eid, n, nomi_classi):
    """L'n-esimo passo della ricerca (da 1; n oltre la fine = l'ultimo). Ritorna un dizionario con:
        n, pos (slot decisi: la ricerca sta per decidere lo slot pos), blocco [g, o, c] di quello slot, ordine (k-esima
        classe della ricerca -> indice nei dati), orario [g][o][c] = [docente, codocente] con -1 negli slot non assegnati,
        passaggi [g][o][c] = quante volte la ricerca ha deciso quella posizione fino a questo passo,
        bloccato (nessun docente utilizzabile in questo nodo) con i motivi [[docente, motivo]] (con --motivi), esito
        'vuoto' / 'indietro' / 'scarto' / None (vedi _codice: lo dice il passo successivo) e scarto (motivo del ramo
        scartato: 0 lezioni della classe, 1 ore dei docenti, -1 non registrato) oppure None."""
    giorni = ore = classi = 0
    ordine = []  # k-esima classe dell'ordine della ricerca -> indice nei dati del sito
    slot = []
    pos = visti = 0
    bloccato = False  # nessun docente utilizzabile in questo nodo (c'e' la riga B)
    scarto = None  # il nodo e' un ramo scartato dalla potatura (c'e' la riga S): 0 lezioni della classe, 1 ore dei docenti
    esito = None  # se bloccato: 'vuoto' (lo slot resta vuoto e la ricerca prosegue) o 'indietro' (vicolo cieco), None = non si sa
    conta = {}  # posizione -> quante volte la ricerca l'ha decisa (una per ogni riga P applicata)
    motivi = None  # con --motivi: [docente, motivo] della marcia indietro di questo nodo (None = non registrati)
    completa = False
    for r in righe(url, eid):
        if completa:
            # le righe dopo la P scelta, fino alla P successiva: B (stesso nodo) = nessun docente utilizzabile, M = i
            # suoi motivi, S = ramo scartato dalla potatura; la P successiva dice se la ricerca e' tornata indietro
            if r.startswith('B ') and r.split()[1] == str(pos):
                bloccato = True
            elif r.startswith('M'):
                motivi = [[int(x) for x in t.split(':')] for t in r.split()[1:] if ':' in t]
            elif r.startswith('S '):
                try:
                    scarto = int(r.split()[1])
                except (ValueError, IndexError):
                    scarto = -1
            elif r.startswith('P '):
                try:
                    c = r.split()
                    p_dopo, da_dopo = int(c[1]), int(c[2])
                except (ValueError, IndexError):
                    break
                codice = _codice(p_dopo, pos, bloccato, scarto,
                                 p_dopo == pos + 1 and 0 <= pos - da_dopo < len(c) - 3 and c[3 + pos - da_dopo] == '-')
                esito = TIPO.get(codice)
                if codice == 6:
                    scarto = -1  # tornata indietro senza motivo registrato
                break
            continue
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
            if r[0] != 'P':
                continue
            if p > 0:  # a ogni passo la ricerca decide lo slot p - 1
                conta[p - 1] = conta.get(p - 1, 0) + 1
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
    # lo slot che la ricerca sta per decidere (oltre l'ultimo = orario completo: si segna l'ultimo)
    g, resto = divmod(pos, ore * classi) if pos < giorni * ore * classi else (giorni - 1, ore * classi - 1)
    o, k = divmod(resto, classi)
    blocco = [g, o, ordine[k]]
    passaggi = [[[0] * classi for _ in range(ore)] for _ in range(giorni)]
    for i, v in conta.items():
        gi, resto = divmod(i, ore * classi)
        oi, ki = divmod(resto, classi)
        passaggi[gi][oi][ordine[ki]] = v
    return {'n': visti, 'pos': pos, 'slot': giorni * ore * classi, 'blocco': blocco, 'orario': griglia,
            'ordine': ordine, 'passaggi': passaggi, 'bloccato': bloccato, 'esito': esito, 'motivi': motivi, 'scarto': scarto}


def totale(url, eid, definitivo):
    """Quanti passi sono registrati (contati una volta sola per i file finiti)."""
    if eid in _totali:
        return _totali[eid]
    n = sum(1 for r in righe(url, eid) if r.startswith('P '))
    if definitivo:
        _totali[eid] = n
    return n


MAX_BLOCCO = 2000


def _codice(pos_dopo, pos, ha_b, scarto, slot_dopo_vuoto):
    """Il tipo di un passo, dal passo che lo segue (la regola e' una sola: se il passo dopo sta nello stesso slot o
    prima, la ricerca e' tornata indietro, cioe' il nodo era un blocco):
        0  normale (la ricerca e' andata avanti)
        1  nessun docente utilizzabile ma lo slot e' rimasto vuoto (ora buca) e la ricerca e' andata avanti
        2  vicolo cieco: nessun docente utilizzabile (riga B) e ritorno indietro
        3  nessun docente utilizzabile, esito non registrato (e' l'ultimo passo del file)
        4, 5  ramo scartato dalla potatura (4 + motivo: 0 lezioni della classe, 1 ore dei docenti) e ritorno indietro
        6  ritorno indietro senza motivo registrato (file di un solver piu' vecchio, o nodo senza figli)"""
    if pos_dopo is None:
        return 3 if ha_b else (4 + scarto if scarto in (0, 1) else 0)
    if pos_dopo > pos:
        return 1 if ha_b and slot_dopo_vuoto else 0
    if ha_b:
        return 2
    if scarto in (0, 1):
        return 4 + scarto
    return 6


TIPO = {1: 'vuoto', 2: 'indietro', 3: None, 4: 'scarto', 5: 'scarto', 6: 'scarto'}


def _passi_con_esito(url, eid):
    """Scorre il file e produce, per ogni riga P, (n da 1, pos, da, nuovi, codice): il codice (vedi _codice) si sa solo
    leggendo la riga P successiva, quindi ogni passo esce quando arriva il seguente (l'ultimo alla fine)."""
    pendente = None  # [n, pos, da, nuovi, ha_b, scarto] del passo in attesa del successivo
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
                # il passo prima ha lasciato vuoto il suo slot se questo passo e' lo slot dopo e lo riscrive come "-"
                p0 = pendente[1]
                vuoto = pos == p0 + 1 and 0 <= p0 - da < len(nuovi) and nuovi[p0 - da] == '-'
                yield pendente[:4] + [_codice(pos, p0, pendente[4], pendente[5], vuoto)]
            n += 1
            pendente = [n, pos, da, nuovi, False, None]
        elif pendente and r.startswith('B ') and r.split()[1] == str(pendente[1]):
            pendente[4] = True
        elif pendente and r.startswith('S '):
            try:
                pendente[5] = int(r.split()[1])
            except (ValueError, IndexError):
                pendente[5] = -1
    if pendente:
        yield pendente[:4] + [_codice(None, pendente[1], pendente[4], pendente[5], False)]


def blocco_passi(url, eid, da, k, nomi_classi):
    """Per l'animazione: i passi da `da` (da 1) a da + k - 1 come sole differenze, cosi' il browser li applica uno dopo
    l'altro senza una richiesta per passo. Ritorna:
        ordine   k-esima classe dell'ordine della ricerca -> indice nei dati del sito
        iniziale {slot: i primi slot decisi prima del passo `da` ("i", "i+j" o "-"), conta: [[posizione, volte]]}
        passi    [[pos, da, [slot], codice]] come le righe P (pos slot decisi; gli slot dal `da`-esimo in poi come scritto,
                 gli altri come nel passo prima) con il codice di _codice
        finito   True se dopo questi non ce ne sono altri."""
    da, k = max(1, da), max(1, min(k, MAX_BLOCCO))
    ordine, slot, conta, passi, iniziale, finito = [], [], {}, [], None, True
    for r in righe(url, eid):
        if r.startswith('K '):
            ordine = [nomi_classi.index(x) if x in nomi_classi else i for i, x in enumerate(r.split()[1:])]
            break
    for n, pos, d, nuovi, codice in _passi_con_esito(url, eid):
        if n == da:
            iniziale = {'slot': list(slot), 'conta': [[i, v] for i, v in conta.items()]}
        if n >= da:
            if len(passi) >= k:
                finito = False
                break
            passi.append([pos, d, nuovi, codice])
        conta[pos - 1] = conta.get(pos - 1, 0) + 1
        del slot[d:]
        slot += ['-'] * (d - len(slot))
        slot += nuovi
    if iniziale is None:
        iniziale = {'slot': list(slot), 'conta': [[i, v] for i, v in conta.items()]}
    return {'da': da, 'ordine': ordine, 'iniziale': iniziale, 'passi': passi, 'finito': finito}


_vicoli = {}  # esecuzione -> elenco dei passi senza docenti utilizzabili (solo per file finiti)
MAX_VICOLI = 50000


def vicoli(url, eid, definitivo):
    """I passi (da 1) che sono blocchi della ricerca, o che vanno segnalati. Ritorna ([[passo, tipo, motivo]], troncato):
        'indietro'  vicolo cieco: nessun docente utilizzabile e la ricerca e' tornata indietro
        'scarto'    ramo scartato dalla potatura (motivo 0 lezioni della classe, 1 ore dei docenti, -1 non registrato):
                    la ricerca e' tornata indietro subito
        'vuoto'     nessun docente utilizzabile ma l'ora e' rimasta vuota (ora buca) e la ricerca e' andata avanti: non e'
                    un ritorno indietro, si segnala ma non si conta come blocco
        None        nessun docente utilizzabile, esito non registrato (e' l'ultimo passo registrato)
    Al massimo MAX_VICOLI."""
    if eid in _vicoli:
        return _vicoli[eid]
    elenco, troncato = [], False
    for n, pos, da, nuovi, codice in _passi_con_esito(url, eid):
        if codice == 0:
            continue
        if len(elenco) >= MAX_VICOLI:
            troncato = True
            break
        elenco.append([n, TIPO[codice], codice - 4 if codice in (4, 5) else -1 if codice == 6 else None])
    if definitivo:
        _vicoli[eid] = (elenco, troncato)
    return elenco, troncato
