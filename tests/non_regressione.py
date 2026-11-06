#!/usr/bin/env python3
"""Test di non regressione dei solver: ogni solver su ogni caso di tests/casi/, confrontato con tests/attesi.json.

Per ogni (solver, caso) si confrontano esito, nodi, backtrack e il primo orario trovato (impronta): una modifica che
non deve cambiare la ricerca deve dare gli stessi valori. Ogni orario trovato passa anche dal verificatore indipendente
(verifica_orario.py: tutte le regole). Sui dati di prova scritti nel solver (senza --input) si confronta l'md5 di
orario.txt, come in CLAUDE.md.

Uso, dalla radice del repository:
    python3 tests/non_regressione.py                  tutti i casi, i solver main, per-classe, in-avanti, giorni-liberi e
                                                      ordini-classi
    python3 tests/non_regressione.py --solver main    un solver solo (anche piu' volte; due-livelli solo se chiesto)
    python3 tests/non_regressione.py --aggiorna       riscrive tests/attesi.json con i valori di adesso (dopo una
                                                      modifica voluta della ricerca: va detto nel commit)
    python3 tests/non_regressione.py --caso seme_3_8  solo i casi il cui nome contiene il testo
Esce con 1 se qualcosa non torna. I casi sono file JSON nel formato di --input (tests/casi/*.json): quelli costruiti
apposta (fattibilita_*: dati impossibili, che devono finire con «errore nei dati» in in-avanti) e quelli del
generatore del sito (seme_*: gli stessi del modulo del test, salvati qui cosi' il test non dipende dal sito).
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
import time

QUI = os.path.dirname(os.path.abspath(__file__))
RADICE = os.path.dirname(QUI)
sys.path.insert(0, QUI)
from verifica_orario import verifica  # noqa: E402

SOLVER_PREDEFINITI = ['main', 'per-classe', 'in-avanti', 'giorni-liberi', 'ordini-classi']
TEMPO_MASSIMO = 60  # secondi per (solver, caso): i casi scelti finiscono in pochi secondi
MD5_RIFERIMENTO = 'b7e8ad1800ea16bc0fc8be628c926fe6'  # orario.txt di main sui dati di prova (CLAUDE.md)


def cartella_solver(nome):
    return RADICE if nome == 'main' else os.path.join(RADICE, 'solutori', nome)


def sorgenti(nome):
    """I .c dal CMakeLists.txt del solver (come fa il sito)."""
    testo = open(os.path.join(cartella_solver(nome), 'CMakeLists.txt')).read()
    blocco = re.search(r'add_executable\(\s*\S+(.*?)\)', testo, re.S).group(1)
    return [os.path.join(cartella_solver(nome), f) for f in blocco.split() if f.endswith('.c')]


def compila(nome, cartella_bin):
    exe = os.path.join(cartella_bin, 'solver-' + nome)
    subprocess.run([os.environ.get('CC', 'gcc'), '-O2', '-std=gnu11', '-w'] + sorgenti(nome) + ['-o', exe, '-lm'],
                   check=True)
    return exe


def leggi_traccia(percorso):
    d = orario = fine = errore = None
    with open(percorso) as f:
        for riga in f:
            try:
                x = json.loads(riga)
            except ValueError:
                continue
            t = x.get('tipo')
            if t == 'dati':
                d = x
            elif t == 'orario' and orario is None:
                orario = x['orario']
            elif t == 'fine':
                fine = x
            elif t == 'errore':
                errore = x['messaggio']
    return d, orario, fine, errore


def esegui(exe, dati, lavoro):
    """Un solver su un caso: {'esito', 'nodi', 'backtrack', 'orario' (impronta), 'errore', 'valido', 'secondi'}."""
    with open(os.path.join(lavoro, 'dati.json'), 'w') as f:
        json.dump(dati, f)
    traccia = os.path.join(lavoro, 'traccia.jsonl')
    inizio = time.time()
    try:
        subprocess.run([exe, '--input', 'dati.json', '--traccia', 'traccia.jsonl', '-n', '1', '--txt', '/dev/null',
                        '--max-backtrack', '0'], cwd=lavoro, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       timeout=TEMPO_MASSIMO)
    except subprocess.TimeoutExpired:
        return {'esito': 'tempo scaduto', 'secondi': round(time.time() - inizio, 1)}
    d, orario, fine, errore = leggi_traccia(traccia)
    r = {'secondi': round(time.time() - inizio, 2)}
    if errore:
        r.update(esito='errore nei dati', errore=errore)
    elif fine:
        r.update(esito=fine['esito'], nodi=fine['nodi'], backtrack=fine['backtrack'])
    else:
        r['esito'] = 'errore'
    if orario is not None:
        r['orario'] = hashlib.md5(json.dumps(orario).encode()).hexdigest()[:12]
        r['valido'] = not verifica(d, orario)
        r['errori'] = verifica(d, orario)[:3]
    return r


def md5_dati_di_prova(exe, lavoro):
    """Il solver sui suoi dati di prova (senza --input): l'md5 di orario.txt senza le righe 'Current time'."""
    subprocess.run([exe, '-n', '11000'], cwd=lavoro, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
    h = hashlib.md5()
    with open(os.path.join(lavoro, 'orario.txt'), 'rb') as f:
        for riga in f:
            if not riga.startswith(b'Current time:'):
                h.update(riga)
    os.remove(os.path.join(lavoro, 'orario.txt'))
    return h.hexdigest()


def confronta(nome, atteso, trovato):
    """Le differenze tra l'atteso e il trovato, per i campi che contano."""
    campi = ['esito', 'nodi', 'backtrack', 'orario', 'errore']
    return ['%s: atteso %r, trovato %r' % (k, atteso.get(k), trovato.get(k)) for k in campi
            if atteso.get(k) != trovato.get(k)]


def main():
    a = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    a.add_argument('--solver', action='append', help='quali solver (default: %s)' % ', '.join(SOLVER_PREDEFINITI))
    a.add_argument('--caso', help='solo i casi il cui nome contiene questo testo')
    a.add_argument('--aggiorna', action='store_true', help='riscrive tests/attesi.json con i valori di adesso')
    args = a.parse_args()
    solver = args.solver or SOLVER_PREDEFINITI
    percorso_attesi = os.path.join(QUI, 'attesi.json')
    attesi = json.load(open(percorso_attesi)) if os.path.exists(percorso_attesi) else {}
    casi = sorted(f[:-5] for f in os.listdir(os.path.join(QUI, 'casi')) if f.endswith('.json'))
    if args.caso:
        casi = [c for c in casi if args.caso in c]
    problemi = 0
    with tempfile.TemporaryDirectory() as tmp:
        for nome in solver:
            exe = compila(nome, tmp)
            lavoro = os.path.join(tmp, nome)
            os.makedirs(lavoro)
            risultati = attesi.setdefault(nome, {}) if args.aggiorna else {}
            if not args.caso and nome == 'main':  # gli altri solver non hanno un riferimento (e per-classe e' lento qui)
                md5 = md5_dati_di_prova(exe, lavoro)
                atteso = attesi.get(nome, {}).get('dati_di_prova')
                if args.aggiorna:
                    risultati['dati_di_prova'] = md5
                elif atteso and md5 != atteso:
                    problemi += 1
                    print('%-12s dati di prova: md5 %s, atteso %s' % (nome, md5, atteso))
                elif nome == 'main' and md5 != MD5_RIFERIMENTO:
                    problemi += 1
                    print('%-12s dati di prova: md5 %s, il riferimento di CLAUDE.md e\' %s' % (nome, md5, MD5_RIFERIMENTO))
                else:
                    print('%-12s dati di prova: md5 %s ok' % (nome, md5))
            for caso in casi:
                dati = json.load(open(os.path.join(QUI, 'casi', caso + '.json')))
                atteso = attesi.get(nome, {}).get(caso)
                if not args.aggiorna and atteso and atteso.get('esito') == 'tempo scaduto':
                    print('%-12s %-24s saltato (per questo solver non finisce in tempo)' % (nome, caso))
                    continue
                r = esegui(exe, dati, lavoro)
                if 'valido' in r and not r['valido']:
                    problemi += 1
                    print('%-12s %-24s ORARIO NON VALIDO: %s' % (nome, caso, '; '.join(r['errori'])))
                if args.aggiorna:
                    risultati[caso] = {k: v for k, v in r.items() if k in ('esito', 'nodi', 'backtrack', 'orario', 'errore')}
                    print('%-12s %-24s %s %s' % (nome, caso, r['esito'], ('%d nodi' % r['nodi']) if 'nodi' in r else ''))
                    continue
                if atteso is None:
                    print('%-12s %-24s (nessun valore atteso: --aggiorna per registrarlo) %s' % (nome, caso, r['esito']))
                    continue
                diff = confronta(nome, atteso, r)
                if diff:
                    problemi += 1
                    print('%-12s %-24s DIVERSO: %s' % (nome, caso, '; '.join(diff)))
                else:
                    print('%-12s %-24s ok (%s, %.1f s)' % (nome, caso, r['esito'], r['secondi']))
    if args.aggiorna:
        json.dump(attesi, open(percorso_attesi, 'w'), indent=1, sort_keys=True, ensure_ascii=False)
        print('scritto', percorso_attesi)
    elif problemi:
        print('\n%d problemi' % problemi)
        return 1
    else:
        print('\ntutto come atteso')
    return 0


if __name__ == '__main__':
    sys.exit(main())
