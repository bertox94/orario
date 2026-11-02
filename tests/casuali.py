#!/usr/bin/env python3
"""Prova il solver su dati casuali.

Uso: tests/casuali.py [--da SEME] [--quanti N] [--classi K] [--timeout SEC]

Per ogni seme compila (una volta) il solver, lo lancia con `--casuale SEME --classi K -n 1`, controlla che i dati
generati rispettino i vincoli (classi 29..36 ore, in pratica 30..36; docenti 5..24 ore) e classifica l'esito:
  trovato          almeno un orario completo
  nessuna          la ricerca e' finita senza trovare orari
  timeout          non finito entro il tempo dato
Esce con codice 1 se qualche dato generato viola i vincoli."""
import argparse, os, re, subprocess, sys, tempfile, collections, time

AQUI = os.path.dirname(os.path.abspath(__file__))
RADICE = os.path.dirname(AQUI)
FILES = ['main.c', 'data_initialization/database_initializers.c', 'data_initialization/generatore.c',
         'printer/printer.c', 'priority_queue/prio_queue.c']


def compila(dest):
    cmd = ['gcc', '-O2', '-std=gnu11', '-w', '-include', 'time.h'] + [os.path.join(RADICE, f) for f in FILES] + \
          ['-o', dest]
    subprocess.run(cmd, check=True)


def controlla_dati(out):
    errori = []
    classi = [int(m.group(1)) for m in re.finditer(r'^Classe \S+:\s+(\d+) ore', out, re.M)]
    docenti = [int(m.group(1)) for m in re.finditer(r'^Ins\. \S+\s*: (\d+) ore', out, re.M)]
    if not classi or not docenti:
        errori.append('dati non trovati nell\'output')
    for o in classi:
        if not 29 <= o <= 36:
            errori.append('classe con %d ore' % o)
    for o in docenti:
        if not 5 <= o <= 24:
            errori.append('docente con %d ore' % o)
    return errori, len(classi), len(docenti)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--da', type=int, default=1)
    ap.add_argument('--quanti', type=int, default=20)
    ap.add_argument('--classi', type=int, default=8)
    ap.add_argument('--timeout', type=float, default=20)
    a = ap.parse_args()

    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, 'solver')
        compila(exe)
        esiti = collections.Counter()
        cattivi = 0
        for seme in range(a.da, a.da + a.quanti):
            t = time.time()
            try:
                r = subprocess.run([exe, '--casuale', str(seme), '--classi', str(a.classi), '-n', '1'], cwd=tmp,
                                   capture_output=True, text=True, timeout=a.timeout)
                out = r.stdout
                m = re.search(r'Totale generati: (\d+)', out)
                esito = 'trovato' if m and int(m.group(1)) > 0 else 'nessuna'
                if r.returncode != 0:
                    esito = 'errore'
            except subprocess.TimeoutExpired as e:
                out = (e.stdout or b'').decode() if isinstance(e.stdout, bytes) else (e.stdout or '')
                esito = 'timeout'
            errori, nc, nd = controlla_dati(out)
            if errori:
                cattivi += 1
            esiti[esito] += 1
            print('seme %4d: %2d classi, %3d docenti, %-8s %5.1fs %s' % (
                seme, nc, nd, esito, time.time() - t, ('DATI NON VALIDI: ' + '; '.join(errori)) if errori else ''),
                flush=True)
        print('\nRiepilogo:', dict(esiti), '| dati non validi:', cattivi)
        sys.exit(1 if cattivi else 0)


if __name__ == '__main__':
    main()
