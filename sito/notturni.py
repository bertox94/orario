"""I test notturni: un elenco di casi (seme, classi, ritardi, ordine delle classi) che si rieseguono tutti insieme, in
un giro, per vedere se i loro numeri cambiano.

Tutto sta in lavoro/notturni/, in file JSON:
- test.json: l'elenco dei test, ognuno con il suo numero;
- impostazioni.json: il limite di nodi, uno solo per tutti i test (lo usa il giro, e si cambia dalla pagina), e il
  giro della notte: se parte da solo, a che ora, l'ultimo giorno in cui e' scattato e l'ultima volta che e' saltato;
- giri/<n>.json: un giro, con il limite usato e il risultato di ogni test (esito, nodi, cella piu' avanti, tempo).

Un giro esegue i test uno dopo l'altro con ./untitled caso (caso.h), in una cartella provvisoria: del test si tengono
solo i numeri, il file dei passi si cancella. Ogni risultato si confronta con l'ultimo dello stesso test fatto con lo
stesso limite: «uguale» o «diverso» (esito, nodi, cella piu' avanti; il tempo no, dipende dal computer). Con un
limite diverso i numeri non si confrontano: «limite cambiato». Un giro alla volta, e mai insieme a un test in tempo
reale.

Il giro della notte: se e' acceso, all'ora scelta il sito aggiunge all'elenco un livello (tutti i casi con una classe
in piu' della notte prima, partendo da tre: per i semi e gli ordini scelti nella pagina, senza e con ritardi; senza
filtri) e avvia il giro da solo, una volta al giorno. Il giro dovrebbe finire entro l'ora di fine: se va oltre
finisce lo stesso, ma risulta fuori tempo e dalla notte dopo non si aggiungono altri livelli, finche' un giro della
notte non torna a finire in tempo (i test si tolgono a mano). Se all'ora di partenza gira altro (un giro o un test in
tempo reale) quella notte salta, senza test nuovi, e la pagina lo dice. Se il sito a quell'ora era spento, si aspetta
la notte dopo.
"""
import datetime
import json
import os
import shutil
import subprocess
import threading
import time

from . import solver, test_realtime

LIMITE = 10000000   # il limite di nodi finche' non lo si cambia dalla pagina
CLASSI_MIN, CLASSI_MAX = 3, 120  # i livelli della notte: da tre classi a quante ne regge il solver
SEMI, ORDINI = 4, 2  # un livello: semi da 0 a SEMI e ordini da 0 a ORDINI, finche' non si cambiano dalla pagina
# il giro della notte parte e deve finire a queste ore, finche' non si cambiano dalla pagina
ORA_DELLA_NOTTE, FINE_DELLA_NOTTE = '22:00', '08:00'
CONFRONTATI = ('esito', 'nodi', 'pos_max')
PRECEDENTI = 5      # quante esecuzioni precedenti di un test mostra la pagina

_lock = threading.Lock()
_giro = {'id': None, 'processo': None, 'fermare': False}  # il giro in corso
_orologio = {'partito': False}                             # il thread che guarda l'ora per il giro della notte


def _cartella(lavoro, *pezzi):
    return os.path.join(lavoro, 'notturni', *pezzi)


def _leggi(percorso, senza):
    try:
        with open(percorso) as f:
            return json.load(f)
    except (OSError, ValueError):
        return senza


def _scrivi(percorso, valore):
    os.makedirs(os.path.dirname(percorso), exist_ok=True)
    with open(percorso + '.tmp', 'w') as f:
        json.dump(valore, f)
    os.replace(percorso + '.tmp', percorso)


# ---- l'elenco dei test e il limite ----

def elenco(lavoro):
    return _leggi(_cartella(lavoro, 'test.json'), [])


def _impostazioni(lavoro):
    return _leggi(_cartella(lavoro, 'impostazioni.json'), {})


def _imposta(lavoro, **valori):
    """Cambia alcune impostazioni e lascia le altre come sono."""
    with _lock:
        _scrivi(_cartella(lavoro, 'impostazioni.json'), dict(_impostazioni(lavoro), **valori))


