"""I dati della scuola nel database, e la loro conversione nel formato che il solver legge con --input."""
import json

# Regole fisse del solver (data_initialization/database_initializers.h): servono ai controlli delle pagine.
ORE_OBBLIGATORIE = 5
MAX_ORE_DOCENTE_IN_CLASSE_AL_GIORNO = 3


def leggi(c):
    """Tutti i dati: calendario, classi, docenti (in ordine di id) e lezioni con i nomi."""
    cal = c.execute('select giorni, ore from calendario').fetchone()
    classi = c.execute('select id, nome from classi order by id').fetchall()
    docenti = c.execute('select id, nome, ingresso from docenti order by id').fetchall()
    lezioni = c.execute('''
        select l.*, cl.nome as classe, d.nome as docente, cd.nome as codocente
        from lezioni l join classi cl on cl.id = l.classe_id join docenti d on d.id = l.docente_id
        left join docenti cd on cd.id = l.codocente_id
        order by cl.id, d.nome, l.id''').fetchall()
    return {'giorni': cal['giorni'], 'ore': cal['ore'], 'classi': classi, 'docenti': docenti, 'lezioni': lezioni}


def per_solver(dati):
    """Il JSON per --input (vedi data_initialization/dati_da_file.h). Le lezioni del sito (docente con N ore, di cui M
    con un codocente) diventano due lezioni del solver: N - M ore da solo e M ore in coppia."""
    indice_classe = {cl['id']: i for i, cl in enumerate(dati['classi'])}
    indice_docente = {d['id']: i for i, d in enumerate(dati['docenti'])}
    lezioni = []
    for l in dati['lezioni']:
        c, p = indice_classe[l['classe_id']], indice_docente[l['docente_id']]
        da_solo = l['ore'] - l['ore_codocente']
        if da_solo > 0:
            lezioni.append({'classe': c, 'docente': p, 'codocente': None, 'ore': da_solo})
        if l['codocente_id'] is not None:
            lezioni.append({'classe': c, 'docente': p, 'codocente': indice_docente[l['codocente_id']],
                            'ore': l['ore_codocente']})
    return {'giorni': dati['giorni'], 'ore': dati['ore'],
            'classi': [{'nome': cl['nome']} for cl in dati['classi']],
            'docenti': [{'nome': d['nome'], 'ingresso': d['ingresso']} for d in dati['docenti']],
            'lezioni': lezioni}


def scrivi_per_solver(c, file):
    with open(file, 'w') as f:
        json.dump(per_solver(leggi(c)), f, ensure_ascii=False, indent=1)


def sostituisci(c, dati_solver):
    """Sostituisce tutti i dati con quelli in formato solver (la sezione "dati" del JSON di --json).

    Le lezioni del solver (da solo, o in coppia) si ricompongono in lezioni del sito: le ore in coppia si uniscono
    alle ore da solo dello stesso docente nella stessa classe, se ne ha (quel docente e' il titolare, l'altro il
    codocente); altrimenti diventano una lezione tutta in coppia."""
    c.execute('delete from lezioni')
    c.execute('delete from classi')
    c.execute('delete from docenti')
    c.execute('update calendario set giorni = %s, ore = %s', (dati_solver['giorni'], dati_solver['ore']))
    id_classe = [c.execute('insert into classi (nome) values (%s) returning id', (cl['nome'],)).fetchone()['id']
                 for cl in dati_solver['classi']]
    id_docente = [c.execute('insert into docenti (nome, ingresso) values (%s, %s) returning id',
                            (d['nome'], d.get('ingresso', 0))).fetchone()['id'] for d in dati_solver['docenti']]
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
