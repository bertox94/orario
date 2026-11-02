#!/usr/bin/env python3
"""Prova il solver su dati casuali, in tre fasi.

Uso: tests/casuali.py [--da SEME] [--quanti N] [--classi K] [--prima SEC] [--dopo SEC] [--piu-lenti M]
                      [--jobs J] [--max-mb MB] [--out CARTELLA]

1. Fase 1: ogni caso (seme da --da, --quanti casi) gira al massimo --prima secondi (default 60).
2. Fase 2: i casi non finiti nella fase 1 girano di nuovo per al massimo --dopo secondi (default 600; 0 = salta).
3. Fase 3: i casi ancora non finiti, e i --piu-lenti casi finiti con piu' marce indietro (default 3), vengono rilanciati
   con -b: il programma scrive in orario.txt l'orario parziale a ogni marcia indietro.

Per ogni caso c'e' una cartella <out>/seme_<N>/ (default tests/risultati) con:
   dati.txt            descrizione del caso (classi, docenti, lezioni, codocenze)
   risultato.txt       esito di ogni fase, marce indietro, tempi
   orario.txt          il primo orario trovato (se la ricerca e' finita)
   marce_indietro/orario.txt   solo per i casi della fase 3: orario parziale a ogni marcia indietro, ciascuno al massimo --max-mb megabyte (default 4096, 0 = senza limite).

Controlla anche che i dati generati rispettino i vincoli (classi 29..36 ore, docenti 5..24 ore): esce con 1 se no."""
import argparse, os, re, resource, shutil, subprocess, sys, tempfile, time
from concurrent.futures import ThreadPoolExecutor

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
        errori.append("dati non trovati nell'output")
    errori += ['classe con %d ore' % o for o in classi if not 29 <= o <= 36]
    errori += ['docente con %d ore' % o for o in docenti if not 5 <= o <= 24]
    return errori, len(classi), len(docenti)


def lancia(exe, seme, classi, timeout, cartella, backtrack=False, max_mb=4096):
    """Esegue un caso nella sua cartella. Ritorna un dizionario con l'esito."""
    os.makedirs(cartella, exist_ok=True)
    dati = os.path.join(os.path.dirname(cartella) if backtrack else cartella, 'dati.txt')
    cmd = [exe, '--casuale', str(seme), '--classi', str(classi), '-n', '1', '--dati', dati] + (['-b'] if backtrack else [])
    limite = max_mb * 2 ** 20 if max_mb > 0 else resource.RLIM_INFINITY
    t = time.time()
    try:
        r = subprocess.run(cmd, cwd=cartella, capture_output=True, text=True, timeout=timeout,
                           preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_FSIZE, (limite, limite)))
        out, finito = r.stdout, r.returncode == 0
        errore = r.returncode != 0
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode() if isinstance(e.stdout, bytes) else (e.stdout or '')
        finito, errore = False, False
    m = re.search(r'Totale generati: (\d+)', out)
    marce = re.search(r'Marce indietro: (\d+)', out)
    esito = 'non finito' if not finito else ('trovato' if m and int(m.group(1)) > 0 else 'nessuna soluzione')
    if errore:
        esito = 'interrotto (file troppo grande)' if backtrack else 'errore'
    errori, nc, nd = controlla_dati(out)
    return {'seme': seme, 'esito': esito, 'finito': finito, 'marce': int(marce.group(1)) if marce else None,
            'sec': time.time() - t, 'classi': nc, 'docenti': nd, 'errori': errori}


