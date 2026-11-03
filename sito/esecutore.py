"""Le prove in esecuzione: test in tempo reale e batterie notturne, in thread dello stesso processo del sito.

- Test in tempo reale: un caso, avviato subito; il progresso del solver (--progresso) resta in memoria per la pagina.
- Test notturni: una prova alla volta, quando arriva la sua ora; i casi girano in parallelo (SITO_PARALLELI) in tre
  fasi come tests/casuali.py: fase 1 breve, fase 2 lunga per i non finiti, fase 3 registrando la ricerca passo passo
  (--passi, compressa, nel database) per quelli ancora non finiti.
- Pianificazioni: ogni giorno indicato, all'ora indicata, creano una prova notturna.

Se il sito si riavvia mentre una prova gira, al riavvio la prova risulta "interrotta" (i processi sono persi).
"""
import concurrent.futures
import datetime
import json
import os
import re
import shutil
import tempfile
import threading
import time
import traceback

import psycopg

from . import archivio, db, modello, solver

# esecuzioni (id) in corso -> (id della prova, solver.Esecuzione): per leggere il progresso e per fermarle
ATTIVE = {}
PASSI_NOTTURNI = 100000  # nodi registrati nella fase 3 dei test notturni, se la prova non dice altro
_lock = threading.Lock()
_avviato = False


def avvia(app):
    """Avvia i thread di servizio (una volta per processo)."""
    global _avviato
    if _avviato:
        return
    _avviato = True
    config = app.config['SITO']
    riprendi_dopo_riavvio(config)
    threading.Thread(target=_ciclo_pianificazioni, args=(config,), daemon=True, name='pianificazioni').start()
    threading.Thread(target=_ciclo_notturni, args=(config,), daemon=True, name='notturni').start()


def riprendi_dopo_riavvio(config):
    shutil.rmtree(config.cartella_esecuzioni, ignore_errors=True)  # resti di esecuzioni interrotte (in RAM)
    with db.connetti(config.database_url) as c:
        c.execute("update esecuzioni set esito = 'interrotta', finito = false, fine = now() where esito is null")
        c.execute("update prove set stato = 'interrotta', fine = now(), "
                  "messaggio = coalesce(messaggio, 'il sito e'' stato riavviato mentre la prova era in corso') "
                  "where stato in ('in corso', 'annullamento')")
        c.commit()



def nome_caso(caso):
    if caso['tipo'] == 'casuale':
        return 'seme %d%s' % (caso['seme'], ' ritardi' if caso.get('ritardi') else '')
    return caso.get('nome', 'dati della scuola')


# ---- un'esecuzione ----

def _nuova_esecuzione(c, prova_id, caso, fase, opzioni):
    return c.execute('insert into esecuzioni (prova_id, caso, caso_dati, fase, opzioni) '
                     'values (%s, %s, %s, %s, %s) returning id',
                     (prova_id, nome_caso(caso), json.dumps(caso), fase, json.dumps(opzioni))
                     ).fetchone()['id']


def _salva_risultato(c, esecuzione_id, r):
    c.execute('update esecuzioni set esito = %s, finito = %s, marce = %s, trovati = %s, secondi = %s, diagnosi = %s, '
              'diagnosi_testo = %s, risultato = %s, fine = now() where id = %s',
              (r['esito'], r['finito'], r['marce'], r['trovati'], r['secondi'], r['diagnosi'], r['diagnosi_testo'],
               json.dumps(r['json']) if r.get('json') else None, esecuzione_id))


def esegui_caso(config, exe, prova_id, caso, fase, opzioni):
    """Esegue un caso registrandolo nel database (prima "in corso", poi con il risultato). Ritorna il risultato."""
    with db.connetti(config.database_url) as c:
        eid = _nuova_esecuzione(c, prova_id, caso, fase, opzioni)
        c.commit()
    try:
        r = _esegui(config, exe, prova_id, eid, caso, opzioni)
    except Exception as ex:
        with db.connetti(config.database_url) as c:
            c.execute("update esecuzioni set esito = 'errore', finito = false, fine = now(), diagnosi = %s "
                      "where id = %s", (str(ex), eid))
            c.commit()
        raise
    with db.connetti(config.database_url) as c:
        _salva_risultato(c, eid, r)
        c.commit()
    r['esecuzione_id'] = eid
    return r


def _cartella_lavoro(config):
    """Una cartella temporanea (in RAM, vedi SITO_LAVORO) per i file di una sola esecuzione."""
    os.makedirs(config.cartella_esecuzioni, exist_ok=True)
    return tempfile.mkdtemp(prefix='e-', dir=config.cartella_esecuzioni)


