"""I test notturni: un elenco di casi (seme, classi, ritardi, ordine delle classi) che si rieseguono tutti insieme, in
un giro, per vedere se i loro numeri cambiano.

Tutto sta in lavoro/notturni/, in file JSON:
- test.json: l'elenco dei test, ognuno con il suo numero;
- impostazioni.json: il limite di nodi, uno solo per tutti i test (lo usa il giro, e si cambia dalla pagina), e il
  giro della notte: se parte da solo, a che ora, l'ultimo giorno in cui e' scattato e l'ultima volta che e' saltato;
- giri/<n>.json: un giro, con il limite usato e il risultato di ogni test (esito, nodi, cella piu' avanti, tempo);
- visti.json: per ogni test, il risultato di cui e' stata confermata la presa visione (il report, «I cambiamenti»).

Un giro esegue i test uno dopo l'altro con ./untitled caso (caso.h), in una cartella provvisoria: del test si tengono
solo i numeri, il file dei passi si cancella. Ogni risultato si confronta con l'ultimo dello stesso test fatto con lo
stesso limite: «uguale» o «diverso» (esito, nodi, cella piu' avanti; il tempo no, dipende dal computer). Con un
limite diverso i numeri non si confrontano: «limite cambiato». Un giro alla volta, e mai insieme a un test in tempo
reale.

Il giro della notte: se e' acceso, all'ora scelta il sito aggiunge all'elenco i casi di base con un ordine delle
classi nuovo, e avvia il giro da solo, una volta al giorno. I casi di base si scelgono nella pagina: i semi da 0 a
quello scelto e le classi da... a..., senza e con ritardi, senza filtri. La prima sera entrano con l'ordine 0 (quello
generato), la seconda con l'ordine 1, e cosi' via: gli stessi dati, con le classi in un altro ordine, cioe' un'altra
strada della ricerca. Il giro dovrebbe finire entro l'ora di fine: se va oltre finisce lo stesso, ma risulta fuori
tempo e dalla sera dopo non si aggiungono altri ordini, finche' un giro della notte non torna a finire in tempo (i test
si tolgono a mano). Se all'ora di partenza gira altro (un giro o un test in tempo reale) quella notte salta, senza
test nuovi, e la pagina lo dice. Se il sito a quell'ora era spento, si aspetta la notte dopo.
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
# i casi di base del giro della notte, finche' non si cambiano dalla pagina: i semi da 0 a SEMI, le classi da
# CLASSI_DA a CLASSI_A
SEMI, CLASSI_DA, CLASSI_A = 0, 3, 50
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


def _parametri(t):
    """Quello che fa di un test quel test: due test con gli stessi parametri sono lo stesso."""
    return t['seme'], t['classi'], t['ritardi'], t['ordine']


def aggiungi(lavoro, seme, classi, ritardi, ordine, da_solo=False):
    """Aggiunge il caso ai test notturni, se non c'e' gia' (seme, classi, ritardi e ordine uguali). Ritorna il test, e
    se e' nuovo. da_solo: lo ha aggiunto il giro della notte."""
    with _lock:
        tutti = elenco(lavoro)
        for t in tutti:
            if _parametri(t) == (seme, classi, ritardi, ordine):
                return t, False
        t = {'id': _numero_nuovo(lavoro, tutti), 'seme': seme, 'classi': classi, 'ritardi': ritardi,
             'ordine': ordine, 'aggiunto': time.time(), 'da_solo': da_solo}
        _scrivi(_cartella(lavoro, 'test.json'), tutti + [t])
        return t, True


def aggiungi_tanti(lavoro, casi, da_solo=False):
    """Aggiunge tanti casi (seme, classi, ritardi, ordine) in una volta sola, saltando quelli che ci sono gia'."""
    with _lock:
        tutti = elenco(lavoro)
        presenti = {_parametri(t) for t in tutti}
        for seme, classi, ritardi, ordine in casi:
            if (seme, classi, ritardi, ordine) in presenti:
                continue
            presenti.add((seme, classi, ritardi, ordine))
            tutti.append({'id': _numero_nuovo(lavoro, tutti), 'seme': seme, 'classi': classi, 'ritardi': ritardi,
                          'ordine': ordine, 'aggiunto': time.time(), 'da_solo': da_solo})
        _scrivi(_cartella(lavoro, 'test.json'), tutti)


