"""I dati di un caso (generatore.py: gli stessi del solver per ogni seme) riassunti per la pagina del test: le classi
con le lezioni, i docenti, le codocenze e il loro grafo; e i docenti nell'ordine del solver (per i passi)."""
import functools
import math

from . import generatore


def dati_del_caso(info):
    """I dati del test, rigenerati dal seme (identici a quelli del solver, data_initialization/generatore.c)."""
    return _genera(info["seme"], info["classi"], bool(info["ritardi"]))


@functools.lru_cache(maxsize=32)
def _genera(seme, classi, ritardi):
    return generatore.genera(seme, classi, ritardi)


def docenti_in_ordine(dati):
    """I nomi dei docenti nell'ordine del solver (data_initialization/generatore.c): docente e poi codocente di ogni
    lezione, alla loro prima comparsa (come data_initialization/generatore.c). L'indice nei passi e' la posizione qui."""
    nomi = []
    for l in dati['lezioni']:
        for d in (l['docente'], l.get('codocente')):
            if d and d not in nomi:
                nomi.append(d)
    return nomi


def riassunto_dati(dati):
    """Per ogni classe le ore e le lezioni; per ogni docente le ore, l'ora di ingresso e le classi; le codocenze."""
    classi, docenti, coppie = {}, {}, {}
    ingresso = {e['docente']: e['ora'] for e in dati.get('ingressi', [])}

    def docente(nome, classe, ore):
        d = docenti.setdefault(nome, {'nome': nome, 'ore': 0, 'classi': [], 'ingresso': ingresso.get(nome, 0)})
        d['ore'] += ore
        if classe not in d['classi']:
            d['classi'].append(classe)

    for l in dati['lezioni']:
        c = classi.setdefault(l['classe'], {'nome': l['classe'], 'ore': 0, 'lezioni': []})
        c['ore'] += l['ore']
        cod = l.get('codocente')
        c['lezioni'].append({'docente': l['docente'], 'ore': l['ore'], 'codocente': cod,
                             'ore_codocenza': l.get('ore_codocenza', 0)})
        docente(l['docente'], l['classe'], l['ore'])
        if cod:
            docente(cod, l['classe'], l['ore_codocenza'])
            k = tuple(sorted((l['docente'], cod)))
            x = coppie.setdefault(k, {'a': k[0], 'b': k[1], 'ore': 0, 'classi': []})
            x['ore'] += l['ore_codocenza']
            if l['classe'] not in x['classi']:
                x['classi'].append(l['classe'])
    return list(classi.values()), sorted(docenti.values(), key=lambda d: d['nome'].lower()), list(coppie.values())


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


