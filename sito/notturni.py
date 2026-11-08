"""I test notturni: ogni notte gli stessi test, e i loro numeri confrontati con quelli accettati.

Un test notturno e' un caso (dati salvati nel database) con un solver e le opzioni della ricerca. La ricerca si ferma
sempre allo stesso nodo (--ferma-nodi) o prima, se trova l'orario o prova tutto: quindi i numeri (esito, nodi,
backtrack, posizione piu' avanti, primo orario trovato) sono sempre gli stessi finche' la ricerca non cambia. Il tempo
si mostra ma non si confronta (dipende dal computer e dal carico).

Alla prima esecuzione i numeri diventano il riferimento. Dopo, ogni esecuzione e' «uguale» o «diverso» (con le
differenze campo per campo): un cambiamento voluto della ricerca si accetta dalla pagina, e i numeri nuovi diventano il
riferimento.

Cambiare il limite di nodi (richiesta di Alb dell'8 novembre 2026): con un limite nuovo i numeri non sono confrontabili
con la storia. Dopo il cambio ogni test gira due volte, con il limite nuovo e con quello di prima (ferma_nodi_prima del
test): confronti, peggiorati, migliorati e la pagina usano il limite di prima finche' le esecuzioni degli ultimi 5 giri
del test non hanno tutti e due i limiti; poi il test passa al limite nuovo e gira una volta sola.

Un giro esegue tutti i test attivi di una scuola uno dopo l'altro (non in parallelo: i tempi restano confrontabili e il
Raspberry non si satura). Parte da solo ogni notte all'ora impostata (thread di sorveglianza, uno per processo del sito)
o a mano. Un solo giro alla volta.
"""
import concurrent.futures
import datetime
import hashlib
import json
import re
import os
import shutil
import tempfile
import threading
import time
import traceback

from . import db, generatore, solver

CAMPI = ('esito', 'nodi', 'backtrack', 'pos_max', 'orario', 'errore')  # quelli che devono coincidere
NOMI_CAMPI = {'esito': 'risultato', 'nodi': 'nodi', 'backtrack': 'backtrack', 'pos_max': 'posizione massima',
              'orario': 'orario trovato', 'errore': 'errore nei dati'}
FERMA_NODI = 50000000  # default, come nel test in tempo reale: su un Mac fino a circa 40 s per test con main
CASI_DEL_REPOSITORY = os.path.join(solver.RADICE, 'tests', 'casi')

_lock = threading.Lock()
_giro = {'id': None, 'esecuzione': None, 'fermare': False, 'test': None}  # il giro in corso
_sorveglianza_avviata = False


def _json(v):
    return json.dumps(v, ensure_ascii=False)


def impronta_orario(orario):
    """L'impronta del primo orario trovato (la stessa di tests/non_regressione.py)."""
    return hashlib.md5(json.dumps(orario).encode()).hexdigest()[:12] if orario is not None else None


# ---- creare i test ----

def intervallo(testo, minimo, massimo):
    """Semi, classi e ordini delle classi, tutti allo stesso modo: numeri o intervalli col trattino, separati da virgole
    o spazi. "1-6, 9" -> [1, 2, 3, 4, 5, 6, 9]; anche "1 - 6" e il trattino lungo (ValueError se non si legge)."""
    valori = []
    testo = re.sub(r'\s*[-–—]\s*', '-', testo.strip())
    for parte in re.split(r'[,;\s]+', testo):
        if not parte:
            continue
        a, trattino, b = parte.partition('-')
        a, b = int(a), int(b if trattino else a)  # «8-» non si legge
        if not minimo <= a <= b <= massimo:
            raise ValueError(parte)
        valori.extend(range(a, b + 1))
    if not valori:
        raise ValueError('vuoto')
    return sorted(set(valori))


def fino_a(testo, inizio, minimo, massimo):
    """Il modulo dei test notturni: un numero solo e' l'ultimo valore, e si parte da inizio (semi e ordini da 0,
    classi da 8): «5» -> [0..5]; se e' piu' piccolo di inizio, solo lui. Vuoto: solo inizio. Un intervallo col trattino
    o piu' numeri separati da virgole si leggono come prima (intervallo)."""
    testo = testo.strip()
    if not testo:
        return [inizio]
    if re.search(r'[-–—,;\s]', testo):
        return intervallo(testo, minimo, massimo)
    n = int(testo)
    if not minimo <= n <= massimo:
        raise ValueError(testo)
    return list(range(min(inizio, n), n + 1))