def togli(lavoro, tid):
    togli_tanti(lavoro, [tid])


def togli_tanti(lavoro, ids):
    """Toglie dall'elenco i test con questi numeri. I loro risultati restano nei giri gia' fatti."""
    ids = set(ids)
    with _lock:
        _scrivi(_cartella(lavoro, 'test.json'), [t for t in elenco(lavoro) if t['id'] not in ids])


# ---- i giri ----

def _giri(lavoro):
    """I numeri dei giri fatti, in ordine."""
    d = _cartella(lavoro, 'giri')
    return sorted(int(x[:-5]) for x in os.listdir(d) if x.endswith('.json') and x[:-5].isdigit()) \
        if os.path.isdir(d) else []


def giro(lavoro, gid):
    return _leggi(_cartella(lavoro, 'giri', '%d.json' % gid), None)


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


# ---- il report: le contraddizioni, i cambiamenti e i casi per stato ----

# un orario non c'e': lo dice la ricerca, arrivata in fondo, o la verifica dei dati (condizioni necessarie)
IMPOSSIBILE = ('nessuna soluzione', 'errore nei dati')
RISPOSTA = ('trovato',) + IMPOSSIBILE   # il test ha una risposta: l'orario c'e', o non c'e'
# gli stati di un caso (seme, classi, ritardi) guardando tutti i suoi ordini delle classi, nell'ordine della pagina
STATI = ('impossibile', 'sempre trovato', 'trovato con alcuni ordini', 'mai trovato')


def _chiave(r):
    """Quello che di un'esecuzione si conferma con «Visto»: l'esito e i nodi, con il limite di quel giro."""
    return [r.get('esito'), r.get('nodi'), r.get('limite')]


def _visti(lavoro):
    """Per numero di test, la chiave (_chiave) dell'esecuzione di cui e' stata confermata la presa visione."""
    return _leggi(_cartella(lavoro, 'visti.json'), {})


def _da_verificare(tutte, visto):
    """Se l'ultima esecuzione di un test e' da verificare: (giudizio, record, frase), oppure None.
    tutte: le esecuzioni del test, dalla piu' vecchia; visto: la chiave dell'esecuzione confermata con «Visto».
    Il meglio di sempre vale per un limite di nodi: si guardano solo le esecuzioni fatte con il limite dell'ultima, e
    cambiando il limite si riparte da capo (tranne che per gli allarmi sulle risposte, che valgono con ogni limite).
    Se con questo limite non c'e' un «Visto», vale la prima esecuzione: la prima volta non c'e' niente da verificare.
    Un test e' da verificare quando il suo risultato non e' quello visto e, rispetto al meglio di sempre, e':
    - allarme: non riuscito, un orario che non passa la verifica, oppure trovato dove una volta (con qualunque
      limite) era impossibile, o il contrario: una delle due risposte e' sbagliata;
    - migliorato: una risposta dove c'era solo il limite, o la stessa risposta con meno nodi del record;
    - tornato al record: la stessa risposta con i nodi del record (dopo essere stata diversa);
    - peggiorato: la stessa risposta con piu' nodi del record, oppure aveva una risposta e ora si ferma al limite;
    - diverso: un'altra risposta dello stesso tipo (nessuna soluzione ed errore nei dati).
    Il record e' la risposta con meno nodi tra le esecuzioni prima di quelle uguali all'ultima. Un test fermo al
    limite che con questo limite non ha mai avuto una risposta resta fuori."""
    u = tutte[-1]
    stesse = [x for x in tutte if x['limite'] == u['limite']]
    if _chiave(u) == (visto if visto is not None and visto[2:] == [u['limite']] else _chiave(stesse[0])):
        return None
    if u.get('esito') == 'non riuscito':
        return 'allarme', None, 'il test non è riuscito'
    if u.get('verificato') is False:
        return 'allarme', None, 'l\'orario trovato non passa la verifica'
    if u.get('esito') in RISPOSTA:
        for x in reversed(tutte):
            if x.get('esito') in RISPOSTA and (x['esito'] == 'trovato') != (u['esito'] == 'trovato'):
                return 'allarme', x, 'nel giro %d era «%s»: una delle due risposte è sbagliata' % (x['giro'], x['esito'])
    while stesse and _chiave(stesse[-1]) == _chiave(u):
        stesse.pop()
    risposte = [x for x in stesse if x.get('esito') in RISPOSTA]
    record = min(risposte, key=lambda x: x.get('nodi') or 0, default=None)
    if u.get('esito') == 'limite di nodi':
        return ('peggiorato', record, 'aveva una risposta, ora si ferma al limite') if record else None
    if u.get('esito') not in RISPOSTA:
        return None
    if record is None:
        return 'migliorato', None, 'prima si fermava al limite'
    if record['esito'] != u['esito'] or None in (record.get('nodi'), u.get('nodi')):
        return 'diverso', record, 'prima «%s»' % record['esito']
    if u['nodi'] == record['nodi']:
        return 'tornato al record', record, 'di nuovo i nodi del record'
    return ('migliorato' if u['nodi'] < record['nodi'] else 'peggiorato'), record, None


