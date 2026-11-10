"""I dati di un caso (dal solver: ./untitled dati) riassunti per la pagina del test: le classi con le lezioni, i
docenti, le codocenze e il loro grafo."""
import math


def riassunto_dati(dati):
    """I dati del caso per la pagina:
    - classi: nome, ore, quanti docenti, lezioni (docente, ore, codocente, ore_codocenza: le ore insieme);
    - docenti (in ordine di nome): ore in tutto, da solo e in coppia, ora di ingresso, per ogni classe le sue ore,
      e i codocenti con le ore insieme e le classi;
    - coppie: le codocenze (a, b, ore, classi);
    - riepilogo: i totali."""
    classi, docenti, coppie = {}, {}, {}
    ingresso = {e['docente']: e['ora'] for e in dati.get('ingressi', [])}

    def docente(nome, classe, ore, insieme, con=None):
        d = docenti.setdefault(nome, {'nome': nome, 'ore': 0, 'in_coppia': 0, 'classi': {}, 'codocenti': {},
                                      'ingresso': ingresso.get(nome, 0)})
        d['ore'] += ore
        d['in_coppia'] += insieme
        c = d['classi'].setdefault(classe, {'classe': classe, 'ore': 0, 'in_coppia': 0})
        c['ore'] += ore
        c['in_coppia'] += insieme
        if con:
            x = d['codocenti'].setdefault(con, {'nome': con, 'ore': 0, 'classi': []})
            x['ore'] += insieme
            if classe not in x['classi']:
                x['classi'].append(classe)

    for l in dati['lezioni']:
        c = classi.setdefault(l['classe'], {'nome': l['classe'], 'ore': 0, 'lezioni': [], 'nomi': set()})
        c['ore'] += l['ore']
        cod, insieme = l.get('codocente'), l.get('ore_codocenza', 0)
        c['lezioni'].append({'docente': l['docente'], 'ore': l['ore'], 'codocente': cod, 'ore_codocenza': insieme})
        c['nomi'].add(l['docente'])
        docente(l['docente'], l['classe'], l['ore'], insieme, cod)
        if cod:
            c['nomi'].add(cod)
            docente(cod, l['classe'], insieme, insieme, l['docente'])
            k = tuple(sorted((l['docente'], cod)))
            x = coppie.setdefault(k, {'a': k[0], 'b': k[1], 'ore': 0, 'classi': []})
            x['ore'] += insieme
            if l['classe'] not in x['classi']:
                x['classi'].append(l['classe'])
    for c in classi.values():
        c['docenti'] = len(c.pop('nomi'))
    elenco = sorted(docenti.values(), key=lambda d: d['nome'].lower())
    for d in elenco:
        d['classi'] = list(d['classi'].values())
        d['codocenti'] = sorted(d['codocenti'].values(), key=lambda x: -x['ore'])
    riepilogo = {'classi': len(classi), 'docenti': len(elenco), 'ore': sum(c['ore'] for c in classi.values()),
                 'ore_in_coppia': sum(x['ore'] for x in coppie.values()), 'coppie': len(coppie),
                 'in_ritardo': sum(1 for d in elenco if d['ingresso']),
                 'ore_docente_max': max((d['ore'] for d in elenco), default=1)}
    return list(classi.values()), elenco, list(coppie.values()), riepilogo


def grafo(coppie, lato=560):
    """Il grafo delle codocenze: i docenti che ne hanno su un cerchio, un filo per coppia, spessore secondo le ore."""
    nomi = sorted({c['a'] for c in coppie} | {c['b'] for c in coppie}, key=str.lower)
    if not nomi:
        return None
    r, centro = lato / 2 - 90, lato / 2
    punti = {}
    for i, n in enumerate(nomi):
        a = 2 * math.pi * i / len(nomi) - math.pi / 2
        punti[n] = (centro + r * math.cos(a), centro + r * math.sin(a), math.cos(a))
    fili = [dict(c, x1=punti[c['a']][0], y1=punti[c['a']][1], x2=punti[c['b']][0], y2=punti[c['b']][1],
                 spessore=1 + 1.2 * c['ore']) for c in coppie]
    nodi = [{'nome': n, 'x': x, 'y': y, 'lx': x + 8 * cx / max(abs(cx), 0.01) * min(1, abs(cx) * 3),
             'ancora': 'start' if cx > 0.15 else 'end' if cx < -0.15 else 'middle'} for n, (x, y, cx) in punti.items()]
    return {'lato': lato, 'fili': fili, 'nodi': nodi}