def nome_casuale(seme, classi, ritardi, ordine=0):
    nome = 'seme %d, %d classi%s' % (seme, classi, ', ritardi' if ritardi else '')
    return nome + (', ordine %d' % ordine if ordine else '')


def presente(c, scuola_id, solutore, dati, opzioni):
    """L'id del test notturno attivo con gli stessi dati, solver e opzioni, o None."""
    r = c.execute('select t.id from test_notturni t join dati_notturni d on d.id = t.dati_id '
                  'where d.scuola_id = %s and d.impronta = md5(%s::jsonb::text) and t.scuola_id = %s '
                  'and t.solutore = %s and t.opzioni = %s::jsonb and t.attivo',
                  (scuola_id, _json(dati), scuola_id, solutore, _json(opzioni))).fetchone()
    return r['id'] if r else None


def nei_notturni(c, scuola_id, eid):
    """Il test in tempo reale eid e' gia' tra i test notturni (stessi dati, solver e opzioni, con il suo limite di
    nodi)? L'id del test notturno, False se non c'e', None se il test non ha i dati. Il confronto dei dati si fa nel
    database: i dati di un test non arrivano a Python."""
    r = c.execute("""
        select p.dati_scuola is not null as con_dati,
               (select t.id from test_notturni t join dati_notturni d on d.id = t.dati_id
                where d.scuola_id = p.scuola_id and d.impronta = md5(p.dati_scuola::text)
                  and t.scuola_id = p.scuola_id and t.attivo
                  and t.solutore = coalesce(e.opzioni->>'solutore', 'main')
                  and t.opzioni = jsonb_build_object('orari', coalesce((e.opzioni->>'orari')::int, 1))
                limit 1) as tid
        from esecuzioni e join prove p on p.id = e.prova_id where e.id = %s and p.scuola_id = %s""",
                  (eid, scuola_id)).fetchone()
    if not r or not r['con_dati']:
        return None
    return r['tid'] or False


def da_esecuzione(c, scuola_id, eid):
    """Il test notturno che si fa da un test in tempo reale (stessi dati e stesso solver; il limite di nodi del test, o
    quello di default), come argomenti di aggiungi, piu' 'presente': l'id se c'e' gia'. None se non ci sono i dati."""
    e = c.execute('select e.caso, e.opzioni, p.dati_scuola, p.opzioni as opzioni_prova from esecuzioni e '
                  'join prove p on p.id = e.prova_id where e.id = %s and p.scuola_id = %s', (eid, scuola_id)).fetchone()
    if not e or not e['dati_scuola']:
        return None
    o = e['opzioni'] or {}
    t = {'nome': e['caso'], 'solutore': o.get('solutore', 'main'),
         'caso': dict((e['opzioni_prova'] or {}).get('caso') or {}, da_test=eid), 'dati': e['dati_scuola'],
         'opzioni': {'orari': o.get('orari', 1)}}  # il limite di nodi e' quello della pagina (ferma_nodi_impostato)
    t['presente'] = presente(c, scuola_id, t['solutore'], t['dati'], t['opzioni'])
    return t


def aggiungi(c, scuola_id, nome, solutore, caso, dati, opzioni):
    """Aggiunge un test (se non c'e' gia' lo stesso: stessi dati, solver e opzioni). Ritorna (id, True se e' nuovo):
    l'id e' quello del test che c'era gia', se c'era."""
    gia = presente(c, scuola_id, solutore, dati, opzioni)
    if gia:
        return gia, False
    dati_id = salva_dati(c, scuola_id, [dati])[0]
    r = c.execute('insert into test_notturni (scuola_id, nome, solutore, caso, dati_id, opzioni) '
                  'values (%s, %s, %s, %s, %s, %s) returning id',
                  (scuola_id, nome, solutore, _json(caso), dati_id, _json(opzioni))).fetchone()
    return r['id'], True


def salva_dati(c, scuola_id, tanti):
    """I dati di alcuni casi in dati_notturni (una volta sola: se ci sono gia' si usano quelli). Ritorna i loro id,
    nello stesso ordine."""
    righe = c.execute(_SALVA_DATI, (_json([{'i': i, 'dati': d} for i, d in enumerate(tanti)]), scuola_id,
                                    scuola_id)).fetchall()
    ids = {r['i']: r['id'] for r in righe}
    return [ids[i] for i in range(len(tanti))]