def _test_del_report(lavoro):
    """Test per test, dall'ultima esecuzione di ognuno: (righe, da verificare, quanti mai eseguiti).
    - righe: caso, ordine, esito, nodi, cella piu' avanti, giro e limite dell'ultima esecuzione, piu' il record: la
      stessa risposta con meno nodi di sempre, tra le esecuzioni fatte con il limite dell'ultima;
    - da verificare: le righe dei test il cui risultato si scosta dal meglio di sempre (_da_verificare), con il
      giudizio, la frase e il record con cui si confrontano."""
    giri = [x for x in (giro(lavoro, gid) for gid in _giri(lavoro)) if x]   # dal piu' vecchio
    visti = _visti(lavoro)
    righe, da_verificare, mai = [], [], 0
    for t in elenco(lavoro):
        tutte = [dict(x['risultati'][str(t['id'])], giro=x['id'], limite=x['limite'])
                 for x in giri if str(t['id']) in x.get('risultati', {}) and _e_suo(t, x)]
        if not tutte:
            mai += 1
            continue
        u = tutte[-1]
        riga = dict({k: t[k] for k in ('id', 'seme', 'classi', 'ritardi', 'ordine')},
                    **{k: u.get(k) for k in ('esito', 'nodi', 'pos_max', 'celle', 'giro', 'limite')})
        stesse = [x for x in tutte if x.get('esito') == u.get('esito') and x.get('nodi') is not None
                  and x['limite'] == u['limite']]
        riga['record'] = min(x['nodi'] for x in stesse) if stesse and u.get('esito') in RISPOSTA else None
        righe.append(riga)
        v = _da_verificare(tutte, visti.get(str(t['id'])))
        if v:
            giudizio, record, frase = v
            if giudizio == 'allarme':   # l'esecuzione che contraddice l'ultima non e' un record: e' gia' nella frase
                record = None
            da_verificare.append(dict(riga, giudizio=giudizio, frase=frase, record=record and record.get('nodi'),
                                      esito_prima=record and record.get('esito'),
                                      giro_record=record and record['giro']))
    return righe, da_verificare, mai


def _caso(r):
    """Il caso di un test: gli stessi dati, qualunque sia l'ordine delle classi."""
    return r['classi'], r['seme'], r['ritardi']


def stato_del_caso(righe):
    """Lo stato di un caso dalle ultime esecuzioni dei suoi ordini (righe, almeno una): «impossibile» se nessun ordine
    puo' avere un orario (tutti nessuna soluzione o errore nei dati: i dati sono gli stessi, l'ordine non c'entra);
    altrimenti secondo quanti ordini trovano l'orario: tutti, alcuni, nessuno."""
    if all(r['esito'] in IMPOSSIBILE for r in righe):
        return 'impossibile'
    trovati = sum(r['esito'] == 'trovato' for r in righe)
    return 'sempre trovato' if trovati == len(righe) else 'trovato con alcuni ordini' if trovati else 'mai trovato'


