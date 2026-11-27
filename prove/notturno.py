#!/usr/bin/env python3
"""I test notturni fuori dal sito: tanti casi casuali, i due metodi insieme (orario/insieme.py), e un riepilogo.
Lo lancia ogni notte GitHub (.github/workflows/notturni.yml); si puo' lanciare anche a mano.

    python3 prove/notturno.py --semi 0-19 --classi 3-14 --tempo 60 --esce risultati.json
        fa girare i casi (semi x classi x senza e con ritardi) e scrive i risultati
    ... --parti 4 --parte 2
        solo un quarto dei casi (il secondo): per dividere il lavoro tra piu' macchine
    python3 prove/notturno.py --riepilogo risultati-*.json
        mette insieme i risultati e scrive il riepilogo (in Markdown) su stdout; esce con 1 se c'e' un allarme:
        un orario che non passa la verifica, o i due metodi che si contraddicono
"""
import argparse
import collections
import json
import os
import sys
import time

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, RADICE)

NOMI = {'highs': 'HiGHS', 'cpsat': 'CP-SAT'}
CERTI = ('trovato', 'impossibile')


def intervallo(testo):
    """«3-14» -> i numeri da 3 a 14; «7» -> solo 7."""
    da, _, a = testo.partition('-')
    return range(int(da), int(a or da) + 1)


def esegui(semi, classi, tempo, parti, parte):
    """I casi di questa parte, uno dopo l'altro. Ritorna una riga per caso."""
    from orario import genera, insieme, scuola   # (qui: per il riepilogo i risolutori non servono)
    casi = [(seme, k, r) for seme in semi for k in classi for r in (0, 1)]
    casi = casi[parte - 1::parti]   # un caso ogni `parti`, partendo dal numero della parte: cosi' ogni parte ha
                                    # casi piccoli e grandi
    righe = []
    for n, (seme, k, r) in enumerate(casi, 1):
        try:
            s = scuola.leggi_testo(genera.genera(seme, k, bool(r)))
        except ValueError:
            continue   # (un caso che non si riesce a generare non e' un caso)
        stato = insieme.risolvi(s, tempo)
        riga = {'caso': [seme, k, r], 'esito': stato['esito'], 'secondi': stato['secondi'],
                'risolto_da': stato.get('risolto_da'),
                'verificato': bool(stato['verifica']['ok']) if 'verifica' in stato else None,
                'metodi': {nome: [m['esito'], m['secondi'], m.get('verificato')] for nome, m in stato['metodi'].items()}}
        righe.append(riga)
        print('%3d/%d  seme %2d, %2d classi, %s ritardi | %-15s | %s' % (
            n, len(casi), seme, k, 'con' if r else 'senza', riga['esito'],
            ' | '.join('%s: %s %.1f s' % (NOMI.get(nome, nome), m[0], m[1]) for nome, m in riga['metodi'].items())),
            flush=True)
    return righe