_SALVA_DATI = '''
    with r as (select x.i, x.dati, md5(x.dati::text) as impronta from jsonb_to_recordset(%s::jsonb) as x(i int, dati jsonb)),
    nuovi as (insert into dati_notturni (scuola_id, impronta, dati)
              select distinct on (impronta) %s, impronta, dati from r order by impronta
              on conflict do nothing returning id, impronta)
    select r.i, coalesce(n.id, d.id) as id from r left join nuovi n on n.impronta = r.impronta
    left join dati_notturni d on d.scuola_id = %s and d.impronta = r.impronta'''


def pulisci_dati(c, scuola_id):
    """Toglie i dati che nessun test usa piu' (dopo aver eliminato dei test)."""
    c.execute('delete from dati_notturni d where d.scuola_id = %s and not exists '
              '(select 1 from test_notturni t where t.dati_id = d.id)', (scuola_id,))


def genera(c, scuola_id, semi, classi, ritardi, solutori, orari=1, casi_repository=False, ordini=(0,), possibili=None):
    """I test di tutte le combinazioni (seme x classi x ritardi x ordine delle classi) per ogni solver, piu' (se
    chiesto) i casi di tests/casi. Ritorna (aggiunti, gia' presenti, [messaggi per i casi che il generatore non sa
    fare], [id di tutti i test chiesti, nuovi e gia' presenti])."""
    opzioni = {'orari': orari}  # il limite di nodi e' quello della pagina, letto a ogni esecuzione
    casi, problemi = [], []
    for seme in semi:
        for n in classi:
            for r in ritardi:
                try:
                    base = generatore.genera(seme, n, r)
                except ValueError as ex:
                    problemi.append('%s: %s' % (nome_casuale(seme, n, r), ex))
                    continue
                for k in ordini:
                    caso = {'tipo': 'casuale', 'seme': seme, 'classi': n, 'ritardi': r}
                    if k:
                        caso['ordine'] = k
                    casi.append((nome_casuale(seme, n, r, k), caso, generatore.riordina_classi(base, k)))
    if casi_repository and os.path.isdir(CASI_DEL_REPOSITORY):
        for f in sorted(os.listdir(CASI_DEL_REPOSITORY)):
            if f.endswith('.json'):
                with open(os.path.join(CASI_DEL_REPOSITORY, f)) as fh:
                    casi.append(('tests/casi/' + f[:-5], {'tipo': 'file', 'nome': 'tests/casi/' + f}, json.load(fh)))
    scartati = 0
    if possibili:  # solo i casi possibili: possibili(lista di dati) -> [True/False] (vedi dati_possibili)
        ok = possibili([d for _, _, d in casi])
        scartati = ok.count(False)
        casi = [x for x, si in zip(casi, ok) if si]
    aggiunti = gia = 0
    ids = []
    # i dati una volta per caso, poi i test (uno per solver) con l'id dei dati: alla query arrivano solo i nomi
    test, visti = [], set()
    for k in range(0, len(casi), _PER_VOLTA):
        blocco = casi[k:k + _PER_VOLTA]
        for (nome, caso, _), dati_id in zip(blocco, salva_dati(c, scuola_id, [d for _, _, d in blocco])):
            for s in solutori:
                if (s, dati_id) in visti:  # due casi con gli stessi dati: un test solo
                    continue
                visti.add((s, dati_id))
                test.append({'nome': nome, 'solutore': s, 'caso': caso, 'dati_id': dati_id})
    gia += len(casi) * len(solutori) - len(test)
    for k, i in enumerate(range(0, len(test), _PER_VOLTA)):
        if k and k & (k - 1) == 0:
            # dopo 1, 2, 4, 8... blocchi: il database rifa' le statistiche, se no crede che la tabella sia ancora
            # piccola e non usa l'indice per trovare i test che ci sono gia'
            c.execute('analyze test_notturni')
        for r in c.execute(_AGGIUNGI_TANTI, (_json(test[i:i + _PER_VOLTA]), scuola_id, _json(opzioni), scuola_id,
                                             _json(opzioni))).fetchall():
            ids.append(r['id'])
            if r['nuovo']:
                aggiunti += 1
            else:
                gia += 1
    return aggiunti, gia, problemi, ids, scartati