# I cambiamenti di un caso rispetto al suo meglio di sempre, una tabella l'uno nella pagina, in quest'ordine.
CAMBIAMENTI = (('allarme', 'Allarmi'), ('risolto', 'Non risolto → Risolto'), ('non risolto', 'Risolto → Non risolto'),
               ('piu ordini', 'Più ordini trovano l\'orario'), ('meno ordini', 'Meno ordini trovano l\'orario'),
               ('meno nodi', 'Stesso esito, meno nodi'), ('piu nodi', 'Stesso esito, più nodi'),
               ('altro', 'Altri cambiamenti'))


def cambiamento_del_caso(suoi, voci):
    """Che cosa e' cambiato in un caso: una delle chiavi di CAMBIAMENTI. suoi: le ultime esecuzioni dei suoi ordini;
    voci: quelle, tra loro, che si scostano dal meglio di sempre (_da_verificare), almeno una.
    Un caso e' risolto quando almeno un ordine trova l'orario. Prima di tutto gli allarmi; poi si contano gli ordini
    che trovano l'orario adesso e quelli che lo trovavano nel meglio di sempre (un ordine che non e' tra le voci non
    e' cambiato): da nessuno ad alcuni il caso e' risolto, da alcuni a nessuno non lo e' piu', altrimenti sono di
    piu' o di meno. Se sono gli stessi, i nodi: con un ordine peggiorato «piu' nodi», altrimenti «meno nodi» (anche
    tornato al record). Il resto (per esempio un ordine che ora dice che l'orario non c'e') e' «altro»."""
    if any(v['giudizio'] == 'allarme' for v in voci):
        return 'allarme'
    cambiati = {v['id']: v for v in voci}
    adesso = sum(r['esito'] == 'trovato' for r in suoi)
    prima = sum((cambiati[r['id']]['esito_prima'] if r['id'] in cambiati else r['esito']) == 'trovato' for r in suoi)
    if adesso != prima:
        return ('risolto' if not prima else 'non risolto' if not adesso
                else 'piu ordini' if adesso > prima else 'meno ordini')
    if any(v['esito'] != v['esito_prima'] or v['record'] is None or v['nodi'] is None for v in voci):
        return 'altro'
    return 'piu nodi' if any(v['giudizio'] == 'peggiorato' for v in voci) else 'meno nodi'


def contraddizione_del_caso(suoi):
    """Se gli ordini di un caso si contraddicono: (ordini che trovano l'orario, [(ordine, esito)] di quelli che dicono
    che non c'e'), oppure None. I dati sono gli stessi e cambia solo l'ordine delle classi, quindi l'orario o c'e' o
    non c'e': se un ordine lo trova e un altro risponde «nessuna soluzione» o «errore nei dati», una delle due
    risposte e' sbagliata (se l'orario trovato passa la verifica, la seconda). Non e' un cambiamento nel tempo: vale
    finche' i risultati sono questi, e non si conferma con «Visto»."""
    trovano = [r['ordine'] for r in suoi if r['esito'] == 'trovato']
    negano = [(r['ordine'], r['esito']) for r in suoi if r['esito'] in IMPOSSIBILE]
    return (trovano, negano) if trovano and negano else None


