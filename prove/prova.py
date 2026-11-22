#!/usr/bin/env python3
"""La prova del solver: compila, risolve un elenco di casi e controlla i risultati.

    python3 prove/prova.py              confronta con prove/attesi.json
    python3 prove/prova.py --aggiorna   riscrive prove/attesi.json (dopo una modifica voluta alla ricerca)

Per ogni caso (seme, classi, ritardi) guarda l'esito, i nodi e, se l'orario c'e', che passi la verifica. La ricerca e'
sempre la stessa per lo stesso caso: se un numero cambia, e' cambiato il solver.
"""
import glob
import json
import os
import subprocess
import sys
import tempfile

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ATTESI = os.path.join(RADICE, 'prove', 'attesi.json')
# il limite di nodi di ogni caso: basso, perche' la prova deve durare poco
# le cartelle del programma in C (come nel Makefile)
CARTELLE = ('scuola', 'generatore', 'solver', 'verifica', 'passi', 'spiegazioni', 'programma')
LIMITE = 200000
# i casi: una griglia piccola (semi, classi, con e senza ritardi) e quattro casi scelti, tra cui i piu' grandi
CASI = [(seme, classi, ritardi) for seme in range(6) for classi in (3, 5, 8, 12) for ritardi in (0, 1)] + \
       [(10, 3, 1), (1, 14, 1), (2, 20, 0), (3, 30, 1)]


def esegui():
    trovati = {}
    # tutto in una cartella provvisoria: l'eseguibile e il file del caso
    with tempfile.TemporaryDirectory() as d:
        exe, caso = os.path.join(d, 'orario'), os.path.join(d, 'caso.txt')
        subprocess.run(['gcc', '-O2', '-std=gnu11', '-Wall', '-Wextra'] + sorted(f for c in CARTELLE for f in glob.glob('%s/%s/*.c' % (RADICE, c)))
                       + ['-o', exe], check=True)
        for seme, classi, ritardi in CASI:
            with open(caso, 'w') as f:
                subprocess.run([exe, 'genera', str(seme), str(classi), str(ritardi)], stdout=f, check=True)
            r = subprocess.run([exe, 'risolvi', caso, '--limite', str(LIMITE)], capture_output=True, text=True)
            s = json.loads(r.stdout)
            # il nome del caso e' la chiave in attesi.json
            nome = 'seme %d, %d classi, %s ritardi' % (seme, classi, 'con' if ritardi else 'senza')
            trovati[nome] = {'esito': s['esito'], 'nodi': s['nodi']}
            if s['esito'] == 'trovato':
                trovati[nome]['verificato'] = bool(s['verifica']['ok'])
    return trovati


def main():
    trovati = esegui()
    non_validi = [k for k, v in trovati.items() if v.get('verificato') is False]
    for k in non_validi:
        print('NON VALIDO  %s: l\'orario trovato non passa la verifica' % k)
    if '--aggiorna' in sys.argv:
        with open(ATTESI, 'w') as f:
            json.dump(trovati, f, indent=1, ensure_ascii=False)
            f.write('\n')
        print('Scritto %s (%d casi).' % (ATTESI, len(trovati)))
        return 1 if non_validi else 0
    with open(ATTESI) as f:
        attesi = json.load(f)
    # diverso: un caso in piu' o in meno, o con esito, nodi o verifica cambiati
    diversi = [k for k in sorted(set(attesi) | set(trovati)) if attesi.get(k) != trovati.get(k)]
    for k in diversi:
        print('DIVERSO  %s\n  atteso:  %s\n  trovato: %s' % (k, attesi.get(k), trovati.get(k)))
    print('%d casi, %s' % (len(trovati), 'tutto come atteso' if not diversi else '%d diversi' % len(diversi)))
    return 1 if diversi or non_validi else 0


if __name__ == '__main__':
    sys.exit(main())