def dati_possibili(config, tanti):
    """Per ogni dati della lista: False se la verifica dei dati del solver (quella che fa prima della ricerca: «errore
    nei dati») dice che l'orario non puo' esistere, se no True. Il solver si ferma al primo nodo; i casi si verificano
    in parallelo, uno per processore. Un caso che passa la verifica puo' comunque non avere soluzione: la verifica
    guarda solo condizioni necessarie."""
    exe = solver.compila(config.cartella_bin, 'main')
    base = tempfile.mkdtemp(prefix='v-', dir=_cartella(config))
    opzioni = {'orari': 1, 'max_backtrack': 0, 'max_nodi': 0, 'ferma_nodi': 1}

    def uno(i_dati):
        i, dati = i_dati
        r = solver.Esecuzione(exe, dati, opzioni, os.path.join(base, str(i))).attendi()
        return not r.get('errore_dati') and r['esito'] != 'errore nei dati'

    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 2) as ex:
            return list(ex.map(uno, enumerate(tanti)))
    finally:
        shutil.rmtree(base, ignore_errors=True)


# Aggiungere tanti test in una volta: una query ogni _PER_VOLTA casi per i dati e una ogni _PER_VOLTA test (invece
# di due per test), con i test che ci sono gia' trovati nel database dall'indice (scuola, solver, dati). Prima ogni
# test era un confronto con i dati di tutti i test della scuola e due andate e ritorno con il database (dal
# Raspberry al Mac: la rete): con migliaia di test, minuti.
_PER_VOLTA = 200
_AGGIUNGI_TANTI = '''
    with r as (select x.* from jsonb_to_recordset(%s::jsonb) as x(nome text, solutore text, caso jsonb, dati_id int)),
    gia as (select r.solutore, r.dati_id, t.id from r cross join lateral (
                select id from test_notturni t where t.scuola_id = %s and t.solutore = r.solutore
                and t.dati_id = r.dati_id and t.opzioni = %s::jsonb and t.attivo order by id limit 1) t),
    nuovi as (insert into test_notturni (scuola_id, nome, solutore, caso, dati_id, opzioni)
              select %s, r.nome, r.solutore, r.caso, r.dati_id, %s::jsonb from r
              where not exists (select 1 from gia where gia.solutore = r.solutore and gia.dati_id = r.dati_id)
              returning id)
    select id, true as nuovo from nuovi union all select id, false from gia'''


# ---- eseguire un test ----

def esegui_test(config, test, ferma_nodi):
    """Il solver sul test, senza registrare backtrack ne' nodi. Ritorna i numeri della ricerca (vedi CAMPI) e i
    secondi. Si ferma prima se il giro viene fermato (esito 'fermato')."""
    cartella = tempfile.mkdtemp(prefix='n-', dir=_cartella(config))
    try:
        exe = solver.compila(config.cartella_bin, test['solutore'])
        opzioni = {'solutore': test['solutore'], 'orari': test['opzioni'].get('orari', 1), 'max_backtrack': 0,
                   'max_nodi': 0, 'ferma_nodi': ferma_nodi}
        e = solver.Esecuzione(exe, test['dati'], opzioni, cartella)
        with _lock:
            _giro['esecuzione'] = e
            fermare = _giro['fermare']
        if fermare:
            e.ferma()
        try:
            r = e.attendi()
        finally:
            with _lock:
                _giro['esecuzione'] = None
        fine = r['fine'] or {}
        return {'esito': r['esito'], 'nodi': r['nodi'], 'backtrack': r['backtrack'] if r['fine'] else None,
                'pos_max': fine.get('pos_max'), 'orario': impronta_orario(r['orari'][0] if r['orari'] else None),
                'errore': r['errore_dati'], 'secondi': r['secondi']}
    finally:
        shutil.rmtree(cartella, ignore_errors=True)


def confronta(riferimento, trovato):
    """Le differenze tra il riferimento accettato e un'esecuzione: [{campo, atteso, trovato}]."""
    return [{'campo': k, 'atteso': riferimento.get(k), 'trovato': trovato.get(k)} for k in CAMPI
            if riferimento.get(k) != trovato.get(k)]


def _cartella(config):
    d = os.path.join(config.lavoro, 'notturni')
    os.makedirs(d, exist_ok=True)
    return d


# ---- un giro ----

def giro_in_corso():
    with _lock:
        return _giro['id']


