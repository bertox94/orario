"""I due metodi insieme: HiGHS (esatto.py) e CP-SAT (cpsat.py) cercano l'orario dello stesso caso nello stesso momento,
ognuno in un processo suo, e di ognuno si tengono l'esito e il tempo.

Perche' tutti e due: nessuno dei due e' meglio dell'altro in tutto, e inciampano su casi diversi. Basta che uno dia
una risposta certa («trovato» o «impossibile») e il caso e' deciso; l'altro continua comunque fino alla sua risposta
o al suo tempo, cosi' si vede sempre come vanno tutti e due.

Che cosa esce (lo «stato dei metodi», che finisce nello stato scritto da __main__.py):
    {"esito": "in corso" | "trovato" | "impossibile" | "limite di tempo" | "contraddizione",
     "risolto_da": nome del metodo che ha dato per primo una risposta certa (se c'e'),
     "secondi": quando e' arrivata quella risposta (o, se non c'e', quando hanno finito tutti),
     "orario", "aule", "verifica": del primo orario trovato,
     "metodi": {nome: {"esito", "secondi", "verificato" (se ha trovato un orario)}, ...}}
Un metodo puo' anche risultare «non installato» (manca OR-Tools: si va avanti con l'altro) o «errore».
«Contraddizione»: uno ha trovato un orario valido e l'altro dice che non esiste. Uno dei due sbaglia: e' un allarme.
"""
import importlib.util
import multiprocessing
import queue
import signal
import time

from . import verifica

TROVATO, IMPOSSIBILE, LIMITE_DI_TEMPO, CONTRADDIZIONE = 'trovato', 'impossibile', 'limite di tempo', 'contraddizione'
# i metodi, nell'ordine in cui si mostrano: il nome breve, come si scrive, il modulo, e il pacchetto che gli serve
METODI = (('highs', 'HiGHS', 'esatto', 'scipy'), ('cpsat', 'CP-SAT', 'cpsat', 'ortools'))


def _lavora(modulo, scuola, tempo, risposte, nome):
    """Dentro il processo di un metodo: cerca l'orario e mette la risposta nella coda."""
    try:
        esito, orario, aule, _ = importlib.import_module('orario.' + modulo).risolvi(scuola, tempo)
        risposte.put((nome, esito, orario, aule))
    except Exception as e:   # un metodo che si rompe non deve fermare l'altro
        risposte.put((nome, 'errore: %s' % e, None, None))


def risolvi(scuola, tempo=300, a_ogni_passo=None):
    """Lancia i metodi installati sulla scuola, ognuno con `tempo` secondi, e ritorna lo stato dei metodi (vedi in
    cima). `a_ogni_passo`, se c'e', e' chiamata con lo stato a ogni novita' (un metodo che risponde): serve a chi
    vuole mostrare la prima risposta senza aspettare l'altro metodo."""
    partenza = time.time()
    stato = {'esito': 'in corso', 'secondi': 0, 'metodi': {}}
    risposte, processi = multiprocessing.Queue(), {}
    # 1. un processo per ogni metodo installato
    for nome, _, modulo, pacchetto in METODI:
        if importlib.util.find_spec(pacchetto) is None:
            stato['metodi'][nome] = {'esito': 'non installato', 'secondi': 0}
            continue
        stato['metodi'][nome] = {'esito': 'in corso', 'secondi': 0}
        processi[nome] = multiprocessing.Process(target=_lavora, args=(modulo, scuola, tempo, risposte, nome),
                                                 daemon=True)
        processi[nome].start()

    # se chi ci ha lanciato ci ferma (il sito, con «Ferma»), si fermano anche i processi dei metodi
    def fermati(*_):
        for p in processi.values():
            p.terminate()
        raise SystemExit(1)
    try:
        vecchio = signal.signal(signal.SIGTERM, fermati)
    except ValueError:   # (non siamo nel filo principale: niente da fare)
        vecchio = None

    # 2. le risposte, man mano che arrivano
    attesi = set(processi)
    while attesi:
        try:
            nome, esito, orario, aule = risposte.get(timeout=1)
        except queue.Empty:
            # un processo morto senza rispondere (ucciso dal sistema, per esempio): non lo si aspetta piu'
            for morto in [n for n in attesi if not processi[n].is_alive()]:
                attesi.discard(morto)
                stato['metodi'][morto] = {'esito': 'errore: il processo si e\' chiuso', 'secondi': round(time.time() - partenza, 2)}
            continue
        attesi.discard(nome)
        suo = stato['metodi'][nome] = {'esito': esito, 'secondi': round(time.time() - partenza, 2)}
        if orario:
            # ogni orario trovato si ricontrolla, di chiunque sia
            controllo = verifica.verifica(scuola, orario, aule)
            suo['verificato'] = bool(controllo['ok'])
            if 'orario' not in stato:
                stato.update(orario=orario, aule=aule, verifica=controllo)
        _tira_le_somme(stato, finito=not attesi, adesso=round(time.time() - partenza, 2))
        if a_ogni_passo and attesi:
            a_ogni_passo(stato)
    if not processi:
        _tira_le_somme(stato, finito=True, adesso=0)
    if vecchio is not None:
        signal.signal(signal.SIGTERM, vecchio)
    return stato


def _tira_le_somme(stato, finito, adesso):
    """L'esito del caso, dagli esiti dei metodi arrivati fin qui."""
    esiti = {nome: m['esito'] for nome, m in stato['metodi'].items()}
    trovato = [n for n, e in esiti.items() if e == TROVATO and stato['metodi'][n].get('verificato')]
    impossibile = [n for n, e in esiti.items() if e == IMPOSSIBILE]
    if trovato and impossibile:
        stato['esito'] = CONTRADDIZIONE
    elif TROVATO in esiti.values() or impossibile:
        stato['esito'] = TROVATO if TROVATO in esiti.values() else IMPOSSIBILE
    elif finito:
        stato['esito'] = LIMITE_DI_TEMPO
    # chi ha risposto per primo, e quando (si scrive una volta sola)
    if stato['esito'] != 'in corso' and 'risolto_da' not in stato:
        certi = [n for n in esiti if esiti[n] in (TROVATO, IMPOSSIBILE)]
        if certi:
            primo = min(certi, key=lambda n: stato['metodi'][n]['secondi'])
            stato.update(risolto_da=primo, secondi=stato['metodi'][primo]['secondi'])
        else:
            stato['secondi'] = adesso
