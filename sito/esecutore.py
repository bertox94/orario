"""I test in tempo reale, in thread dello stesso processo del sito.

Un test: i dati (un caso casuale, i dati della scuola o un file) vanno al solver come --input; mentre la ricerca gira,
la sua traccia arriva riga per riga (vedi solver.Esecuzione) e ogni backtrack finisce nella tabella backtrack, cosi' la
pagina del test lo mostra subito. Alla fine l'esito, gli orari trovati e i numeri della ricerca vanno nell'esecuzione.

Se il sito si riavvia mentre un test gira, al riavvio il test risulta "interrotto" (il processo e' perso).
"""
import json
import os
import shutil
import tempfile
import threading
import time
import traceback

from . import db, generatore, modello, solver

def _json(v):
    """Un valore per una colonna jsonb (come testo: lo converte Postgres)."""
    return json.dumps(v, ensure_ascii=False)


# esecuzioni (id) in corso -> (id della prova, solver.Esecuzione, nome del caso)
ATTIVE = {}
_lock = threading.Lock()
_avviato = False


def avvia(app):
    """All'avvio del sito (una volta per processo)."""
    global _avviato
    if _avviato:
        return
    _avviato = True
    riprendi_dopo_riavvio(app.config['SITO'])


def riprendi_dopo_riavvio(config):
    shutil.rmtree(config.cartella_esecuzioni, ignore_errors=True)  # resti di esecuzioni interrotte (in RAM)
    with db.connetti(config.database_url) as c:
        c.execute("update esecuzioni set esito = 'interrotto', finito = false, fine = now() where esito is null")
        c.execute("update prove set stato = 'interrotta', fine = now(), "
                  "messaggio = coalesce(messaggio, 'il sito e'' stato riavviato mentre il test era in corso') "
                  "where stato in ('in corso', 'annullamento')")
        c.commit()


def nome_caso(caso):
    if caso['tipo'] == 'casuale':
        return 'seme %d, %d classi%s' % (caso['seme'], caso.get('classi', 8), ', ritardi' if caso.get('ritardi') else '')
    if caso['tipo'] == 'scuola':
        return 'dati della scuola'
    return caso.get('nome') or 'file'


def _cartella_lavoro(config):
    """Una cartella temporanea (in RAM, vedi SITO_LAVORO) per i file di una sola esecuzione."""
    os.makedirs(config.cartella_esecuzioni, exist_ok=True)
    return tempfile.mkdtemp(prefix='e-', dir=config.cartella_esecuzioni)


def dati_del_caso(c, scuola_id, caso):
    """I dati per --input di un caso: generati da un seme, quelli attuali della scuola, o quelli gia' pronti."""
    if caso['tipo'] == 'casuale':
        return generatore.genera(caso['seme'], caso.get('classi', 8), caso.get('ritardi', False))
    if caso['tipo'] == 'scuola':
        return modello.per_main(modello.leggi(c, scuola_id))
    return caso['dati']


# ---- dati da un file ----

def da_formato_precedente(d):
    """Dati nel formato del solver precedente (classi e docenti per indice, lezioni da solo e in coppia separate)
    convertiti per --input: ogni lezione diventa una chiamata (in coppia: tutte le ore insieme al codocente)."""
    classi = [cl['nome'] for cl in d['classi']]
    docenti = [p['nome'] for p in d['docenti']]
    lezioni = []
    for l in d['lezioni']:
        x = {'classe': classi[l['classe']], 'docente': docenti[l['docente']], 'ore': l['ore']}
        if l.get('codocente') is not None:
            x.update(codocente=docenti[l['codocente']], ore_codocenza=l['ore'])
        lezioni.append(x)
    return {'giorni': d['giorni'], 'ore': d['ore'], 'lezioni': lezioni,
            'ingressi': [{'docente': p['nome'], 'ora': p['ingresso']} for p in d['docenti'] if p.get('ingresso')]}


def valida(config, dati):
    """Fa leggere i dati al solver. Ritorna None se vanno bene, altrimenti il motivo."""
    exe = solver.compila(config.cartella_bin)
    cartella = _cartella_lavoro(config)
    try:
        return solver.valida(exe, dati, cartella)
    finally:
        shutil.rmtree(cartella, ignore_errors=True)