def test_in_corso():
    """L'id del test che sta girando adesso (o None)."""
    with _lock:
        return _giro['test']


def avvia_giro(config, scuola_id, motivo, soli=None):
    """Crea un giro e lo esegue in un thread. soli: gli id dei test da eseguire (default: tutti gli attivi).
    Ritorna l'id del giro, o None se ce n'e' gia' uno in corso (o non c'e' nessun test)."""
    with _lock:
        if _giro['id'] is not None:
            return None
        _giro.update(id=-1, fermare=False, test=None)  # occupato finche' il giro non e' nel database
    try:
        with db.connetti(config.database_url) as c:
            # i test attivi, di un solver che c'e' ancora (un solver eliminato, come due-livelli, si salta)
            q = 'select id from test_notturni where scuola_id = %s and attivo and solutore = any(%s)'
            ids = [r['id'] for r in c.execute(q + ' order by id', (scuola_id, list(solver.solutori())))]
            if soli is not None:
                ids = [i for i in ids if i in set(soli)]
            if not ids:
                with _lock:
                    _giro['id'] = None
                return None
            gid = c.execute("insert into giri_notturni (scuola_id, stato, motivo, commit) values (%s, 'in corso', %s, %s) "
                            "returning id", (scuola_id, motivo, solver.commit_attuale())).fetchone()['id']
            c.commit()
    except Exception:
        with _lock:
            _giro['id'] = None
        raise
    with _lock:
        _giro['id'] = gid
    threading.Thread(target=_esegui_giro, args=(config, gid, ids), daemon=True, name='notturni-%d' % gid).start()
    return gid


def ferma_giro():
    with _lock:
        if _giro['id'] is None:
            return False
        _giro['fermare'] = True
        e = _giro['esecuzione']
    if e:
        e.ferma()
    return True


def _esegui_giro(config, gid, ids):
    stato = 'finito'
    try:
        for tid in ids:
            with _lock:
                if _giro['fermare']:
                    stato = 'fermato'
                    break
                _giro['test'] = tid
            with db.connetti(config.database_url) as c:
                test = c.execute('select t.*, d.dati from test_notturni t join dati_notturni d on d.id = t.dati_id '
                                 'where t.id = %s', (tid,)).fetchone()
                if not test:
                    continue
                ferma_nodi = ferma_nodi_impostato(c)  # ogni test legge il limite della pagina quando parte
            # dopo un cambio del limite anche con il limite di prima, finche' il test non passa a quello nuovo
            limiti = [ferma_nodi] + [x for x in [test['ferma_nodi_prima']] if x and x != ferma_nodi]
            for limite in limiti:
                with _lock:
                    if _giro['fermare']:
                        break
                with db.connetti(config.database_url) as c:
                    nid = c.execute('insert into esecuzioni_notturne (giro_id, test_id, ferma_nodi) values (%s, %s, %s) '
                                    'returning id', (gid, tid, limite)).fetchone()['id']
                    pulisci_esecuzioni(c, gid, tid)  # anche mentre gira: 5 esecuzioni, compresa quella in corso
                    c.commit()
                try:
                    r = esegui_test(config, test, limite)
                except Exception as ex:  # il test risulta in errore, il giro continua
                    traceback.print_exc()
                    r = {'esito': 'errore', 'errore': str(ex), 'secondi': None}
                _registra(config, nid, test, r, limite, prima=limite != ferma_nodi)
            # subito, test per test: se passa al limite nuovo, peggiorato o migliorato, e di questo test restano le
            # ultime 5 esecuzioni
            try:
                with db.connetti(config.database_url) as c:
                    aggiorna_passaggio(c, tid, ferma_nodi)
                    registra_variazioni(c, gid, tid)
                    pulisci_esecuzioni(c, gid, tid)
                    c.commit()
            except Exception:  # non deve fermare il giro
                traceback.print_exc()
    except Exception:
        traceback.print_exc()
        stato = 'interrotto'
    finally:
        with db.connetti(config.database_url) as c:
            c.execute('update giri_notturni set stato = %s, fine = now() where id = %s', (stato, gid))
            c.commit()
            try:
                pulisci_esecuzioni(c, gid)  # i giri rimasti senza esecuzioni
                c.commit()
            except Exception:  # la pulizia non deve far perdere il giro
                traceback.print_exc()
                c.rollback()
        with _lock:
            _giro.update(id=None, esecuzione=None, fermare=False, test=None)