def report(lavoro):
    """Per la pagina, caso per caso: un caso (seme, classi, ritardi) sta su una riga con tutti i suoi ordini delle
    classi, dall'ultima esecuzione di ognuno.
    - cambiamenti: i casi che hanno almeno un ordine il cui risultato si scosta dal suo meglio di sempre
      (_da_verificare), finche' non se ne conferma la presa visione (visto), con quegli ordini dentro (voci), divisi
      per tipo di cambiamento (cambiamento_del_caso): una tabella per tipo, nell'ordine di CAMBIAMENTI, solo quelle
      che hanno almeno un caso;
    - contraddizioni: i casi i cui ordini si contraddicono adesso (contraddizione_del_caso), con gli ordini che
      trovano l'orario e quelli che dicono che non c'e';
    - gruppi: per ogni stato di STATI (stato_del_caso) i suoi casi, ognuno con: quanti ordini ha e quanti trovano
      l'orario, quali si' e quali no; tra quelli che lo trovano, i nodi del migliore e del peggiore, l'ordine migliore
      e il record di sempre; tra quelli che non lo trovano, la cella piu' avanti a cui uno e' arrivato;
    - altri: quanti casi non hanno ancora nessuna esecuzione."""
    righe, verificare, _ = _test_del_report(lavoro)
    casi = {}
    for r in righe:
        casi.setdefault(_caso(r), []).append(r)
    tutti = {_caso(t) for t in elenco(lavoro)}
    gruppi = {g: [] for g in STATI}
    contraddizioni = []
    for caso in sorted(casi):
        suoi = sorted(casi[caso], key=lambda r: r['ordine'])
        contro = contraddizione_del_caso(suoi)
        if contro:
            contraddizioni.append({'classi': caso[0], 'seme': caso[1], 'ritardi': caso[2],
                                   'trovano': contro[0], 'negano': contro[1]})
        trovati = [r for r in suoi if r['esito'] == 'trovato' and r['nodi'] is not None]
        no = [r for r in suoi if r['esito'] != 'trovato']
        fermi = [r for r in no if r.get('celle') and r.get('pos_max') is not None]
        migliore = min(trovati, key=lambda r: r['nodi'], default=None)
        avanti = max(fermi, key=lambda r: min(r['pos_max'] + 1, r['celle']), default=None)
        record = [r['record'] for r in trovati if r['record'] is not None]
        gruppi[stato_del_caso(suoi)].append({
            'classi': caso[0], 'seme': caso[1], 'ritardi': caso[2], 'ordini': len(suoi), 'trovati': len(trovati),
            'ordini_trovati': [r['ordine'] for r in trovati], 'ordini_no': [r['ordine'] for r in no],
            'esiti': sorted({r['esito'] for r in no}),
            'nodi': migliore and migliore['nodi'], 'ordine_migliore': migliore and migliore['ordine'],
            'nodi_peggiore': max((r['nodi'] for r in trovati), default=None),
            'record': min(record) if record else None,
            'cella': avanti and min(avanti['pos_max'] + 1, avanti['celle']), 'celle': avanti and avanti['celle'],
            'ordine_avanti': avanti and avanti['ordine'], 'limite': max(r['limite'] for r in suoi)})
    cambiati = {}
    for v in sorted(verificare, key=lambda v: v['ordine']):
        c = cambiati.setdefault(_caso(v), {'classi': v['classi'], 'seme': v['seme'], 'ritardi': v['ritardi'],
                                           'voci': [], 'ids': []})
        c['voci'].append(v)
        c['ids'].append(v['id'])
    tabelle = {chiave: [] for chiave, _ in CAMBIAMENTI}
    for caso in sorted(cambiati):
        tabelle[cambiamento_del_caso(casi[caso], cambiati[caso]['voci'])].append(cambiati[caso])
    return {'cambiamenti': [{'chiave': chiave, 'titolo': titolo, 'casi': tabelle[chiave]}
                            for chiave, titolo in CAMBIAMENTI if tabelle[chiave]],
            'casi_cambiati': len(cambiati), 'contraddizioni': contraddizioni, 'gruppi': gruppi, 'altri': len(tutti - set(casi))}


def visto(lavoro, ids):
    """Conferma la presa visione dei test con questi numeri: si ricorda il loro ultimo risultato, e i test escono da
    «I cambiamenti» finche' il risultato non cambia di nuovo."""
    ids = {str(i) for i in ids}
    with _lock:
        visti = _visti(lavoro)
        for x in _test_del_report(lavoro)[1]:
            if str(x['id']) in ids:
                visti[str(x['id'])] = _chiave(x)
        _scrivi(_cartella(lavoro, 'visti.json'), visti)


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
    """Il giro della notte: se e' acceso, da che ora a che ora ('HH:MM'), i casi di base (semi da 0 a `semi`, classi
    da `classi_da` a `classi_a`), il prossimo ordine delle classi e quanti test nuovi porta, se l'ultimo giro della
    notte e' finito fuori tempo (allora non si aggiunge niente) e l'ultima volta che e' saltato (o None)."""
    i = _impostazioni(lavoro)
    ordine, nuovi = _prossimo_ordine(lavoro)
    return {'acceso': i.get('notte_acceso', False), 'ora': i.get('notte_ora', ORA_DELLA_NOTTE),
            'fine': i.get('notte_fine', FINE_DELLA_NOTTE), 'semi': i.get('notte_semi', SEMI),
            'classi_da': i.get('notte_classi_da', CLASSI_DA), 'classi_a': i.get('notte_classi_a', CLASSI_A),
            'ordine': ordine, 'nuovi': len(nuovi), 'fuori_tempo': _fuori_tempo(lavoro),
            'saltato': i.get('notte_saltato')}


