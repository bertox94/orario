"""I test notturni: un elenco di casi (seme, classi, con o senza ritardi) che si rieseguono tutti insieme, in un giro,
per vedere se i risultati cambiano quando cambia il solver.

Tutto sta in lavoro/notti/:
- casi.json: i casi, ognuno con il suo numero (mai riusato: i giri ricordano i risultati per numero);
- impostazioni.json: il limite di nodi, uno per tutti, e il giro della notte (se parte da solo, e a che ora);
- giri/<n>.json: un giro, con il limite usato e il risultato di ogni caso;
- visti.json: per ogni caso, il risultato di cui si e' gia' presa visione.

Un giro esegue i casi uno dopo l'altro, senza registrare i passi. Un giro alla volta, e mai insieme a una prova in
tempo reale. Il giro della notte, se e' acceso, parte da solo una volta al giorno all'ora scelta.

I cambiamenti: il risultato di un caso si confronta con quello di cui si e' presa visione (la prima volta, il primo
che ha avuto con quel limite di nodi). Se e' diverso il caso resta tra i cambiamenti finche' non lo si conferma."""
import datetime
import os
import shutil
import threading
import time

from . import archivio, prove, solver

LIMITE = 2000000
CONFRONTATI = ('esito', 'nodi')
_lock = threading.Lock()
_giro = {'numero': None, 'processo': None, 'fermare': False}
_orologio = {'partito': False}


def _file(*pezzi):
    return archivio.percorso('notti', *pezzi)


def impostazioni():
    i = archivio.leggi(_file('impostazioni.json'), {})
    return {'limite': i.get('limite', LIMITE), 'notte': i.get('notte', False), 'ora': i.get('ora', '22:00'),
            'ultimo_numero': i.get('ultimo_numero', 0), 'ultima_notte': i.get('ultima_notte')}


def imposta(**valori):
    with _lock:
        archivio.scrivi(_file('impostazioni.json'), dict(impostazioni(), **valori))


def casi():
    return archivio.leggi(_file('casi.json'), [])


def aggiungi(nuovi):
    """Aggiunge i casi (seme, classi, ritardi) che non ci sono gia'. Ritorna quanti ne ha aggiunti."""
    with _lock:
        tutti, i = casi(), impostazioni()
        presenti = {(c['seme'], c['classi'], c['ritardi']) for c in tutti}
        aggiunti = 0
        for seme, classi, ritardi in nuovi:
            if (seme, classi, ritardi) in presenti:
                continue
            presenti.add((seme, classi, ritardi))
            aggiunti += 1
            tutti.append({'numero': i['ultimo_numero'] + aggiunti, 'seme': seme, 'classi': classi, 'ritardi': ritardi})
        archivio.scrivi(_file('casi.json'), tutti)
        archivio.scrivi(_file('impostazioni.json'), dict(i, ultimo_numero=i['ultimo_numero'] + aggiunti))
        return aggiunti


def togli(numeri):
    """Toglie dall'elenco i casi con questi numeri. I loro risultati restano nei giri gia' fatti."""
    with _lock:
        archivio.scrivi(_file('casi.json'), [c for c in casi() if c['numero'] not in set(numeri)])


# ---- i giri ----

def giro(numero):
    return archivio.leggi(_file('giri', '%d.json' % numero))


def in_corso():
    with _lock:
        return _giro['numero']


def _esegui_caso(caso, limite):
    """Un caso: il solver in una cartella provvisoria. Il suo risultato, o None se il giro e' stato fermato."""
    d = _file('provvisoria')
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    try:
        try:
            solver.genera(caso['seme'], caso['classi'], caso['ritardi'], os.path.join(d, 'caso.txt'))
        except ValueError:
            return {'esito': 'non generato'}
        p = solver.risolvi('caso.txt', limite, 'stato.json', cartella=d)
        with _lock:
            _giro['processo'] = p
        p.wait()
        with _lock:
            _giro['processo'] = None
            if _giro['fermare']:
                return None
        s = archivio.leggi(os.path.join(d, 'stato.json'), {})
        if s.get('in_corso', 1):
            return {'esito': 'non riuscito'}
        r = {k: s.get(k) for k in ('esito', 'nodi', 'tentativi', 'secondi', 'profondita_massima', 'celle')}
        if 'verifica' in s:
            r['verificato'] = bool(s['verifica']['ok'])
        return r
    finally:
        shutil.rmtree(d, ignore_errors=True)