ESECUZIONI_DA_TENERE = 5  # per ogni test notturno restano le sue ultime 5 esecuzioni (richiesta di Alb)

# Peggiorati e migliorati: un'esecuzione e' migliore di un'altra se arriva piu' lontano (concluso: trovato, nessuna
# soluzione o errore nei dati; poi limite di nodi; poi errore del solver), e a parita' se usa meno nodi. Il tempo non
# conta (dipende dal carico del computer). Le esecuzioni fermate a mano o interrotte non contano.
_RANGO = {'trovato': 2, 'nessuna soluzione': 2, 'errore nei dati': 2, 'limite di nodi': 1, 'errore': 0}


def _valore_esecuzione(n):
    return (_RANGO.get(n['esito'], 0), -(n['nodi'] or 0))


def _numeri(n):
    return {'inizio': n['inizio'].isoformat() if n['inizio'] else None, 'esito': n['esito'], 'nodi': n['nodi'],
            'ferma_nodi': n['ferma_nodi'],
            'backtrack': n['backtrack'], 'secondi': n['secondi'], 'giro_id': n['giro_id']}


def registra_variazioni(c, gid, tid=None):
    """Dopo un test (tid; None: tutti i test del giro gid): se l'ultima esecuzione e' peggiore di una delle precedenti ancora
    tenute, il test e' «peggiorato» (confrontato con la migliore); se no, se e' migliore di una, «migliorato»
    (confrontato con la peggiore). Una riga per test e tipo in variazioni_notturne, aggiornata se c'era gia'."""
    giro = c.execute('select scuola_id, commit from giri_notturni where id = %s', (gid,)).fetchone()
    if not giro:
        return
    # solo le esecuzioni con il limite che la pagina mostra: quello di prima, se il test gira ancora con tutti e due
    righe = c.execute('''
        select n.* from esecuzioni_notturne n join test_notturni t on t.id = n.test_id
        where n.test_id in (select test_id from esecuzioni_notturne where giro_id = %s)
          and (%s::int is null or n.test_id = %s)
          and n.ferma_nodi = coalesce(t.ferma_nodi_prima, %s)
          and n.esito is not null and n.esito not in ('fermato', 'interrotto')
        order by n.test_id, n.id''', (gid, tid, tid, ferma_nodi_impostato(c))).fetchall()
    per_test = {}
    for n in righe:
        per_test.setdefault(n['test_id'], []).append(n)
    for tid, lista in per_test.items():
        ultima = lista[-1]
        prima = lista[:-1]
        if ultima['giro_id'] != gid or not prima:
            continue
        v = _valore_esecuzione(ultima)
        migliore = max(prima, key=_valore_esecuzione)
        peggiore = min(prima, key=_valore_esecuzione)
        if v < _valore_esecuzione(migliore):
            tipo, confronto = 'peggiorato', migliore
        elif v > _valore_esecuzione(peggiore):
            tipo, confronto = 'migliorato', peggiore
        else:
            continue
        c.execute('''insert into variazioni_notturne (scuola_id, test_id, tipo, commit, dopo, prima)
                     values (%s, %s, %s, %s, %s, %s)
                     on conflict (test_id, tipo) do update set quando = now(), commit = excluded.commit,
                         dopo = excluded.dopo, prima = excluded.prima''',
                  (giro['scuola_id'], tid, tipo, giro['commit'], _json(_numeri(ultima)), _json(_numeri(confronto))))


def pulisci_esecuzioni(c, gid, tid=None):
    """Dopo ogni test di un giro (tid) e alla fine del giro (tutti i test della sua scuola): di ogni test restano le
    ultime ESECUZIONI_DA_TENERE esecuzioni per ogni limite di nodi, e i giri rimasti senza esecuzioni si tolgono. Le
    esecuzioni con un limite che non e' ne' quello impostato ne' quello di prima del test si tolgono quando ci sono le
    ultime 5 con il limite impostato. Il riferimento accettato di un test resta (e' nel test)."""
    c.execute('''
        delete from esecuzioni_notturne n using (
            select x.id, row_number() over (partition by x.test_id, x.ferma_nodi order by x.id desc) as k,
                   x.ferma_nodi is distinct from %s and x.ferma_nodi is distinct from t.ferma_nodi_prima
                   and (select count(*) from esecuzioni_notturne y where y.test_id = x.test_id and y.ferma_nodi = %s)
                       >= %s as altro_limite
            from esecuzioni_notturne x join test_notturni t on t.id = x.test_id
            where t.scuola_id = (select scuola_id from giri_notturni where id = %s)
              and (%s::int is null or x.test_id = %s)) v
        where n.id = v.id and (v.k > %s or v.altro_limite)''',
              (ferma_nodi_impostato(c), ferma_nodi_impostato(c), ESECUZIONI_DA_TENERE, gid, tid, tid,
               ESECUZIONI_DA_TENERE))
    c.execute('''
        delete from giri_notturni g where g.scuola_id = (select scuola_id from giri_notturni where id = %s)
          and g.stato <> 'in corso' and not exists (select 1 from esecuzioni_notturne n where n.giro_id = g.id)''',
              (gid,))


