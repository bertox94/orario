"""I test notturni: un elenco di casi (seme, classi, ritardi, ordine delle classi) che si rieseguono tutti insieme, in
un giro, per vedere se i loro numeri cambiano.

Tutto sta in lavoro/notturni/, in file JSON:
- test.json: l'elenco dei test, ognuno con il suo numero;
- impostazioni.json: il limite di nodi, uno solo per tutti i test (lo usa il giro, e si cambia dalla pagina), e il
  giro della notte: se parte da solo, a che ora, l'ultimo giorno in cui e' scattato e l'ultima volta che e' saltato;
- giri/<n>.json: un giro, con il limite usato e il risultato di ogni test (esito, nodi, cella piu' avanti, tempo);
- ricerca.json: l'ultima ricerca di nuovi casi.

Un giro esegue i test uno dopo l'altro con ./untitled caso (caso.h), in una cartella provvisoria: del test si tengono
solo i numeri, il file dei passi si cancella. Ogni risultato si confronta con l'ultimo dello stesso test fatto con lo
stesso limite: «uguale» o «diverso» (esito, nodi, cella piu' avanti; il tempo no, dipende dal computer). Con un
limite diverso i numeri non si confrontano: «limite cambiato». Un giro alla volta, e mai insieme a un test in tempo
reale.

«Cerca nuovi casi» lancia ./untitled test N ORDINI LIMITE (prove.h), le stesse prove che si fanno dal terminale, e
aggiunge all'elenco i casi che arrivano al limite e non ci sono gia'.

Il giro della notte: se e' acceso, all'ora scelta il sito avvia il giro da solo, una volta al giorno. Se a quell'ora
gira altro (un giro, una ricerca, un test in tempo reale) il giro di quella notte salta, e la pagina lo dice. Se il
sito a quell'ora era spento, si aspetta la notte dopo.
"""
import datetime
import json
import os
import re
import shutil
import signal
import subprocess
import threading
import time

from . import solver, test_realtime

LIMITE = 10000000   # il limite di nodi finche' non lo si cambia dalla pagina
ORA_DELLA_NOTTE = '02:00'  # l'ora del giro della notte finche' non la si cambia dalla pagina
CONFRONTATI = ('esito', 'nodi', 'pos_max')

_lock = threading.Lock()
_giro = {'id': None, 'processo': None, 'fermare': False}  # il giro in corso
_cerca = {'in_corso': False, 'processo': None}             # la ricerca di nuovi casi in corso
_orologio = {'partito': False}                             # il thread che guarda l'ora per il giro della notte
# una riga di ./untitled test: «seme 0, 5 classi, senza ritardi, ordine 2: ...» (l'ordine c'e' solo se non e' 0)
_RIGA = re.compile(r'seme (\d+), (\d+) classi, (con|senza) ritardi(?:, ordine (\d+))?:')


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


def aggiungi(lavoro, seme, classi, ritardi, ordine):
    """Aggiunge il caso ai test notturni, se non c'e' gia' (seme, classi, ritardi e ordine uguali). Ritorna il test, e
    se e' nuovo."""
    with _lock:
        tutti = elenco(lavoro)
        for t in tutti:
            if (t['seme'], t['classi'], t['ritardi'], t['ordine']) == (seme, classi, ritardi, ordine):
                return t, False
        t = {'id': max((x['id'] for x in tutti), default=0) + 1, 'seme': seme, 'classi': classi, 'ritardi': ritardi,
             'ordine': ordine, 'aggiunto': time.time()}
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


def _prima(lavoro, tid, nodi, fino_a):
    """L'ultimo risultato del test tid, nei giri prima di fino_a: (risultato, limite di quel giro), con preferenza per
    un giro fatto con il limite `nodi` (quello con cui si confronta). (None, None) se il test non ha mai girato."""
    altro = (None, None)
    for gid in reversed([g for g in _giri(lavoro) if g < fino_a]):
        g = giro(lavoro, gid) or {}
        r = g.get('risultati', {}).get(str(tid))
        if not r:
            continue
        if g.get('limite') == nodi:
            return r, nodi
        if altro[0] is None:
            altro = (r, g.get('limite'))
    return altro


