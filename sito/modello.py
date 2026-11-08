"""I dati della scuola nel database, e la loro conversione nel formato che il solver legge con --input.

Il formato del solver (ingresso_uscita/dati_json.h) e' quello di inizializza_dati() in main: giorni, ore, lezioni come
chiamate set_classe_insegnante_ore / set_classe_insegnante_ore_cod (classe, docente, ore e, se c'e', codocente con le
ore in coppia), ore di ingresso. Una lezione del sito e' esattamente una di quelle chiamate.

Le versioni salvate prima di questo formato usano quello del solver precedente (classi e docenti per indice, lezioni da
solo e in coppia separate): sostituisci() le riconosce e le legge ancora.
"""
import json
import os
import re

# Regole fisse del solver (data_initialization/database_initializers.h): servono ai controlli delle pagine.
ORE_OBBLIGATORIE = 5
MAX_ORE_DOCENTE_IN_CLASSE_AL_GIORNO = 3


def leggi(c, scuola_id):
    """Tutti i dati di una scuola: calendario, classi, docenti (in ordine di id) e lezioni con i nomi."""
    cal = c.execute('select giorni, ore from calendario where scuola_id = %s', (scuola_id,)).fetchone()
    classi = c.execute('select id, nome from classi where scuola_id = %s order by id', (scuola_id,)).fetchall()
    docenti = c.execute('select id, nome, ingresso from docenti where scuola_id = %s order by id',
                        (scuola_id,)).fetchall()
    lezioni = c.execute('''
        select l.*, cl.nome as classe, d.nome as docente, cd.nome as codocente
        from lezioni l join classi cl on cl.id = l.classe_id join docenti d on d.id = l.docente_id
        left join docenti cd on cd.id = l.codocente_id
        where cl.scuola_id = %s
        order by cl.id, d.nome, l.id''', (scuola_id,)).fetchall()
    return {'giorni': cal['giorni'], 'ore': cal['ore'], 'classi': classi, 'docenti': docenti, 'lezioni': lezioni}


def per_main(dati):
    """Il JSON per --input del solver: una lezione del sito per ogni chiamata set_classe_insegnante_ore[_cod]; le ore
    di ingresso dei docenti che entrano dopo la prima ora."""
    # nell'ordine in cui le lezioni sono state inserite: nel solver l'ordine delle chiamate decide gli indici dei
    # docenti, quindi l'ordine della coda a parita' di ore (i dati importati danno la stessa ricerca dell'originale)
    lezioni = []
    for l in sorted(dati['lezioni'], key=lambda l: l['id']):
        x = {'classe': l['classe'], 'docente': l['docente'], 'ore': l['ore']}
        if l['codocente_id'] is not None:
            x.update(codocente=l['codocente'], ore_codocenza=l['ore_codocente'])
        lezioni.append(x)
    return {'giorni': list(dati['giorni']), 'ore': list(dati['ore']), 'lezioni': lezioni,
            'ingressi': [{'docente': d['nome'], 'ora': d['ingresso']} for d in dati['docenti'] if d['ingresso']]}


def formato_main(d):
    """Vero se i dati sono nel formato del solver attuale (lezioni con i nomi), falso se nel formato precedente."""
    lezioni = d.get('lezioni') or []
    return bool(lezioni) and isinstance(lezioni[0].get('classe'), str)


RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def dati_del_programma():
    """I dati scritti in inizializza_dati() di main (data_initialization/database_initializers.c), nel formato di
    --input: si leggono dal sorgente, nello stesso ordine delle chiamate."""
    nomi = {}
    for f in ('classes.h', 'professors.h', 'days.h', 'hours.h'):
        for m in re.finditer(r'char\s+(\w+)\[\]\s*=\s*"([^"]*)"', open(os.path.join(RADICE, 'constants', f)).read()):
            nomi[m.group(1)] = m.group(2)
    src = open(os.path.join(RADICE, 'data_initialization', 'database_initializers.c')).read()
    corpo = src[src.index('void inizializza_dati()'):]
    corpo = re.sub(r'/\*.*?\*/', '', corpo, flags=re.S)       # niente chiamate commentate
    corpo = re.sub(r'//[^\n]*', '', corpo)
    lezioni = []
    for m in re.finditer(r'set_classe_insegnante_ore(_cod)?\(([^)]*)\);', corpo):
        a = [x.strip() for x in m.group(2).split(',')]
        l = {'classe': nomi[a[0]], 'docente': nomi[a[1]], 'ore': int(a[2])}
        if m.group(1):
            l.update(codocente=nomi[a[3]], ore_codocenza=int(a[4]))
        lezioni.append(l)
    return {'giorni': [nomi[x] for x in re.findall(r'set_giorno_settimana\((\w+)\)', corpo)],
            'ore': [nomi[x] for x in re.findall(r'set_ora_giornaliera\((\w+)\)', corpo)],
            'lezioni': lezioni,
            'ingressi': [{'docente': nomi[n], 'ora': int(o)} for n, o in
                         re.findall(r'ora_di_entrata_prof\[p_insegnante\((\w+)\)\]\s*=\s*(\d+)', corpo)]}


def sostituisci(c, scuola_id, dati_solver):
    """Sostituisce tutti i dati con quelli in formato solver: quello attuale (vedi per_main) o quello precedente (la
    sezione "dati" del JSON di --json del solver di prima).

    Le lezioni del solver (da solo, o in coppia) si ricompongono in lezioni del sito: le ore in coppia si uniscono
    alle ore da solo dello stesso docente nella stessa classe, se ne ha (quel docente e' il titolare, l'altro il
    codocente); altrimenti diventano una lezione tutta in coppia."""
    c.execute('delete from lezioni where classe_id in (select id from classi where scuola_id = %s)', (scuola_id,))
    c.execute('delete from classi where scuola_id = %s', (scuola_id,))
    c.execute('delete from docenti where scuola_id = %s', (scuola_id,))
    c.execute('update calendario set giorni = %s, ore = %s where scuola_id = %s',
              (dati_solver['giorni'], dati_solver['ore'], scuola_id))
    if formato_main(dati_solver):
        _sostituisci_main(c, scuola_id, dati_solver)
        return
    id_classe = [c.execute('insert into classi (scuola_id, nome) values (%s, %s) returning id',
                           (scuola_id, cl['nome'])).fetchone()['id'] for cl in dati_solver['classi']]
    id_docente = [c.execute('insert into docenti (scuola_id, nome, ingresso) values (%s, %s, %s) returning id',
                            (scuola_id, d['nome'], d.get('ingresso', 0))).fetchone()['id']
                  for d in dati_solver['docenti']]
    da_solo = {}  # (classe, docente) -> ore da solo
    coppie = []
    for l in dati_solver['lezioni']:
        if l.get('codocente') is None:
            k = (l['classe'], l['docente'])
            da_solo[k] = da_solo.get(k, 0) + l['ore']
        else:
            coppie.append(l)
    righe = []  # (classe, docente, ore, codocente, ore_codocente)
    for l in coppie:
        cl, p, q = l['classe'], l['docente'], l['codocente']
        titolare, codocente = (q, p) if da_solo.get((cl, p), 0) == 0 and da_solo.get((cl, q), 0) > 0 else (p, q)
        extra = da_solo.pop((cl, titolare), 0)
        righe.append((cl, titolare, l['ore'] + extra, codocente, l['ore']))
    for (cl, p), ore in da_solo.items():
        righe.append((cl, p, ore, None, 0))
    for cl, p, ore, q, ore_q in sorted(righe, key=lambda r: (r[0], r[1])):
        c.execute('insert into lezioni (classe_id, docente_id, ore, codocente_id, ore_codocente) '
                  'values (%s, %s, %s, %s, %s)',
                  (id_classe[cl], id_docente[p], ore, id_docente[q] if q is not None else None, ore_q))