def _prepara_caso(config, caso, cartella):
    """Un caso con i dati nel database (da_prova) diventa un caso con il file per --input, scritto nella cartella."""
    if caso.get('tipo') != 'input' or not caso.get('da_prova'):
        return caso
    with db.connetti(config.database_url) as c:
        r = c.execute('select dati_scuola from prove where id = %s', (caso['da_prova'],)).fetchone()
    if not r or not r['dati_scuola']:
        raise RuntimeError('I dati del caso non ci sono piu\' (prova %s eliminata?)' % caso['da_prova'])
    file = os.path.join(cartella, 'input.json')
    with open(file, 'w') as f:
        json.dump(r['dati_scuola'], f, ensure_ascii=False)
    return {**caso, 'file': file}


def _esegui(config, exe, prova_id, eid, caso, opzioni):
    """Lancia il solver (con --progresso) in una cartella in RAM, lo tiene in ATTIVE finche' gira e al termine mette i
    file nel database (dati, orari, uscita; la ricerca passo passo compressa arriva direttamente, a pezzi)."""
    cartella = _cartella_lavoro(config)
    try:
        caso_file = _prepara_caso(config, caso, cartella)
        scrittore = archivio.Scrittore(config.database_url, eid, 'orario.txt.gz') if opzioni.get('passi') else None
        e = solver.Esecuzione(exe, solver.argomenti(caso_file, opzioni, progresso=True), cartella,
                              timeout=opzioni.get('tempo'), su_gz=scrittore.scrivi if scrittore else None)
        with _lock:
            ATTIVE[eid] = (prova_id, e)
        try:
            r = e.attendi()
        finally:
            with _lock:
                ATTIVE.pop(eid, None)
        if scrittore and not e.errore_gz:
            scrittore.chiudi()
        for nome in ('dati.txt', 'orario.txt'):
            archivio.salva_file(config.database_url, eid, nome, os.path.join(cartella, nome))
        archivio.salva(config.database_url, eid, 'uscita.txt', r['uscita'].encode())
        return r
    finally:
        shutil.rmtree(cartella, ignore_errors=True)


def in_corso(esecuzione_id):
    with _lock:
        return esecuzione_id in ATTIVE


def progresso(esecuzione_id):
    """L'ultima riga di progresso del solver (None se non e' in corso o non ne ha ancora scritte)."""
    with _lock:
        a = ATTIVE.get(esecuzione_id)
    return a[1].ultimo_progresso if a else None


def ferma_esecuzione(esecuzione_id):
    with _lock:
        a = ATTIVE.get(esecuzione_id)
    if a:
        a[1].ferma()
    return a is not None


def _caso_con_dati(c, prova_id, dati, nome):
    """I dati di un caso (formato --input) si conservano nella prova, nel database; il caso li richiama."""
    c.execute('update prove set dati_scuola = %s where id = %s', (json.dumps(dati, ensure_ascii=False), prova_id))
    return {'tipo': 'input', 'nome': nome, 'da_prova': prova_id}


def _caso_della_scuola(c, scuola_id, prova_id):
    """Fotografia dei dati della scuola: la prova usa i dati di quel momento."""
    return _caso_con_dati(c, prova_id, modello.per_solver(modello.leggi(c, scuola_id)), 'dati della scuola')


# ---- test in tempo reale ----

def prepara_vista(config, exe, caso, prova_id):
    """Salva nella prova i dati del caso (classi, giorni, ore, docenti), presi da una --solo-verifica lampo: servono
    alla pagina della prova per disegnare la mappa dei blocchi mentre la ricerca gira."""
    cartella = _cartella_lavoro(config)
    try:
        r = solver.esegui(exe, _prepara_caso(config, caso, cartella),
                          {'solo_verifica': True, 'motivi': False, 'tempo': 60}, os.path.join(cartella, 'vista'))
    finally:
        shutil.rmtree(cartella, ignore_errors=True)
    if r.get('json'):
        with db.connetti(config.database_url) as c:
            c.execute('update prove set vista = %s where id = %s',
                      (json.dumps(r['json']['dati'], ensure_ascii=False), prova_id))
            c.commit()


def vista(c, prova_id):
    r = c.execute('select vista from prove where id = %s', (prova_id,)).fetchone()
    return r['vista'] if r else None