def riepilogo(righe):
    """Il riepilogo in Markdown, e se c'e' un allarme."""
    righe = sorted(righe, key=lambda r: r['caso'])
    nome_caso = lambda r: 'seme %d, %d classi, %s ritardi' % (r['caso'][0], r['caso'][1], 'con' if r['caso'][2] else 'senza')
    esiti = collections.Counter(r['esito'] for r in righe)
    non_validi = [r for r in righe if r['verificato'] is False or any(m[2] is False for m in r['metodi'].values())]
    contrari = [r for r in righe if r['esito'] == 'contraddizione']
    t = ['# Test notturni dell\'orario', '',
         '%d casi: %s.' % (len(righe), ', '.join('%d %s' % (q, e) for e, q in esiti.most_common())), '',
         '| Metodo | Risposta certa | Solo lui | Al limite di tempo | Secondi in tutto |', '|---|---|---|---|---|']
    metodi = [n for n in NOMI if any(n in r['metodi'] for r in righe)]
    # (un metodo non installato si dice nella tabella, ma non entra nel confronto)
    installati = [n for n in metodi if any(r['metodi'].get(n, ['non installato'])[0] != 'non installato' for r in righe)]
    for nome in metodi:
        suoi = [r for r in righe if nome in r['metodi']]
        if suoi and all(r['metodi'][nome][0] == 'non installato' for r in suoi):
            t.append('| %s | non installato | | | |' % NOMI[nome])
            continue
        certo = [r for r in suoi if r['metodi'][nome][0] in CERTI]
        solo = [r for r in certo if not any(m[0] in CERTI for altro, m in r['metodi'].items() if altro != nome)]
        t.append('| %s | %d | %d | %d | %.0f |' % (NOMI[nome], len(certo), len(solo),
                                                  sum(1 for r in suoi if r['metodi'][nome][0] == 'limite di tempo'),
                                                  sum(r['metodi'][nome][1] for r in suoi)))
    t += ['', '**Allarmi**: %s.' % ('nessuno' if not (non_validi or contrari) else
                                    '%d orari non validi, %d contraddizioni' % (len(non_validi), len(contrari)))]
    for r in non_validi:
        t.append('- ORARIO NON VALIDO: %s' % nome_caso(r))
    for r in contrari:
        t.append('- CONTRADDIZIONE: %s (%s)' % (nome_caso(r), ', '.join('%s %s' % (NOMI.get(n, n), m[0]) for n, m in r['metodi'].items())))
    senza = [r for r in righe if r['esito'] == 'limite di tempo']
    if senza:
        t += ['', '**Senza risposta** (%d): %s.' % (len(senza), '; '.join(nome_caso(r) for r in senza))]
    # i casi in cui ha risposto uno solo dei due: dove i metodi sono diversi
    if len(installati) > 1:
        diversi = [r for r in righe if len({r['metodi'][n][0] in CERTI for n in installati if n in r['metodi']}) > 1]
        if not diversi:
            t += ['', 'I due metodi hanno risposto negli stessi casi.']
        else:
            t += ['', 'Dove ha risposto uno solo dei due:', '',
                  '| Caso | Esito | ' + ' | '.join(NOMI[n] for n in installati) + ' |', '|---|---|' + '---|' * len(installati)]
            for r in diversi:
                t.append('| %s | %s | %s |' % (nome_caso(r), r['esito'], ' | '.join(
                    '%s %.1f s' % (r['metodi'][n][0], r['metodi'][n][1]) if n in r['metodi'] else '' for n in installati)))
    return '\n'.join(t) + '\n', bool(non_validi or contrari)


def main():
    ap = argparse.ArgumentParser(description='I test notturni fuori dal sito.')
    ap.add_argument('--semi', default='0-19')
    ap.add_argument('--classi', default='3-14')
    ap.add_argument('--tempo', type=float, default=60, help='i secondi per caso')
    ap.add_argument('--parti', type=int, default=1)
    ap.add_argument('--parte', type=int, default=1)
    ap.add_argument('--esce', help='il file dove scrivere i risultati (JSON)')
    ap.add_argument('--riepilogo', nargs='+', metavar='FILE', help='mette insieme questi risultati e ne scrive il riepilogo')
    a = ap.parse_args()
    if a.riepilogo:
        righe = []
        for file in a.riepilogo:
            with open(file) as f:
                righe += json.load(f)
        testo, allarme = riepilogo(righe)
        sys.stdout.write(testo)
        return 1 if allarme else 0
    partenza = time.time()
    righe = esegui(intervallo(a.semi), intervallo(a.classi), a.tempo, a.parti, a.parte)
    if a.esce:
        with open(a.esce, 'w') as f:
            json.dump(righe, f)
    print('%d casi in %.0f secondi' % (len(righe), time.time() - partenza))
    return 0


if __name__ == '__main__':
    sys.exit(main())