def dati_da_file_caricato(config, f):
    """I dati di un file JSON caricato: nel formato di --input, oppure del solver precedente (anche dentro un
    risultato.json). Ritorna (dati per --input, nome, None) oppure (None, None, errore)."""
    try:
        contenuto = json.load(f)
    except ValueError:
        return None, None, 'Il file non e\' JSON valido.'
    d = contenuto.get('dati', contenuto) if isinstance(contenuto, dict) else None
    if not isinstance(d, dict) or not d.get('lezioni'):
        return None, None, 'Formato del file sconosciuto: serve un file esportato dai dati della scuola.'
    try:
        dati = d if modello.formato_main(d) else da_formato_precedente(d)
    except (KeyError, IndexError, TypeError):
        return None, None, 'Formato del file sconosciuto: serve un file esportato dai dati della scuola.'
    errore = valida(config, dati)
    if errore:
        return None, None, 'File non valido: %s' % errore
    nome = (f.filename or 'file').rsplit('/', 1)[-1][:60]
    return dati, 'file %s' % nome, None


# ---- un test ----

class _Registro:
    """Riceve la traccia del solver mentre arriva e la scrive nel database: i backtrack a gruppi (al massimo ogni
    mezzo secondo), i dati subito (la pagina li usa per i nomi)."""

    def __init__(self, url, eid):
        self.url, self.eid = url, eid
        self.attesa = []
        self.attesa_nodi = []
        self.ultimo = time.time()
        self.lock = threading.Lock()
        self.dati = None
        self.orari = []
        self.backtrack = 0

    def ricevi(self, x):
        tipo = x.get('tipo')
        if tipo == 'dati':
            self.dati = x
            with db.connetti(self.url) as c:
                c.execute('update esecuzioni set risultato = %s where id = %s', (_json({'dati': x}), self.eid))
                c.commit()
        elif tipo == 'backtrack':
            with self.lock:
                self.attesa.append(x)
                self.backtrack = x['n']
            if len(self.attesa) >= 50 or time.time() - self.ultimo > 0.5:
                self.scarica()
        elif tipo == 'nodi':
            with self.lock:
                self.attesa_nodi.append(x)
            if len(self.attesa_nodi) >= 50 or time.time() - self.ultimo > 0.5:
                self.scarica()
        elif tipo == 'orario':
            self.orari.append(x['orario'])

    def scarica(self):
        with self.lock:
            righe, self.attesa = self.attesa, []
            nodi, self.attesa_nodi = self.attesa_nodi, []
            self.ultimo = time.time()
        if not righe and not nodi:
            return
        with db.connetti(self.url) as c:
            for x in nodi:
                c.execute('insert into nodi (esecuzione_id, da, quanti, passi) values (%s, %s, %s, %s) '
                          'on conflict do nothing', (self.eid, x['da'], len(x['passi']), _json(x['passi'])))
            for x in righe:
                c.execute('insert into backtrack (esecuzione_id, n, nodo, pos, giorno, ora, classe, buca_dopo, '
                          'riassunto, record) values (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s) on conflict do nothing',
                          (self.eid, x['n'], x['nodo'], x['pos'], x['giorno'], x['ora'], x['classe'],
                           bool(x['buca_dopo']), _json(riassunto_backtrack(x)), _json(x)))
            c.commit()


def riassunto_backtrack(x):
    """Per un backtrack: a quale controllo si e' fermato ogni docente della classe. Chi passa i controlli del primo
    docente si ferma nelle coppie e poi da solo: conta il controllo del ramo da solo, l'ultimo tentativo."""
    conta = {}
    if not x['docenti'] and not x['slot']['uscita']:
        return {x['slot']['nome']: 1}
    for d in x['docenti']:
        if d['fermato_a'] >= 0:
            nome = d['controlli'][d['fermato_a']]['nome']
        else:
            s = d['da_solo']
            nome = 'da solo: ' + (s['controlli'][s['fermato_a']]['nome'] if s['fermato_a'] >= 0 else '?')
        conta[nome] = conta.get(nome, 0) + 1
    return conta


def _riassunto(r):
    """Una riga per l'elenco dei test: "57 backtrack · 1.234 nodi"."""
    parti = []
    if r.get('errore_dati'):
        return r['errore_dati']
    parti.append('%s backtrack' % '{:,}'.format(r.get('backtrack') or 0).replace(',', '.'))
    if r.get('nodi') is not None:
        parti.append('%s nodi' % '{:,}'.format(r['nodi']).replace(',', '.'))
    return ' · '.join(parti)