def avvia_live(config, scuola_id, caso_richiesto, opzioni, dati_input=None, nome=None):
    """Crea la prova, la avvia in un thread e ritorna l'id dell'esecuzione (per la pagina). Con dati_input (formato
    --input) il caso usa quei dati; con caso_richiesto 'scuola', i dati attuali della scuola."""
    with db.connetti(config.database_url) as c:
        pid = c.execute("insert into prove (scuola_id, tipo, stato, opzioni, commit, inizio) "
                        "values (%s, 'live', 'in corso', %s, %s, now()) returning id",
                        (scuola_id, json.dumps({'caso': caso_richiesto, **opzioni}), solver.commit_attuale())
                        ).fetchone()['id']
        if caso_richiesto['tipo'] == 'scuola':
            caso = _caso_della_scuola(c, scuola_id, pid)
        elif dati_input is not None:
            caso = _caso_con_dati(c, pid, dati_input, nome or 'caso riletto')
        else:
            caso = caso_richiesto
        c.execute('update prove set nome = %s where id = %s', (nome_caso(caso), pid))
        c.commit()
    pronto = threading.Event()
    stato = {}

    def lavoro():
        try:
            exe = solver.compila(cartella_bin=config.cartella_bin)
            prepara_vista(config, exe, caso, pid)
            with db.connetti(config.database_url) as c:
                stato['eid'] = _nuova_esecuzione(c, pid, caso, 1, opzioni)
                c.commit()
            # la pagina puo' aprirsi appena il processo e' partito (ATTIVE viene riempito da _esegui)
            threading.Timer(0.3, pronto.set).start()
            r = _esegui(config, exe, pid, stato['eid'], caso, opzioni)
            with db.connetti(config.database_url) as c:
                _salva_risultato(c, stato['eid'], r)
                c.execute("update prove set stato = 'finita', fine = now(), riepilogo = %s where id = %s",
                          (json.dumps({'esito': r['esito']}), pid))
                c.commit()
        except Exception as ex:  # la prova risulta in errore, con il motivo
            traceback.print_exc()
            with db.connetti(config.database_url) as c:
                if 'eid' in stato:
                    c.execute("update esecuzioni set esito = 'errore', finito = false, fine = now(), diagnosi = %s "
                              "where id = %s and esito is null", (str(ex), stato['eid']))
                c.execute("update prove set stato = 'errore', fine = now(), messaggio = %s where id = %s", (str(ex), pid))
                c.commit()
        finally:
            pronto.set()

    threading.Thread(target=lavoro, daemon=True, name='live-%d' % pid).start()
    pronto.wait(120)  # la compilazione puo' richiedere qualche secondo
    return pid, stato.get('eid')


# ---- test notturni ----

def crea_notturna(c, scuola_id, opzioni, programmata_per=None, nome=None):
    return c.execute("insert into prove (scuola_id, tipo, nome, stato, opzioni, programmata_per) "
                     "values (%s, 'notturna', %s, 'in coda', %s, %s) returning id",
                     (scuola_id, nome, json.dumps(opzioni), programmata_per)).fetchone()['id']


def _ciclo_pianificazioni(config):
    while True:
        try:
            adesso = datetime.datetime.now().astimezone()
            with db.connetti(config.database_url) as c:
                for p in c.execute('select * from pianificazioni where attiva').fetchall():
                    oggi = adesso.date()
                    if adesso.isoweekday() in p['giorni'] and adesso.time() >= p['ora'] and p['ultima_data'] != oggi:
                        crea_notturna(c, p['scuola_id'], p['opzioni'], adesso, nome=p['nome'])
                        c.execute('update pianificazioni set ultima_data = %s where id = %s', (oggi, p['id']))
                c.commit()
        except psycopg.OperationalError as e:
            print('Database non raggiungibile, riprovo tra poco:', (str(e).strip().splitlines() or [''])[0], flush=True)
        except Exception:
            traceback.print_exc()
        time.sleep(20)


def _ciclo_notturni(config):
    while True:
        try:
            with db.connetti(config.database_url) as c:
                p = c.execute("select * from prove where tipo = 'notturna' and stato = 'in coda' "
                              "and (programmata_per is null or programmata_per <= now()) "
                              "order by programmata_per nulls first, id limit 1").fetchone()
                if p:
                    c.execute("update prove set stato = 'in corso', inizio = now(), commit = %s where id = %s",
                              (solver.commit_attuale(), p['id']))
                c.commit()
            if p:
                _esegui_notturna(config, p)
        except psycopg.OperationalError as e:
            print('Database non raggiungibile, riprovo tra poco:', (str(e).strip().splitlines() or [''])[0], flush=True)
        except Exception:
            traceback.print_exc()
        time.sleep(10)


def _annullata(config, prova_id):
    with db.connetti(config.database_url) as c:
        return c.execute('select stato from prove where id = %s', (prova_id,)).fetchone()['stato'] == 'annullamento'