def stampa(fase, r):
    print('[%s] seme %4d: %2d classi, %3d docenti, %-18s marce %-9s %6.1fs %s' % (
        fase, r['seme'], r['classi'], r['docenti'], r['esito'], r['marce'] if r['marce'] is not None else '-',
        r['sec'], ('DATI NON VALIDI: ' + '; '.join(r['errori'])) if r['errori'] else ''), flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--da', type=int, default=1)
    ap.add_argument('--quanti', type=int, default=20)
    ap.add_argument('--classi', type=int, default=8)
    ap.add_argument('--prima', type=float, default=60, help='secondi massimi della fase 1')
    ap.add_argument('--dopo', type=float, default=600, help='secondi massimi della fase 2 (0 = salta)')
    ap.add_argument('--piu-lenti', type=int, default=3, help='quanti casi finiti con piu\' marce indietro rilanciare')
    ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2))
    ap.add_argument('--max-mb', type=int, default=4096,
                    help='dimensione massima di ogni orario.txt nella fase 3, in MB (0 = nessun limite)')
    ap.add_argument('--out', default=os.path.join(AQUI, 'risultati'))
    a = ap.parse_args()

    semi = list(range(a.da, a.da + a.quanti))
    shutil.rmtree(a.out, ignore_errors=True)
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(a.jobs) as pool:
        exe = os.path.join(tmp, 'solver')
        compila(exe)
        cart = lambda s: os.path.join(a.out, 'seme_%d' % s)  # un file/cartella per ogni caso
        storia = {s: [] for s in semi}  # esiti di ogni fase, per risultato.txt

        print('== Fase 1: al massimo %g s per caso, %d in parallelo' % (a.prima, a.jobs))
        ris = {}
        for r in pool.map(lambda s: lancia(exe, s, a.classi, a.prima, cart(s)), semi):
            ris[r['seme']] = r
            storia[r['seme']].append(('fase 1 (max %g s)' % a.prima, r))
            stampa('1', r)

        non_finiti = [s for s in semi if not ris[s]['finito']]
        if non_finiti and a.dopo > 0:
            print('\n== Fase 2: %d casi non finiti, al massimo %g s ciascuno' % (len(non_finiti), a.dopo))
            for r in pool.map(lambda s: lancia(exe, s, a.classi, a.dopo, cart(s)), non_finiti):
                ris[r['seme']] = r
                storia[r['seme']].append(('fase 2 (max %g s)' % a.dopo, r))
                stampa('2', r)

        ancora = [s for s in semi if not ris[s]['finito']]
        lenti = sorted((s for s in semi if ris[s]['finito'] and ris[s]['marce']), key=lambda s: -ris[s]['marce'])
        da_ristampare = ancora + lenti[:a.piu_lenti]
        if da_ristampare:
            print('\n== Fase 3: orario parziale a ogni marcia indietro per %d casi (%d non finiti, %d con piu\' marce '
                  'indietro)' % (len(da_ristampare), len(ancora), len(da_ristampare) - len(ancora)))
            tempo = max(a.prima, a.dopo)
            os.makedirs(a.out, exist_ok=True)
            libero = shutil.disk_usage(a.out).free / 2 ** 20
            massimo = len(da_ristampare) * a.max_mb if a.max_mb > 0 else float('inf')
            if massimo > libero:
                print('ATTENZIONE: spazio libero %.0f MB, i file potrebbero occupare fino a %s MB '
                      '(%d casi in parallelo scrivono insieme)' % (libero, '?' if a.max_mb <= 0 else int(massimo), a.jobs))
            for r in pool.map(lambda s: lancia(exe, s, a.classi, tempo, os.path.join(cart(s), 'marce_indietro'),
                                               backtrack=True, max_mb=a.max_mb), da_ristampare):
                stampa('3', r)
                storia[r['seme']].append(('fase 3, con -b (max %g s)' % tempo, r))
                p = os.path.join(cart(r['seme']), 'marce_indietro', 'orario.txt')
                if os.path.exists(p):
                    print('         -> %s (%.1f MB)' % (os.path.relpath(p, RADICE), os.path.getsize(p) / 1e6))

        for s in semi:  # un file di sintesi per ogni caso
            with open(os.path.join(cart(s), 'risultato.txt'), 'w') as fh:
                fh.write('Caso: --casuale %d --classi %d\n' % (s, a.classi))
                for nome, r in storia[s]:
                    fh.write('%s: %s, marce indietro %s, %.1f s\n' % (
                        nome, r['esito'], r['marce'] if r['marce'] is not None else 'non note', r['sec']))
                    if r['errori']:
                        fh.write('  DATI NON VALIDI: %s\n' % '; '.join(r['errori']))

        riep = {}
        for s in semi:
            riep[ris[s]['esito']] = riep.get(ris[s]['esito'], 0) + 1
        cattivi = sum(1 for s in semi if ris[s]['errori'])
        print('\nRiepilogo:', riep, '| dati non validi:', cattivi)
        sys.exit(1 if cattivi else 0)


if __name__ == '__main__':
    main()