def avvia_live(config, scuola_id, caso, opzioni, solutori=('main',)):
    """Crea la prova e un'esecuzione per ogni solver, lancia ognuno nel suo thread (girano in parallelo) e ritorna
    (id prova, [id delle esecuzioni])."""
    with db.connetti(config.database_url) as c:
        dati = dati_del_caso(c, scuola_id, caso)
        nome = nome_caso(caso)
        caso_salvato = {k: v for k, v in caso.items() if k != 'dati'}
        pid = c.execute("insert into prove (scuola_id, tipo, nome, stato, opzioni, commit, inizio, dati_scuola) "
                        "values (%s, 'live', %s, 'in corso', %s, %s, now(), %s) returning id",
                        (scuola_id, nome, _json({'caso': caso_salvato, 'solutori': list(solutori), **opzioni}),
                         solver.commit_attuale(), _json(dati))).fetchone()['id']
        esecuzioni = []
        for s in solutori:
            o = dict(opzioni, solutore=s) if s != 'main' else dict(opzioni)  # 'main' non si scrive (i test di prima)
            eid = c.execute('insert into esecuzioni (prova_id, caso, caso_dati, fase, opzioni) '
                            'values (%s, %s, %s, 1, %s) returning id',
                            (pid, nome, _json(caso_salvato), _json(o))).fetchone()['id']
            esecuzioni.append((eid, o))
        c.commit()
    pronti = []
    for eid, o in esecuzioni:
        pronto = threading.Event()
        threading.Thread(target=_esegui, args=(config, pid, eid, nome, dati, o, pronto), daemon=True,
                         name='live-%d-%d' % (pid, eid)).start()
        pronti.append(pronto)
    for pronto in pronti:
        pronto.wait(120)  # la compilazione puo' richiedere qualche secondo
    return pid, [eid for eid, _ in esecuzioni]


# la prova e' finita quando non resta nessuna sua esecuzione senza esito
_FINE_PROVA = ("update prove set stato = %s, fine = now(), riepilogo = %s, messaggio = coalesce(%s, messaggio) "
               "where id = %s and not exists (select 1 from esecuzioni where prova_id = %s and esito is null)")


def _esegui(config, pid, eid, nome, dati, opzioni, pronto):
    """Un'esecuzione (un solver) di una prova, dall'inizio alla fine: nel suo thread."""
    cartella = _cartella_lavoro(config)
    registro = _Registro(config.database_url, eid)
    try:
        exe = solver.compila(config.cartella_bin, opzioni.get('solutore', 'main'))
        e = solver.Esecuzione(exe, dati, opzioni, cartella, su_record=registro.ricevi)
        with _lock:
            ATTIVE[eid] = (pid, e, nome)
        pronto.set()
        try:
            r = e.attendi()
        finally:
            with _lock:
                ATTIVE.pop(eid, None)
        registro.scarica()
        # da un solver fermato: i backtrack sono quelli contati fin li' (il progresso li conta anche oltre il
        # limite di quelli registrati)
        if r['fine'] is None:
            r['backtrack'] = max(r['backtrack'] or 0, (e.ultimo_progresso or {}).get('backtrack') or 0)
            r['nodi'] = (e.ultimo_progresso or {}).get('nodi')
        risultato = {'dati': r['dati'], 'orari': r['orari'], 'fine': r['fine'], 'nodi': r['nodi'],
                     'backtrack': r['backtrack'], 'errore_dati': r['errore_dati']}
        with db.connetti(config.database_url) as c:
            c.execute('update esecuzioni set esito = %s, finito = %s, marce = %s, trovati = %s, secondi = %s, '
                      'diagnosi = %s, risultato = %s, fine = now() where id = %s',
                      (r['esito'], r['finito'], r['backtrack'], r['trovati'], r['secondi'], _riassunto(r),
                       _json(risultato), eid))
            c.execute(_FINE_PROVA, ('finita', _json({'esito': r['esito']}), None, pid, pid))
            c.commit()
    except Exception as ex:  # l'esecuzione risulta in errore, con il motivo
        traceback.print_exc()
        with db.connetti(config.database_url) as c:
            c.execute("update esecuzioni set esito = 'errore', finito = false, fine = now(), diagnosi = %s "
                      "where id = %s and esito is null", (str(ex), eid))
            c.execute(_FINE_PROVA, ('errore', _json({'esito': 'errore'}), str(ex), pid, pid))
            c.commit()
    finally:
        pronto.set()
        shutil.rmtree(cartella, ignore_errors=True)


def in_corso(esecuzione_id):
    with _lock:
        return esecuzione_id in ATTIVE


def progresso(esecuzione_id):
    """L'ultima riga di progresso del solver (nodi, backtrack, orari, pos, secondi), o None."""
    with _lock:
        a = ATTIVE.get(esecuzione_id)
    return a[1].ultimo_progresso if a else None


def ferma_esecuzione(esecuzione_id):
    with _lock:
        a = ATTIVE.get(esecuzione_id)
    if a:
        a[1].ferma()
    return a is not None


def elimina_prova(config, prova_id):
    """Cancella la prova (con esecuzioni e backtrack), se non e' in corso."""
    with db.connetti(config.database_url) as c:
        p = c.execute('select stato from prove where id = %s', (prova_id,)).fetchone()
        if not p or p['stato'] in ('in corso', 'annullamento'):
            return False
        c.execute('delete from prove where id = %s', (prova_id,))
        c.commit()
    return True