def casi_della_notturna(o):
    casi = []
    varianti = o.get('varianti', ['normale', 'ritardi'])
    for seme in range(o.get('semi_da', 1), o.get('semi_da', 1) + o.get('quanti', 0)):
        for v in ('normale', 'ritardi'):
            if v in varianti:
                casi.append({'tipo': 'casuale', 'seme': seme, 'classi': o.get('classi', 8), 'ritardi': v == 'ritardi'})
    return casi


def _esegui_notturna(config, prova):
    pid, o = prova['id'], prova['opzioni']
    try:
        exe = solver.compila(cartella_bin=config.cartella_bin)
        casi = casi_della_notturna(o)
        if o.get('scuola'):
            with db.connetti(config.database_url) as c:
                casi.insert(0, _caso_della_scuola(c, prova['scuola_id'], pid))
                c.commit()
        base = {'motivi': True, 'ordine_classi': o.get('ordine_classi'), 'ore_obbligatorie': o.get('ore_obbligatorie'),
                'ore_prof_classe': o.get('ore_prof_classe'), 'classi_giorno': o.get('classi_giorno')}
        ultimi = {}  # nome caso -> ultimo risultato

        def fase(numero, da_fare, opzioni):
            if not da_fare or _annullata(config, pid):
                return
            def lancia(k):
                if _annullata(config, pid):  # annullata mentre il caso aspettava il suo turno: non parte
                    return None
                return esegui_caso(config, exe, pid, k, numero, opzioni)

            with concurrent.futures.ThreadPoolExecutor(config.paralleli) as pool:
                futuri = {pool.submit(lancia, k): k for k in da_fare}
                in_attesa = set(futuri)
                while in_attesa:
                    # ogni 2 secondi si controlla anche se la prova e' stata annullata (dalla pagina, o da un altro
                    # processo): allora si fermano i casi in corso, in modo ordinato
                    finiti, in_attesa = concurrent.futures.wait(in_attesa, timeout=2)
                    for f in finiti:
                        r = f.result()
                        if r:
                            ultimi[nome_caso(futuri[f])] = r
                    if in_attesa and _annullata(config, pid):
                        _ferma_prova(pid)

        def non_finiti(tra):
            return [k for k in tra if nome_caso(k) in ultimi and not ultimi[nome_caso(k)]['finito']]

        fase(1, casi, {**base, 'tempo': o.get('fase1', 60)})
        da_rifare = non_finiti(casi)
        if o.get('fase2', 600):
            fase(2, da_rifare, {**base, 'tempo': o.get('fase2', 600)})
            da_rifare = non_finiti(da_rifare)
        if o.get('fase3', 600) and da_rifare and not _annullata(config, pid):
            # la ricerca passo passo (nel database) per rivedere dove si bloccano
            fase(3, da_rifare, {**base, 'tempo': o.get('fase3', 600), 'passi': o.get('passi', PASSI_NOTTURNI)})
        riepilogo = {}
        for r in ultimi.values():
            riepilogo[r['esito']] = riepilogo.get(r['esito'], 0) + 1
        stato = 'annullata' if _annullata(config, pid) else 'finita'
        with db.connetti(config.database_url) as c:
            c.execute('update prove set stato = %s, fine = now(), riepilogo = %s where id = %s',
                      (stato, json.dumps({'esiti': riepilogo, 'casi': len(casi)}), pid))
            c.commit()
    except Exception as ex:
        traceback.print_exc()
        with db.connetti(config.database_url) as c:
            c.execute("update prove set stato = 'errore', fine = now(), messaggio = %s where id = %s", (str(ex), pid))
            c.commit()


def _ferma_prova(prova_id):
    """Ferma le esecuzioni in corso di una prova."""
    with _lock:
        attive = [e for pid, e in ATTIVE.values() if pid == prova_id]
    for e in attive:
        e.ferma()


def annulla_notturna(config, prova_id):
    with db.connetti(config.database_url) as c:
        stato = c.execute('select stato from prove where id = %s', (prova_id,)).fetchone()['stato']
        if stato == 'in coda':
            c.execute("update prove set stato = 'annullata', fine = now() where id = %s", (prova_id,))
        elif stato == 'in corso':
            c.execute("update prove set stato = 'annullamento' where id = %s", (prova_id,))
        c.commit()
    if stato == 'in corso':
        _ferma_prova(prova_id)


def elimina_prova(config, prova_id):
    """Cancella la prova e i suoi file, che stanno nel database (non se e' in corso)."""
    with db.connetti(config.database_url) as c:
        p = c.execute('select stato from prove where id = %s', (prova_id,)).fetchone()
        if not p or p['stato'] in ('in corso', 'annullamento'):
            return False
        c.execute('delete from prove where id = %s', (prova_id,))
        c.commit()
    return True
