"""I test notturni: un elenco di casi (seme, classi, con o senza ritardi) che si rifanno tutti insieme, in un giro,
per vedere se i risultati cambiano quando cambia il programma.

Tutto sta in lavoro/notti/:
- casi.json: i casi, ognuno con il suo numero (mai riusato: i giri ricordano i risultati per numero);
- impostazioni.json: i secondi che ogni caso ha a disposizione, e il giro della notte (se parte da solo, e a che ora);
- giri/<n>.json: un giro, con il tempo usato e il risultato di ogni caso;
- visti.json: per ogni caso, il risultato di cui si e' gia' presa visione.

Un giro esegue i casi uno dopo l'altro. Un giro alla volta, e mai insieme a una prova. Il giro della notte, se e' acceso, parte da solo una volta al giorno all'ora scelta.

I cambiamenti: il risultato di un caso si confronta con quello di cui si e' presa visione (la prima volta, il primo
che ha avuto con quel tempo). Se e' diverso il caso resta tra i cambiamenti finche' non lo si conferma."""
import datetime
import os
import shutil
import threading
import time

from . import archivio, prove, solver

TEMPO = 120                          # i secondi che ogni caso ha, finche' non lo si cambia dalla pagina
_lock = threading.Lock()             # le richieste arrivano da piu' fili insieme: una alla volta qui
# il giro in corso: il suo numero, il processo che sta girando, e se e' stato chiesto di fermarlo
_giro = {'numero': None, 'processo': None, 'fermare': False}
_orologio = {'partito': False}       # l'orologio del giro della notte parte una volta sola


def _file(*pezzi):
    return archivio.percorso('notti', *pezzi)


def impostazioni():
    i = archivio.leggi(_file('impostazioni.json'), {})
    return {'tempo': i.get('tempo', TEMPO), 'notte': i.get('notte', False), 'ora': i.get('ora', '22:00'),
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


def _esegui_caso(caso, tempo):
    """Un caso: il programma in una cartella provvisoria. Il suo risultato, o None se il giro e' stato fermato."""
    # una cartella provvisoria, vuota a ogni caso e buttata alla fine
    d = _file('provvisoria')
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    try:
        try:
            solver.genera(caso['seme'], caso['classi'], caso['ritardi'], os.path.join(d, 'caso.txt'))
        except ValueError:
            return {'esito': 'non generato'}
        p = solver.risolvi(os.path.join(d, 'caso.txt'), os.path.join(d, 'stato.json'), tempo)
        with _lock:
            # il processo si ricorda, cosi' «Ferma» lo puo' chiudere
            _giro['processo'] = p
        p.wait()
        with _lock:
            _giro['processo'] = None
            if _giro['fermare']:
                return None
        s = archivio.leggi(os.path.join(d, 'stato.json'), {})
        # il programma e' uscito senza scrivere uno stato finale: qualcosa e' andato storto
        if s.get('in_corso', 1):
            return {'esito': 'non riuscito'}
        # il risultato del caso: l'esito, il tempo, e se l'orario trovato passa la verifica
        r = {k: s.get(k) for k in ('esito', 'secondi', 'celle')}
        if 'verifica' in s:
            r['verificato'] = bool(s['verifica']['ok'])
        return r
    finally:
        shutil.rmtree(d, ignore_errors=True)


def _esegui_giro(numero, tempo, elenco):
    """Il giro, in un filo a parte: un caso dopo l'altro, scrivendo il file del giro a ogni caso (la pagina lo legge
    mentre va). Finisce «finito», «fermato» o con un errore."""
    g = giro(numero)
    try:
        for caso in elenco:
            # il caso che sta girando si scrive nel file del giro: la pagina lo mostra
            g['adesso'] = caso['numero']
            archivio.scrivi(_file('giri', '%d.json' % numero), g)
            r = _esegui_caso(caso, tempo)
            if r is None:
                g['stato'] = 'fermato'
                break
            g['risultati'][str(caso['numero'])] = r
        else:
            g['stato'] = 'finito'
    except Exception as e:   # il disco pieno, un file che non si scrive...: il giro finisce e lo dice
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
            raise ValueError('Sta girando una prova: aspetta che finisca, o fermala.')
        elenco = casi()
        if not elenco:
            raise ValueError('Non ci sono casi da eseguire.')
        # il numero del giro e il tempo di adesso; il file del giro nasce subito, «in corso»
        numero = max(archivio.numeri(_file('giri')), default=0) + 1
        tempo = archivio.leggi(_file('impostazioni.json'), {}).get('tempo', TEMPO)
        archivio.scrivi(_file('giri', '%d.json' % numero),
                        {'numero': numero, 'inizio': time.time(), 'fine': None, 'tempo': tempo, 'stato': 'in corso',
                         'adesso': None, 'da_solo': da_solo, 'casi': len(elenco), 'risultati': {}})
        _giro.update(numero=numero, processo=None, fermare=False)
    # il giro va in un filo a parte: la richiesta torna subito alla pagina
    threading.Thread(target=_esegui_giro, args=(numero, tempo, elenco), daemon=True).start()
    return numero


def ferma_giro():
    """Ferma il giro in corso: il processo che sta girando viene chiuso, e il giro finisce «fermato»."""
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
                # e' l'ora scelta, il giro della notte e' acceso, e oggi non e' ancora partito
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
               ('non risolto', 'Risolto → Non risolto'), ('altro', 'Altri cambiamenti'))