def _confronto(lavoro, tid, nodi, gid, r):
    prima, limite_prima = _prima(lavoro, tid, nodi, gid)
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
            r['confronto'] = _confronto(lavoro, t['id'], nodi, gid, r)
            g['risultati'][str(t['id'])] = r
        else:
            g['stato'] = 'finito'
    except Exception as e:  # il solver che non compila, il disco pieno...: il giro finisce e lo dice
        g['stato'] = 'errore: %s' % e
    finally:
        g['adesso'] = None
        g['fine'] = time.time()
        _scrivi(percorso, g)
        with _lock:
            _giro.update(id=None, processo=None, fermare=False)


def avvia_giro(lavoro, da_solo=False):
    """Lancia un giro su tutti i test dell'elenco e ritorna il suo numero. ValueError se non si puo'. da_solo: e' il
    giro della notte, partito senza il tasto."""
    with _lock:
        if _giro['id'] is not None:
            raise ValueError('C\'è già un giro in corso.')
        if _cerca['in_corso']:
            raise ValueError('Sta girando la ricerca di nuovi casi: aspetta che finisca, o fermala.')
        if test_realtime.in_corso() is not None:
            raise ValueError('Sta girando un test in tempo reale: aspetta che finisca, o fermalo.')
        tests = elenco(lavoro)
        if not tests:
            raise ValueError('Non ci sono test notturni da eseguire.')
        gid = max(_giri(lavoro), default=0) + 1
        nodi = limite(lavoro)
        _scrivi(_cartella(lavoro, 'giri', '%d.json' % gid),
                {'id': gid, 'inizio': time.time(), 'fine': None, 'limite': nodi, 'stato': 'in corso', 'adesso': None,
                 'da_solo': da_solo, 'test': [t['id'] for t in tests], 'risultati': {}})
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
    """Il giro della notte: se e' acceso, a che ora ('HH:MM'), e l'ultima volta che e' saltato (o None)."""
    i = _impostazioni(lavoro)
    return {'acceso': i.get('notte_acceso', False), 'ora': i.get('notte_ora', ORA_DELLA_NOTTE),
            'saltato': i.get('notte_saltato')}


def cambia_notte(lavoro, acceso, ora):
    _imposta(lavoro, notte_acceso=acceso, notte_ora=ora)


def controlla_la_notte(lavoro, adesso):
    """Se il giro della notte e' acceso, `adesso` (un datetime) e' nel minuto scelto e oggi non e' ancora scattato,
    avvia il giro. Se gira altro il giro di questa notte salta, e se ne tiene il motivo; con l'elenco vuoto non
    succede niente."""
    n, oggi = notte(lavoro), adesso.strftime('%Y-%m-%d')
    if not n['acceso'] or adesso.strftime('%H:%M') != n['ora'] or _impostazioni(lavoro).get('notte_giorno') == oggi:
        return
    _imposta(lavoro, notte_giorno=oggi)
    if not elenco(lavoro):
        return
    motivo = {'giro': 'c\'era già un giro in corso.',
              'ricerca': 'stava girando la ricerca di nuovi casi.'}.get(occupato())
    if motivo is None and test_realtime.in_corso() is not None:
        motivo = 'stava girando un test in tempo reale.'
    try:
        if motivo is None:
            avvia_giro(lavoro, da_solo=True)
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


# ---- cercare nuovi casi ----

def cerca_in_corso():
    with _lock:
        return _cerca['in_corso']


def ricerca(lavoro):
    """L'ultima ricerca di nuovi casi (o None): quanti ne ha trovati e aggiunti, e se sta ancora girando."""
    return _leggi(_cartella(lavoro, 'ricerca.json'), None)


def _cerca_casi(lavoro, exe, r):
    """Lancia ./untitled test N ORDINI LIMITE e legge le righe man mano: ogni caso che arriva al limite entra nei test
    notturni, se non c'e' gia'."""
    percorso = _cartella(lavoro, 'ricerca.json')
    try:
        p = subprocess.Popen([exe, 'test', str(r['n']), str(r['ordini']), str(r['limite'])],
                             stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, start_new_session=True)
        with _lock:
            _cerca['processo'] = p
        for riga in p.stdout:
            m = _RIGA.match(riga)
            if not m:
                continue
            _, nuovo = aggiungi(lavoro, int(m.group(1)), int(m.group(2)), m.group(3) == 'con', int(m.group(4) or 0))
            r['trovati'] += 1
            r['aggiunti' if nuovo else 'gia_presenti'] += 1
            r['ultimo'] = riga.split(':')[0]
            _scrivi(percorso, r)
        r['stato'] = 'finita' if p.wait() == 0 else 'fermata'
    except Exception as e:
        r['stato'] = 'errore: %s' % e
    finally:
        r['fine'] = time.time()
        _scrivi(percorso, r)
        with _lock:
            _cerca.update(in_corso=False, processo=None)