def cambia_notte(lavoro, acceso, ora, fine, semi, classi_da, classi_a):
    _imposta(lavoro, notte_acceso=acceso, notte_ora=ora, notte_fine=fine, notte_semi=semi,
             notte_classi_da=classi_da, notte_classi_a=classi_a)


def _prossimo_ordine(lavoro):
    """Il prossimo ordine delle classi (0 la prima sera, poi uno in piu' ogni sera) e i casi di base con quell'ordine
    che non sono gia' nell'elenco: per ogni numero di classi e ogni seme scelti, senza e con ritardi."""
    i = _impostazioni(lavoro)
    ordine = i.get('notte_ordine', 0)
    presenti = {_parametri(t) for t in elenco(lavoro)}
    casi = [(seme, classi, ritardi, ordine)
            for classi in range(i.get('notte_classi_da', CLASSI_DA), i.get('notte_classi_a', CLASSI_A) + 1)
            for seme in range(i.get('notte_semi', SEMI) + 1) for ritardi in (False, True)]
    return ordine, [c for c in casi if c not in presenti]


def _fuori_tempo(lavoro):
    """L'ultimo giro della notte e' finito dopo l'ora di fine: l'elenco e' gia' troppo lungo per una notte."""
    for gid in reversed(_giri(lavoro)):
        g = giro(lavoro, gid) or {}
        if g.get('da_solo'):
            return bool(g.get('fuori_tempo'))
    return False


def controlla_la_notte(lavoro, adesso):
    """Il lavoro dell'orologio, con `adesso` (un datetime). Se il giro della notte e' acceso, e' il minuto in cui
    parte e oggi non e' ancora scattato: aggiunge all'elenco i casi di base con il prossimo ordine delle classi (a meno
    che l'ultimo giro della notte sia finito fuori tempo) e avvia il giro, che dovrebbe finire entro l'ora di fine. Se
    gira altro (anche il giro della notte prima, non ancora finito) questa notte salta, senza test nuovi, e se ne
    tiene il motivo."""
    n, oggi = notte(lavoro), adesso.strftime('%Y-%m-%d')
    if not n['acceso'] or adesso.strftime('%H:%M') != n['ora'] or _impostazioni(lavoro).get('notte_giorno') == oggi:
        return
    _imposta(lavoro, notte_giorno=oggi)
    motivo = 'c\'era già un giro in corso.' if in_corso() is not None else \
        'stava girando un test in tempo reale.' if test_realtime.in_corso() is not None else None
    try:
        if motivo is None:
            if not n['fuori_tempo']:
                ordine, nuovi = _prossimo_ordine(lavoro)
                aggiungi_tanti(lavoro, nuovi, da_solo=True)
                _imposta(lavoro, notte_ordine=ordine + 1)
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


def situazione(lavoro):
    """Per la pagina: il limite, il giro in corso (o l'ultimo) e i test. Ogni test ha il suo ultimo risultato, le
    PRECEDENTI esecuzioni prima di quello (di qualunque limite, dalla piu' recente): come sono andate lo dice la
    pagina, che le confronta (notturni.html, giudizio e andamento)."""
    nodi = limite(lavoro)
    giri = [x for x in (giro(lavoro, gid) for gid in reversed(_giri(lavoro))) if x]  # dal piu' recente
    for x in giri:  # i giri del 17 novembre 2026 hanno dentro un report, scritto a fine giro: non si usa piu'
        x.pop('report', None)
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
        tests.append(dict(t, risultato=r, prima=tutte[i + 1:i + 1 + PRECEDENTI]))
    return {'limite': nodi, 'giro': g, 'in_corso': in_corso(), 'tests': tests,
            'realtime': test_realtime.in_corso(), 'notte': notte(lavoro)}