def _esegui_giro(numero, limite, elenco):
    g = giro(numero)
    try:
        solver.programma()
        for caso in elenco:
            g['adesso'] = caso['numero']
            archivio.scrivi(_file('giri', '%d.json' % numero), g)
            r = _esegui_caso(caso, limite)
            if r is None:
                g['stato'] = 'fermato'
                break
            g['risultati'][str(caso['numero'])] = r
        else:
            g['stato'] = 'finito'
    except Exception as e:   # il solver che non compila, il disco pieno...: il giro finisce e lo dice
        g['stato'] = 'errore: %s' % e
    finally:
        g.update(adesso=None, fine=time.time())
        archivio.scrivi(_file('giri', '%d.json' % numero), g)
        with _lock:
            _giro.update(numero=None, processo=None, fermare=False)


def avvia_giro(da_solo=False):
    """Lancia un giro su tutti i casi e ritorna il suo numero. ValueError se non si puo'."""
    with _lock:
        if _giro['numero'] is not None:
            raise ValueError('C\'è già un giro in corso.')
        if prove.in_corso() is not None:
            raise ValueError('Sta girando una prova in tempo reale: aspetta che finisca, o fermala.')
        elenco = casi()
        if not elenco:
            raise ValueError('Non ci sono casi da eseguire.')
        numero = max(archivio.numeri(_file('giri')), default=0) + 1
        limite = archivio.leggi(_file('impostazioni.json'), {}).get('limite', LIMITE)
        archivio.scrivi(_file('giri', '%d.json' % numero),
                        {'numero': numero, 'inizio': time.time(), 'fine': None, 'limite': limite, 'stato': 'in corso',
                         'adesso': None, 'da_solo': da_solo, 'casi': len(elenco), 'risultati': {}})
        _giro.update(numero=numero, processo=None, fermare=False)
    threading.Thread(target=_esegui_giro, args=(numero, limite, elenco), daemon=True).start()
    return numero


def ferma_giro():
    with _lock:
        if _giro['numero'] is None:
            return
        _giro['fermare'] = True
        if _giro['processo'] is not None:
            _giro['processo'].terminate()


def all_avvio():
    """Quando il sito parte: un giro rimasto «in corso» (il sito si e' fermato mentre girava) risulta interrotto, e
    parte l'orologio che ogni venti secondi guarda se e' l'ora del giro della notte."""
    for n in archivio.numeri(_file('giri')):
        g = giro(n)
        if g and g.get('stato') == 'in corso':
            archivio.scrivi(_file('giri', '%d.json' % n), dict(g, stato='interrotto', adesso=None, fine=time.time()))
    with _lock:
        if _orologio['partito']:
            return
        _orologio['partito'] = True

    def guarda():
        while True:
            try:
                adesso, i = datetime.datetime.now(), impostazioni()
                oggi = adesso.strftime('%Y-%m-%d')
                if i['notte'] and adesso.strftime('%H:%M') == i['ora'] and i['ultima_notte'] != oggi:
                    imposta(ultima_notte=oggi)
                    avvia_giro(da_solo=True)
            except Exception:   # gira gia' qualcosa, o un file non si legge: si riprova domani
                pass
            time.sleep(20)
    threading.Thread(target=guarda, daemon=True).start()


# ---- la situazione e i cambiamenti ----

