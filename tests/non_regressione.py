#!/usr/bin/env python3
"""Test di non regressione della ricerca: una modifica che non deve cambiare la ricerca deve dare gli stessi numeri.

    python3 tests/non_regressione.py              confronta con tests/attesi.json
    python3 tests/non_regressione.py --aggiorna   riscrive tests/attesi.json (dopo una modifica voluta alla ricerca)

Compila il solver in una cartella temporanea e per ogni caso di CASI (seme, classi, ritardi, limite di nodi) lancia
./untitled caso: confronta l'esito, i nodi, la cella piu' avanti e l'impronta del file dei passi, cioe' di ogni nodo
visitato e di ogni scelta fatta, nell'ordine. Controlla anche che i dati di prova (./untitled) diano un orario.
"""
import glob
import hashlib
import json
import os
import subprocess
import sys
import tempfile

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ATTESI = os.path.join(RADICE, 'tests', 'attesi.json')
CASI = [(0, 8, 0, 100000), (0, 9, 1, 100000), (1, 8, 0, 100000), (1, 9, 0, 100000), (2, 8, 1, 100000),
        (3, 10, 1, 100000), (5, 9, 0, 100000), (0, 8, 1, 100000), (7, 12, 0, 100000), (4, 3, 1, 100000),
        (5, 6, 1, 100000), (8, 4, 1, 100000), (5, 9, 1, 100000), (7, 10, 1, 100000)]


def esegui():
    with tempfile.TemporaryDirectory() as d:
        exe = os.path.join(d, 'untitled')
        sorgenti = [f for c in ('.', 'data_initialization', 'priority_queue')
                    for f in sorted(glob.glob(os.path.join(RADICE, c, '*.c')))]
        subprocess.run(['gcc', '-O2', '-std=gnu11', '-w'] + sorgenti + ['-o', exe, '-lm'], check=True)
        trovati = {'dati di prova': subprocess.run([exe], capture_output=True, text=True).stdout.split(' in ')[0]}
        for seme, classi, ritardi, limite in CASI:
            subprocess.run([exe, 'caso', str(seme), str(classi), str(ritardi), str(limite), 'stato.json', 'passi.bin'],
                           cwd=d, check=False)
            with open(os.path.join(d, 'stato.json')) as f:
                s = json.load(f)
            with open(os.path.join(d, 'passi.bin'), 'rb') as f:
                passi = hashlib.md5(f.read()).hexdigest()
            trovati['seme %d, %d classi, %s ritardi, %d nodi' % (seme, classi, 'con' if ritardi else 'senza', limite)] = {
                'esito': s['esito'], 'nodi': s['nodi'], 'cella_max': s['pos_max'], 'passi': passi}
    return trovati


def main():
    trovati = esegui()
    if '--aggiorna' in sys.argv:
        with open(ATTESI, 'w') as f:
            json.dump(trovati, f, indent=1, ensure_ascii=False)
            f.write('\n')
        print('Scritto %s (%d casi).' % (ATTESI, len(trovati)))
        return 0
    with open(ATTESI) as f:
        attesi = json.load(f)
    diversi = [k for k in sorted(set(attesi) | set(trovati)) if attesi.get(k) != trovati.get(k)]
    for k in diversi:
        print('DIVERSO  %s\n  atteso:  %s\n  trovato: %s' % (k, attesi.get(k), trovati.get(k)))
    print('%d casi, %s' % (len(trovati), 'tutto come atteso' if not diversi else '%d diversi' % len(diversi)))
    return 1 if diversi else 0


if __name__ == '__main__':
    sys.exit(main())
