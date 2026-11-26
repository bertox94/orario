#!/usr/bin/env python3
"""La prova del programma: genera un elenco di casi, ne cerca l'orario e controlla i risultati.

    python3 prove/prova.py              confronta con prove/attesi.json
    python3 prove/prova.py --aggiorna   riscrive prove/attesi.json (dopo una modifica voluta)

Per ogni caso (seme, classi, ritardi) guarda tre cose:
- l'impronta del caso generato: se cambia, e' cambiato il generatore (e con lui tutti i casi);
- l'esito: «trovato» o «impossibile». Per lo stesso caso e' sempre lo stesso: se cambia, e' cambiato il metodo;
- se l'orario trovato passa la verifica.
I tempi non si confrontano: cambiano da un computer all'altro.
"""
import hashlib
import json
import os
import sys
import tempfile

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, RADICE)
from orario import esatto, genera, scuola, verifica   # noqa: E402 (dopo aver detto dove sta il programma)

ATTESI = os.path.join(RADICE, 'prove', 'attesi.json')
TEMPO = 300   # i secondi che ogni caso ha
# i casi: una griglia piccola (semi, classi, con e senza ritardi) e alcuni casi scelti: i due piu' grandi, due che la
# ricerca di prima non risolveva, e uno impossibile che la ricerca di prima non riusciva a dimostrare
CASI = [(seme, classi, ritardi) for seme in range(6) for classi in (3, 5, 8, 12) for ritardi in (0, 1)] + \
       [(10, 3, 1), (37, 3, 1), (4, 7, 1), (1, 14, 1), (2, 20, 0), (3, 30, 1)]


def esegui():
    trovati = {}
    with tempfile.TemporaryDirectory() as d:
        file = os.path.join(d, 'caso.txt')
        for seme, classi, ritardi in CASI:
            testo = genera.genera(seme, classi, ritardi)
            with open(file, 'w', encoding='utf-8') as f:
                f.write(testo)
            s = scuola.leggi(file)
            esito, orario, _ = esatto.risolvi(s, TEMPO)
            # il nome del caso e' la chiave in attesi.json
            nome = 'seme %d, %d classi, %s ritardi' % (seme, classi, 'con' if ritardi else 'senza')
            trovati[nome] = {'caso': hashlib.sha256(testo.encode()).hexdigest()[:12], 'esito': esito}
            if orario:
                trovati[nome]['verificato'] = bool(verifica.verifica(s, orario)['ok'])
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
    # diverso: un caso in piu' o in meno, o con impronta, esito o verifica cambiati
    diversi = [k for k in sorted(set(attesi) | set(trovati)) if attesi.get(k) != trovati.get(k)]
    for k in diversi:
        print('DIVERSO  %s\n  atteso:  %s\n  trovato: %s' % (k, attesi.get(k), trovati.get(k)))
    print('%d casi, %s' % (len(trovati), 'tutto come atteso' if not diversi else '%d diversi' % len(diversi)))
    return 1 if diversi or non_validi else 0


if __name__ == '__main__':
    sys.exit(main())