def _chiave(r, tempo):
    """Quello che identifica un risultato: l'esito, e il tempo che il caso aveva. Due risultati con la stessa chiave
    sono uguali."""
    return [r.get('esito'), tempo]


def cambiamento(prima, adesso):
    """Che cosa e' cambiato tra due risultati dello stesso caso: una chiave di CAMBIAMENTI.
    Allarme: un orario che non passa la verifica, un caso non riuscito, o trovato dove prima era impossibile (o il
    contrario): una delle due risposte e' sbagliata."""
    a, b = prima.get('esito'), adesso.get('esito')
    # gli allarmi vengono prima di tutto
    if adesso.get('verificato') is False or b in ('non riuscito', 'non generato') or {a, b} == {TROVATO, IMPOSSIBILE}:
        return 'allarme'
    # esito diverso: conta se si e' guadagnato o perso un orario
    return 'risolto' if b == TROVATO else 'non risolto' if a == TROVATO else 'altro'


def _storie():
    """Per ogni caso (per numero) le sue esecuzioni, dalla piu' vecchia: il risultato con il giro e il tempo."""
    storie = {}
    for n in archivio.numeri(_file('giri')):
        g = giro(n) or {}
        for numero, r in g.get('risultati', {}).items():
            storie.setdefault(int(numero), []).append(dict(r, giro=n, tempo=g.get('tempo')))
    return storie


def situazione():
    """Per la pagina: le impostazioni, il giro in corso (o l'ultimo), i casi con l'ultimo risultato e quello prima, e
    i cambiamenti, una tabella per tipo: i casi il cui ultimo risultato non e' quello di cui si e' presa visione."""
    storie, visti = _storie(), archivio.leggi(_file('visti.json'), {})
    giri = archivio.numeri(_file('giri'))
    elenco, tabelle = [], {chiave: [] for chiave, _ in CAMBIAMENTI}
    for c in casi():
        storia = storie.get(c['numero'], [])
        # l'ultimo risultato e quello prima servono alla tabella dei casi
        ultimo = storia[-1] if storia else None
        elenco.append(dict(c, ultimo=ultimo, prima=storia[-2] if len(storia) > 1 else None))
        if not ultimo:
            continue
        stesse = [x for x in storia if x['tempo'] == ultimo['tempo']]   # le esecuzioni con il tempo di adesso
        visto = visti.get(str(c['numero']))
        # il riferimento: il risultato di cui si e' presa visione, se era con questo tempo; se no il primo avuto
        riferimento = next((x for x in stesse if _chiave(x, x['tempo']) == visto), stesse[0])
        suo = _chiave(ultimo, ultimo['tempo'])
        # cambiato rispetto al riferimento; un orario che non passa la verifica si mostra anche se e' il primo
        nuovo = _chiave(riferimento, riferimento['tempo']) != suo
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
                # da adesso il riferimento di questo caso e' il suo ultimo risultato
                visti[str(n)] = _chiave(storie[n][-1], storie[n][-1]['tempo'])
        archivio.scrivi(_file('visti.json'), visti)