def _sostituisci_main(c, scuola_id, d):
    """Classi e docenti nell'ordine in cui compaiono, poi una lezione per ogni chiamata."""
    ingresso = {i['docente']: i['ora'] for i in d.get('ingressi') or []}
    id_classe, id_docente = {}, {}

    def classe(nome):
        if nome not in id_classe:
            id_classe[nome] = c.execute('insert into classi (scuola_id, nome) values (%s, %s) returning id',
                                        (scuola_id, nome)).fetchone()['id']
        return id_classe[nome]

    def docente(nome):
        if nome not in id_docente:
            id_docente[nome] = c.execute('insert into docenti (scuola_id, nome, ingresso) values (%s, %s, %s) '
                                         'returning id', (scuola_id, nome, ingresso.get(nome, 0))).fetchone()['id']
        return id_docente[nome]

    for l in d['lezioni']:
        cl, p = classe(l['classe']), docente(l['docente'])
        q = docente(l['codocente']) if l.get('codocente') else None
        c.execute('insert into lezioni (classe_id, docente_id, ore, codocente_id, ore_codocente) '
                  'values (%s, %s, %s, %s, %s)', (cl, p, l['ore'], q, l.get('ore_codocenza', 0) if q else 0))


def riepilogo(dati):
    """Ore di ogni classe e di ogni docente, con gli avvisi sui limiti evidenti (la verifica completa la fa il
    solver con --solo-verifica)."""
    giorni, ore = len(dati['giorni']), len(dati['ore'])
    classi = {cl['id']: {'nome': cl['nome'], 'ore': 0, 'lezioni': 0, 'avvisi': []} for cl in dati['classi']}
    docenti = {d['id']: {'nome': d['nome'], 'ingresso': d['ingresso'], 'ore': 0, 'classi': set(), 'avvisi': []}
               for d in dati['docenti']}
    for l in dati['lezioni']:
        classi[l['classe_id']]['ore'] += l['ore']
        classi[l['classe_id']]['lezioni'] += 1
        docenti[l['docente_id']]['ore'] += l['ore']
        docenti[l['docente_id']]['classi'].add(l['classe_id'])
        if l['codocente_id'] is not None:
            docenti[l['codocente_id']]['ore'] += l['ore_codocente']
            docenti[l['codocente_id']]['classi'].add(l['classe_id'])
    minimo, massimo = ORE_OBBLIGATORIE * giorni, giorni * ore
    for cl in classi.values():
        if cl['ore'] < minimo:
            cl['avvisi'].append('meno delle %d ore obbligatorie (%d ore x %d giorni)' % (minimo, ORE_OBBLIGATORIE, giorni))
        if cl['ore'] > massimo:
            cl['avvisi'].append('piu\' ore degli slot della settimana (%d)' % massimo)
    for d in docenti.values():
        if d['ingresso'] >= ore:
            d['avvisi'].append('ora di ingresso oltre l\'ultima ora')
        disponibili = max(0, ore - d['ingresso']) * giorni
        if d['ore'] > disponibili:
            d['avvisi'].append('%d ore ma dalla sua ora di ingresso ne ha al massimo %d' % (d['ore'], disponibili))
        d['classi'] = len(d['classi'])
    return {'classi': list(classi.values()), 'docenti': list(docenti.values()),
            'ore_totali': sum(cl['ore'] for cl in classi.values()), 'minimo_classe': minimo, 'massimo_classe': massimo}


# ---- versioni salvate dei dati ----

def salva_versione(c, scuola_id, nome, motivo, utente=None):
    """Salva una copia dei dati attuali della scuola (formato solver). Ritorna l'id, o None se non c'e' niente da
    salvare (nessuna classe e nessun docente)."""
    dati = leggi(c, scuola_id)
    if not dati['classi'] and not dati['docenti']:
        return None
    r = riepilogo(dati)
    return c.execute(
        'insert into versioni_dati (scuola_id, nome, motivo, utente, classi, docenti, ore, dati) '
        'values (%s, %s, %s, %s, %s, %s, %s, %s) returning id',
        (scuola_id, nome, motivo, utente, len(dati['classi']), len(dati['docenti']), r['ore_totali'],
         json.dumps(per_main(dati), ensure_ascii=False))).fetchone()['id']