def _registra(config, nid, test, r, ferma_nodi, prima=False):
    """Scrive l'esecuzione e il confronto: senza riferimento, o con un riferimento fissato con un altro limite di nodi,
    i numeri diventano il riferimento (confronto 'nuovo'). Con il limite di prima (dopo un cambio del limite) il
    riferimento e' riferimento_prima."""
    colonna = 'riferimento_prima' if prima else 'riferimento'
    rif = test[colonna]
    if r['esito'] in ('fermato', 'errore') or r['esito'] is None:
        confronto, diff = 'errore', None  # un test fermato a mano o in errore non dice niente sulla ricerca
    elif rif is None or rif.get('ferma_nodi') != ferma_nodi:
        confronto, diff = 'nuovo', None
    else:
        diff = confronta(rif, r)
        confronto = 'diverso' if diff else 'uguale'
    with db.connetti(config.database_url) as c:
        c.execute('update esecuzioni_notturne set secondi = %s, esito = %s, nodi = %s, backtrack = %s, pos_max = %s, '
                  'orario = %s, errore = %s, confronto = %s, differenze = %s where id = %s',
                  (r.get('secondi'), r['esito'], r.get('nodi'), r.get('backtrack'), r.get('pos_max'), r.get('orario'),
                   r.get('errore'), confronto, _json(diff) if diff else None, nid))
        if confronto == 'nuovo':
            c.execute("update test_notturni set " + colonna + " = %s, accettato = now(), accettato_da = %s where id = %s",
                      (_json(dict(valori(r), ferma_nodi=ferma_nodi)),
                       'prima esecuzione' if rif is None else 'nuovo limite di nodi', test['id']))
        c.commit()


def valori(r):
    """I campi che devono coincidere, da un'esecuzione (dict o riga del database)."""
    return {k: r.get(k) for k in CAMPI}


def accetta(c, nid, chi):
    """Accetta i numeri di un'esecuzione come nuovo riferimento del suo test."""
    n = c.execute('select * from esecuzioni_notturne where id = %s', (nid,)).fetchone()
    if not n or n['confronto'] not in ('diverso', 'accettato'):
        return False
    t = c.execute('select ferma_nodi_prima from test_notturni where id = %s', (n['test_id'],)).fetchone()
    prima = t and t['ferma_nodi_prima'] == n['ferma_nodi'] and n['ferma_nodi'] != ferma_nodi_impostato(c)
    colonna = 'riferimento_prima' if prima else 'riferimento'
    c.execute('update test_notturni set ' + colonna + ' = %s, accettato = now(), accettato_da = %s where id = %s',
              (_json(dict(valori(n), ferma_nodi=n['ferma_nodi'])), chi, n['test_id']))
    c.execute("update esecuzioni_notturne set confronto = 'accettato' where id = %s", (nid,))
    return True


# ---- il cambio del limite di nodi ----

def cambia_limite(c, nuovo):
    """Il limite di nodi passa a `nuovo`. Ogni test che finora ha girato con un altro limite gira anche con quello
    (ferma_nodi_prima, con il suo riferimento), finche' non ha la storia anche con il limite nuovo (aggiorna_passaggio).
    Un test gia' in un cambio tiene il limite di prima di allora; se si torna a quello, il cambio finisce."""
    c.execute('''update test_notturni set ferma_nodi_prima = null, riferimento = coalesce(riferimento_prima, riferimento),
                 riferimento_prima = null where ferma_nodi_prima = %s''', (nuovo,))
    c.execute('''
        with ultime as (
            select distinct on (test_id) test_id, ferma_nodi from esecuzioni_notturne
            where ferma_nodi is not null and esito is not null and esito not in ('fermato', 'interrotto')
            order by test_id, id desc)
        update test_notturni t set ferma_nodi_prima = u.ferma_nodi,
            riferimento_prima = case when (t.riferimento->>'ferma_nodi')::bigint = u.ferma_nodi then t.riferimento end
        from ultime u
        where u.test_id = t.id and t.ferma_nodi_prima is null and u.ferma_nodi <> %s''', (nuovo,))