def limite(lavoro):
    return _impostazioni(lavoro).get('limite', LIMITE)


def cambia_limite(lavoro, nodi):
    _imposta(lavoro, limite=nodi)


def _numero_nuovo(lavoro, tutti):
    """Il numero per un test nuovo: mai uno gia' usato, nemmeno da un test poi eliminato, perche' i giri fatti
    ricordano i risultati per numero e un test nuovo si troverebbe la storia di un altro. L'ultimo numero dato sta
    nelle impostazioni; la prima volta si ricava dai test che ci sono e da quelli dei giri fatti. Va chiamata con
    _lock preso."""
    imp = _impostazioni(lavoro)
    ultimo = imp.get('ultimo_numero')
    if ultimo is None:
        ultimo = max([n for gid in _giri(lavoro) for n in (giro(lavoro, gid) or {}).get('test', [])], default=0)
    ultimo = max([ultimo] + [t['id'] for t in tutti]) + 1
    _scrivi(_cartella(lavoro, 'impostazioni.json'), dict(imp, ultimo_numero=ultimo))
    return ultimo


def aggiungi(lavoro, seme, classi, ritardi, ordine, da_solo=False):
    """Aggiunge il caso ai test notturni, se non c'e' gia' (seme, classi, ritardi e ordine uguali). Ritorna il test, e
    se e' nuovo. da_solo: lo ha aggiunto il giro della notte."""
    with _lock:
        tutti = elenco(lavoro)
        for t in tutti:
            if (t['seme'], t['classi'], t['ritardi'], t['ordine']) == (seme, classi, ritardi, ordine):
                return t, False
        t = {'id': _numero_nuovo(lavoro, tutti), 'seme': seme, 'classi': classi, 'ritardi': ritardi,
             'ordine': ordine, 'aggiunto': time.time(), 'da_solo': da_solo}
        _scrivi(_cartella(lavoro, 'test.json'), tutti + [t])
        return t, True


def togli(lavoro, tid):
    with _lock:
        _scrivi(_cartella(lavoro, 'test.json'), [t for t in elenco(lavoro) if t['id'] != tid])


# ---- i giri ----

def _giri(lavoro):
    """I numeri dei giri fatti, in ordine."""
    d = _cartella(lavoro, 'giri')
    return sorted(int(x[:-5]) for x in os.listdir(d) if x.endswith('.json') and x[:-5].isdigit()) \
        if os.path.isdir(d) else []


def giro(lavoro, gid):
    return _leggi(_cartella(lavoro, 'giri', '%d.json' % gid), None)


def ultimo_giro(lavoro):
    numeri = _giri(lavoro)
    return giro(lavoro, numeri[-1]) if numeri else None


def in_corso():
    """Il numero del giro che sta girando (o None)."""
    with _lock:
        return _giro['id']


def _e_suo(t, g):
    """Il giro g e' della storia del test t solo se e' partito dopo che t e' stato aggiunto: un giro piu' vecchio con
    lo stesso numero parla di un altro test, eliminato (prima del 13 novembre 2026 i numeri si riusavano)."""
    return (g.get('inizio') or 0) >= t.get('aggiunto', 0)


def _prima(lavoro, t, nodi, fino_a):
    """L'ultimo risultato del test t, nei giri prima di fino_a: (risultato, limite di quel giro), con preferenza per
    un giro fatto con il limite `nodi` (quello con cui si confronta). (None, None) se il test non ha mai girato."""
    altro = (None, None)
    for gid in reversed([g for g in _giri(lavoro) if g < fino_a]):
        g = giro(lavoro, gid) or {}
        r = g.get('risultati', {}).get(str(t['id'])) if _e_suo(t, g) else None
        if not r:
            continue
        if g.get('limite') == nodi:
            return r, nodi
        if altro[0] is None:
            altro = (r, g.get('limite'))
    return altro


def _confronto(lavoro, t, nodi, gid, r):
    prima, limite_prima = _prima(lavoro, t, nodi, gid)
    if prima is None:
        return 'primo'
    if limite_prima != nodi:
        return 'limite cambiato'
    return 'uguale' if all(prima.get(k) == r.get(k) for k in CONFRONTATI) else 'diverso'