TROVATO, IMPOSSIBILE = 'trovato', 'impossibile'
# i tipi di cambiamento, una tabella l'uno nella pagina, in quest'ordine
CAMBIAMENTI = (('allarme', 'Allarmi'), ('risolto', 'Non risolto → Risolto'),
               ('non risolto', 'Risolto → Non risolto'), ('meno nodi', 'Stesso esito, meno nodi'),
               ('piu nodi', 'Stesso esito, più nodi'),
               ('altro', 'Altri cambiamenti'))


def _chiave(r, limite):
    return [r.get('esito'), r.get('nodi'), limite]


def cambiamento(prima, adesso):
    """Che cosa e' cambiato tra due risultati dello stesso caso con lo stesso limite: una chiave di CAMBIAMENTI.
    Allarme: un orario che non passa la verifica, un caso non riuscito, o trovato dove prima era impossibile (o il
    contrario): una delle due risposte e' sbagliata."""
    a, b = prima.get('esito'), adesso.get('esito')
    if adesso.get('verificato') is False or b in ('non riuscito', 'non generato') or {a, b} == {TROVATO, IMPOSSIBILE}:
        return 'allarme'
    if a != b:
        return 'risolto' if b == TROVATO else 'non risolto' if a == TROVATO else 'altro'
    if None in (prima.get('nodi'), adesso.get('nodi')) or prima['nodi'] == adesso['nodi']:
        return 'altro'
    return 'meno nodi' if adesso['nodi'] < prima['nodi'] else 'piu nodi'


def _storie():
    """Per ogni caso (per numero) le sue esecuzioni, dalla piu' vecchia: il risultato con il giro e il limite."""
    storie = {}
    for n in archivio.numeri(_file('giri')):
        g = giro(n) or {}
        for numero, r in g.get('risultati', {}).items():
            storie.setdefault(int(numero), []).append(dict(r, giro=n, limite=g.get('limite')))
    return storie


def situazione():
    """Per la pagina: le impostazioni, il giro in corso (o l'ultimo), i casi con l'ultimo risultato e quello prima, e
    i cambiamenti, una tabella per tipo: i casi il cui ultimo risultato non e' quello di cui si e' presa visione."""
    storie, visti = _storie(), archivio.leggi(_file('visti.json'), {})
    giri = archivio.numeri(_file('giri'))
    elenco, tabelle = [], {chiave: [] for chiave, _ in CAMBIAMENTI}
    for c in casi():
        storia = storie.get(c['numero'], [])
        ultimo = storia[-1] if storia else None
        elenco.append(dict(c, ultimo=ultimo, prima=storia[-2] if len(storia) > 1 else None))
        if not ultimo:
            continue
        stesse = [x for x in storia if x['limite'] == ultimo['limite']]
        visto = visti.get(str(c['numero']))
        # il riferimento: il risultato di cui si e' presa visione, se era con questo limite; se no il primo avuto
        riferimento = next((x for x in stesse if _chiave(x, x['limite']) == visto), stesse[0])
        suo = _chiave(ultimo, ultimo['limite'])
        # cambiato rispetto al riferimento; un orario che non passa la verifica si mostra anche se e' il primo
        nuovo = _chiave(riferimento, riferimento['limite']) != suo
        if nuovo or (ultimo.get('verificato') is False and visto != suo):
            tabelle[cambiamento(riferimento, ultimo)].append(dict(c, ultimo=ultimo, prima=riferimento))
    return {'impostazioni': impostazioni(), 'giro': giro(giri[-1]) if giri else None, 'in_corso': in_corso(),
            'prova': prove.in_corso(), 'casi': elenco,
            'cambiamenti': [{'chiave': k, 'titolo': t, 'casi': tabelle[k]} for k, t in CAMBIAMENTI if tabelle[k]]}


def visto(numeri):
    """Conferma la presa visione dei casi con questi numeri: il loro ultimo risultato diventa il riferimento."""
    storie = _storie()
    with _lock:
        visti = archivio.leggi(_file('visti.json'), {})
        for n in numeri:
            if storie.get(n):
                visti[str(n)] = _chiave(storie[n][-1], storie[n][-1]['limite'])
        archivio.scrivi(_file('visti.json'), visti)