def cerca(lavoro, n, ordini):
    """Cerca nuovi casi: le stesse prove di ./untitled test N (semi da 0 a N, classi da 3 a 3 + N, senza e con
    ritardi), per `ordini` ordini delle classi e con il limite dei test notturni. In sottofondo. ValueError se non si
    puo'."""
    with _lock:
        if _cerca['in_corso']:
            raise ValueError('La ricerca di nuovi casi sta già girando.')
        if _giro['id'] is not None:
            raise ValueError('C\'è un giro in corso: aspetta che finisca, o fermalo.')
        if test_realtime.in_corso() is not None:
            raise ValueError('Sta girando un test in tempo reale: aspetta che finisca, o fermalo.')
        _cerca.update(in_corso=True, processo=None)
    try:
        exe = solver.compila(os.path.join(lavoro, 'bin'))
    except Exception:
        with _lock:
            _cerca.update(in_corso=False, processo=None)
        raise
    r = {'n': n, 'ordini': ordini, 'limite': limite(lavoro), 'prove': (n + 1) * (n + 1) * 2 * ordini,
         'inizio': time.time(), 'fine': None, 'stato': 'in corso', 'trovati': 0, 'aggiunti': 0, 'gia_presenti': 0,
         'ultimo': None}
    _scrivi(_cartella(lavoro, 'ricerca.json'), r)
    threading.Thread(target=_cerca_casi, args=(lavoro, exe, r), daemon=True).start()


def ferma_ricerca():
    """Ferma la ricerca di nuovi casi: quelli gia' trovati restano nell'elenco."""
    with _lock:
        if _cerca['processo'] is not None:  # ogni prova gira in un processo figlio: si ferma tutto il gruppo
            os.killpg(_cerca['processo'].pid, signal.SIGTERM)


def occupato():
    """Che cosa sta girando nei test notturni: 'giro', 'ricerca' o None. Finche' gira, niente test in tempo reale."""
    with _lock:
        return 'giro' if _giro['id'] is not None else 'ricerca' if _cerca['in_corso'] else None


def chiudi_giri_interrotti(lavoro):
    """All'avvio del sito: un giro rimasto «in corso» (il sito si e' fermato mentre girava) risulta «interrotto»."""
    for gid in _giri(lavoro):
        g = giro(lavoro, gid)
        if g and g.get('stato') == 'in corso':
            g.update(stato='interrotto', adesso=None, fine=g.get('fine') or time.time())
            _scrivi(_cartella(lavoro, 'giri', '%d.json' % gid), g)
    r = ricerca(lavoro)
    if r and r.get('stato') == 'in corso':  # lo stesso per una ricerca di nuovi casi
        r.update(stato='interrotta', fine=time.time())
        _scrivi(_cartella(lavoro, 'ricerca.json'), r)


def situazione(lavoro):
    """Per la pagina: il limite, il giro in corso (o l'ultimo) e i test, ognuno con il suo ultimo risultato."""
    g = ultimo_giro(lavoro)
    nodi = limite(lavoro)
    quale = max(_giri(lavoro), default=0) + 1
    tests = []
    for t in elenco(lavoro):
        r = (g or {}).get('risultati', {}).get(str(t['id']))
        if r is None:  # non e' nell'ultimo giro (aggiunto dopo, o giro fermato prima): l'ultimo risultato che ha
            r, limite_prima = _prima(lavoro, t['id'], nodi, quale)
            if r is not None and limite_prima != nodi:
                r = dict(r, limite=limite_prima)
        tests.append(dict(t, risultato=r))
    return {'limite': nodi, 'giro': g, 'in_corso': in_corso(), 'tests': tests,
            'realtime': test_realtime.in_corso(), 'ricerca': ricerca(lavoro), 'cerca_in_corso': cerca_in_corso(),
            'notte': notte(lavoro)}