def _esegui_test(lavoro, exe, t, nodi):
    """Un test: il solver in una cartella provvisoria, da cui si leggono i numeri. None se il giro e' stato fermato."""
    d = _cartella(lavoro, 'provvisoria')
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    try:
        p = subprocess.Popen([exe, 'caso', str(t['seme']), str(t['classi']), '1' if t['ritardi'] else '0',
                              str(t['ordine']), str(nodi), 'stato.json', 'passi.bin'], cwd=d,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        with _lock:
            _giro['processo'] = p
        p.wait()
        with _lock:
            _giro['processo'] = None
            if _giro['fermare']:
                return None
        s = _leggi(os.path.join(d, 'stato.json'), {})
        if s.get('in_corso', 1):
            return {'esito': 'non riuscito'}
        r = {k: s.get(k) for k in ('esito', 'nodi', 'pos_max', 'celle', 'secondi')}
        if s.get('errore'):
            r['errore'] = s['errore']
        if 'verifica' in s:
            r['verificato'] = bool(s['verifica']['ok'])
        return r
    finally:
        shutil.rmtree(d, ignore_errors=True)


def _esegui_giro(lavoro, gid, nodi, tests):
    percorso = _cartella(lavoro, 'giri', '%d.json' % gid)
    g = giro(lavoro, gid)
    try:
        exe = solver.compila(os.path.join(lavoro, 'bin'))
        for t in tests:
            g['adesso'] = t['id']
            _scrivi(percorso, g)
            r = _esegui_test(lavoro, exe, t, nodi)
            if r is None:
                g['stato'] = 'fermato'
                break
            r['confronto'] = _confronto(lavoro, t, nodi, gid, r)
            g['risultati'][str(t['id'])] = r
        else:
            g['stato'] = 'finito'
    except Exception as e:  # il solver che non compila, il disco pieno...: il giro finisce e lo dice
        g['stato'] = 'errore: %s' % e
    finally:
        g['adesso'] = None
        g['fine'] = time.time()
        g['fuori_tempo'] = g.get('entro') is not None and g['fine'] > g['entro']  # il giro della notte, oltre l'ora
        _scrivi(percorso, g)
        with _lock:
            _giro.update(id=None, processo=None, fermare=False)


def avvia_giro(lavoro, da_solo=False, entro=None):
    """Lancia un giro su tutti i test dell'elenco e ritorna il suo numero. ValueError se non si puo'. da_solo: e' il
    giro della notte, partito senza il tasto; entro: quando dovrebbe finire (secondi): se va oltre finisce lo stesso, ma
    risulta fuori tempo."""
    with _lock:
        if _giro['id'] is not None:
            raise ValueError('C\'è già un giro in corso.')
        if test_realtime.in_corso() is not None:
            raise ValueError('Sta girando un test in tempo reale: aspetta che finisca, o fermalo.')
        tests = elenco(lavoro)
        if not tests:
            raise ValueError('Non ci sono test notturni da eseguire.')
        gid = max(_giri(lavoro), default=0) + 1
        nodi = limite(lavoro)
        _scrivi(_cartella(lavoro, 'giri', '%d.json' % gid),
                {'id': gid, 'inizio': time.time(), 'fine': None, 'limite': nodi, 'stato': 'in corso', 'adesso': None,
                 'da_solo': da_solo, 'entro': entro, 'test': [t['id'] for t in tests], 'risultati': {}})
        _giro.update(id=gid, processo=None, fermare=False)
    threading.Thread(target=_esegui_giro, args=(lavoro, gid, nodi, tests), daemon=True).start()
    return gid


def ferma_giro():
    """Ferma il giro in corso: il test che sta girando si interrompe, quelli gia' fatti restano."""
    with _lock:
        if _giro['id'] is None:
            return
        _giro['fermare'] = True
        if _giro['processo'] is not None:
            _giro['processo'].terminate()


# ---- il giro della notte ----

def notte(lavoro):
    """Il giro della notte: se e' acceso, da che ora a che ora ('HH:MM'), i semi e gli ordini di ogni livello, il
    prossimo livello (le sue classi e quanti test nuovi porta; None se le classi sono finite), se l'ultimo giro della
    notte e' finito fuori tempo (allora non si aggiunge niente) e l'ultima volta che e' saltato (o None)."""
    i = _impostazioni(lavoro)
    classi, nuovi = _prossimo_livello(lavoro)
    return {'acceso': i.get('notte_acceso', False), 'ora': i.get('notte_ora', ORA_DELLA_NOTTE),
            'fine': i.get('notte_fine', FINE_DELLA_NOTTE), 'semi': i.get('notte_semi', SEMI),
            'ordini': i.get('notte_ordini', ORDINI), 'classi': classi, 'nuovi': len(nuovi),
            'fuori_tempo': _fuori_tempo(lavoro), 'saltato': i.get('notte_saltato')}


def cambia_notte(lavoro, acceso, ora, fine, semi, ordini):
    _imposta(lavoro, notte_acceso=acceso, notte_ora=ora, notte_fine=fine, notte_semi=semi, notte_ordini=ordini)


def _prossimo_livello(lavoro):
    """Il prossimo livello: un numero di classi (si parte da CLASSI_MIN, una in piu' ogni notte) e i suoi casi che non
    sono gia' nell'elenco: per ogni seme da 0 a `semi` e ogni ordine da 0 a `ordini`, senza e con ritardi.
    (None, []) quando le classi sono finite."""
    i = _impostazioni(lavoro)
    classi = i.get('notte_classi', CLASSI_MIN)
    if classi > CLASSI_MAX:
        return None, []
    presenti = {(t['seme'], t['classi'], t['ritardi'], t['ordine']) for t in elenco(lavoro)}
    casi = [(seme, classi, ritardi, ordine) for seme in range(i.get('notte_semi', SEMI) + 1)
            for ordine in range(i.get('notte_ordini', ORDINI) + 1) for ritardi in (False, True)]
    return classi, [c for c in casi if c not in presenti]


def _fuori_tempo(lavoro):
    """L'ultimo giro della notte e' finito dopo l'ora di fine: l'elenco e' gia' troppo lungo per una notte."""
    for gid in reversed(_giri(lavoro)):
        g = giro(lavoro, gid) or {}
        if g.get('da_solo'):
            return bool(g.get('fuori_tempo'))
    return False


def controlla_la_notte(lavoro, adesso):
    """Il lavoro dell'orologio, con `adesso` (un datetime). Se il giro della notte e' acceso, e' il minuto in cui
    parte e oggi non e' ancora scattato: aggiunge all'elenco il prossimo livello (a meno che l'ultimo giro della notte
    sia finito fuori tempo) e avvia il giro, che dovrebbe finire entro l'ora di fine. Se gira altro (anche il giro
    della notte prima, non ancora finito) questa notte salta, senza test nuovi, e se ne tiene il motivo."""
    n, oggi = notte(lavoro), adesso.strftime('%Y-%m-%d')
    if not n['acceso'] or adesso.strftime('%H:%M') != n['ora'] or _impostazioni(lavoro).get('notte_giorno') == oggi:
        return
    _imposta(lavoro, notte_giorno=oggi)
    motivo = 'c\'era già un giro in corso.' if in_corso() is not None else \
        'stava girando un test in tempo reale.' if test_realtime.in_corso() is not None else None
    try:
        if motivo is None:
            classi, nuovi = _prossimo_livello(lavoro)
            if classi is not None and not n['fuori_tempo']:
                for caso in nuovi:
                    aggiungi(lavoro, *caso, da_solo=True)
                _imposta(lavoro, notte_classi=classi + 1)
            ora, minuti = map(int, n['fine'].split(':'))  # la prossima volta che l'orologio segna l'ora di fine
            entro = adesso.replace(hour=ora, minute=minuti, second=0, microsecond=0)
            avvia_giro(lavoro, da_solo=True,
                       entro=(entro if entro > adesso else entro + datetime.timedelta(days=1)).timestamp())
    except (ValueError, RuntimeError) as e:  # qualcosa e' partito proprio adesso, o il solver non compila
        motivo = str(e)
    _imposta(lavoro, notte_saltato=motivo and {'quando': adesso.timestamp(), 'ora': n['ora'], 'motivo': motivo})


def avvia_orologio(lavoro):
    """All'avvio del sito: un thread guarda l'ora ogni venti secondi (cosi' nessun minuto si perde) per il giro della
    notte."""
    with _lock:
        if _orologio['partito']:
            return
        _orologio['partito'] = True

    def guarda():
        while True:
            try:
                controlla_la_notte(lavoro, datetime.datetime.now())
            except Exception:  # un file illeggibile, il disco pieno: si riprova al prossimo sguardo
                pass
            time.sleep(20)
    threading.Thread(target=guarda, daemon=True).start()


def chiudi_giri_interrotti(lavoro):
    """All'avvio del sito: un giro rimasto «in corso» (il sito si e' fermato mentre girava) risulta «interrotto»."""
    for gid in _giri(lavoro):
        g = giro(lavoro, gid)
        if g and g.get('stato') == 'in corso':
            g.update(stato='interrotto', adesso=None, fine=g.get('fine') or time.time())
            _scrivi(_cartella(lavoro, 'giri', '%d.json' % gid), g)


def _andamento(esecuzioni):
    """Una frase sulle esecuzioni di un test (dalla piu' recente): se i numeri sono rimasti uguali o quante volte sono
    cambiati. Si guardano solo quelle fatte con il limite della piu' recente; '' se ce n'e' una sola."""
    stesse = [e for e in esecuzioni if e['limite'] == esecuzioni[0]['limite']]
    if len(stesse) < 2:
        return ''
    cambi = sum(any(a.get(k) != b.get(k) for k in CONFRONTATI) for a, b in zip(stesse, stesse[1:]))
    if cambi == 0:
        return 'nelle ultime %d esecuzioni sempre uguale' % len(stesse)
    return 'nelle ultime %d esecuzioni cambiato %s' % (len(stesse), '1 volta' if cambi == 1 else '%d volte' % cambi)


def situazione(lavoro):
    """Per la pagina: il limite, il giro in corso (o l'ultimo) e i test. Ogni test ha il suo ultimo risultato, le
    PRECEDENTI esecuzioni prima di quello (di qualunque limite, dalla piu' recente) e come sono andate."""
    nodi = limite(lavoro)
    giri = [x for x in (giro(lavoro, gid) for gid in reversed(_giri(lavoro))) if x]  # dal piu' recente
    g = giri[0] if giri else None
    tests = []
    for t in elenco(lavoro):
        # le esecuzioni del test, dalla piu' recente: il risultato, piu' il giro, la data e il limite di quel giro
        tutte = [dict(x['risultati'][str(t['id'])], giro=x['id'], inizio=x['inizio'], limite=x['limite'])
                 for x in giri if str(t['id']) in x.get('risultati', {}) and _e_suo(t, x)]
        # il risultato da mostrare: quello dell'ultimo giro; se li' il test non c'e' (aggiunto dopo, o giro fermato
        # prima), l'ultimo fatto con il limite di adesso; se non c'e' nemmeno quello, l'ultimo che ha
        i = 0 if tutte and tutte[0]['giro'] == g['id'] else \
            next((k for k, e in enumerate(tutte) if e['limite'] == nodi), 0)
        r = tutte[i] if tutte else None
        # un confronto scritto quando i numeri si riusavano puo' essere con il test di un altro: senza esecuzioni sue
        # prima di questa, e' la prima (anche nel riassunto del giro)
        if r and not tutte[i + 1:] and r.get('confronto') != 'primo':
            r['confronto'] = 'primo'
            if g and r['giro'] == g['id']:
                g['risultati'][str(t['id'])]['confronto'] = 'primo'
        tests.append(dict(t, risultato=r, prima=tutte[i + 1:i + 1 + PRECEDENTI],
                          andamento=_andamento(tutte[i:i + 1 + PRECEDENTI]) if r else ''))
    return {'limite': nodi, 'giro': g, 'in_corso': in_corso(), 'tests': tests,
            'realtime': test_realtime.in_corso(), 'notte': notte(lavoro)}