def aggiorna_passaggio(c, tid, limite):
    """Dopo un test: se gli ultimi ESECUZIONI_DA_TENERE giri in cui ha girato hanno tutti un'esecuzione con il limite
    impostato e una con quello di prima, il test passa al limite nuovo (gira una volta sola)."""
    t = c.execute('select ferma_nodi_prima from test_notturni where id = %s', (tid,)).fetchone()
    if not t or t['ferma_nodi_prima'] is None:
        return
    prima = t['ferma_nodi_prima']
    giri = c.execute('''
        select giro_id, bool_or(ferma_nodi = %s) as nuovo, bool_or(ferma_nodi = %s) as vecchio
        from esecuzioni_notturne where test_id = %s and esito is not null and esito not in ('fermato', 'interrotto')
        group by giro_id order by giro_id desc limit %s''', (limite, prima, tid, ESECUZIONI_DA_TENERE)).fetchall()
    if prima == limite or (len(giri) == ESECUZIONI_DA_TENERE and all(g['nuovo'] and g['vecchio'] for g in giri)):
        c.execute('update test_notturni set ferma_nodi_prima = null, riferimento_prima = null where id = %s', (tid,))


# ---- di notte ----

def avvia_sorveglianza(config):
    """Il thread che ogni minuto guarda se e' l'ora dei test notturni (una volta per processo del sito)."""
    global _sorveglianza_avviata
    if _sorveglianza_avviata:
        return
    _sorveglianza_avviata = True
    with db.connetti(config.database_url) as c:  # giri lasciati a meta' da un riavvio del sito
        c.execute("update esecuzioni_notturne set esito = 'interrotto', confronto = 'errore' where esito is null")
        c.execute("update giri_notturni set stato = 'interrotto', fine = now() where stato = 'in corso'")
        c.commit()
    threading.Thread(target=_sorveglia, args=(config,), daemon=True, name='notturni-sorveglianza').start()


def ferma_nodi_impostato(c):
    """Il limite di nodi dei test notturni (uno per tutti, in alto nella pagina, in milioni da 1 a 100)."""
    r = c.execute("select valore from impostazioni where nome = 'ferma_nodi_notturni'").fetchone()
    try:
        return max(1, int(r['valore'])) if r and r['valore'] else FERMA_NODI
    except ValueError:
        return FERMA_NODI


def ora_impostata(c):
    r = c.execute("select valore from impostazioni where nome = 'ora_test_notturni'").fetchone()
    return (r['valore'] if r else '') or '02:00'


def _sorveglia(config):
    while True:
        try:
            _forse_avvia(config)
        except Exception:  # per esempio il database non raggiungibile: si riprova al minuto dopo
            traceback.print_exc()
        time.sleep(60)


def _forse_avvia(config, adesso=None):
    """Dall'ora impostata fino a sei ore dopo, per ogni scuola con test attivi e senza un giro di notte oggi: un giro.
    Le scuole vanno una dopo l'altra (un giro alla volta)."""
    adesso = adesso or datetime.datetime.now().astimezone()
    with db.connetti(config.database_url) as c:
        valore = ora_impostata(c)
        if valore == 'mai':
            return
        hh, mm = (int(x) for x in valore.split(':'))
        partenza = adesso.replace(hour=hh, minute=mm, second=0, microsecond=0)
        if not partenza <= adesso < partenza + datetime.timedelta(hours=6):
            return
        scuole = [r['scuola_id'] for r in c.execute(
            "select distinct t.scuola_id from test_notturni t where t.attivo and not exists ("
            "  select 1 from giri_notturni g where g.scuola_id = t.scuola_id and g.motivo = 'notte' and g.inizio >= %s)"
            " order by 1", (partenza,))]
    for sid in scuole:
        if giro_in_corso() is None:
            avvia_giro(config, sid, 'notte')
        return  # una scuola per volta: la prossima al controllo dopo la fine di questo giro
