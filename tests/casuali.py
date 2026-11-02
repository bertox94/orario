#!/usr/bin/env python3
"""Prova il solver su dati casuali, in tre fasi.

Uso: tests/casuali.py [--da SEME] [--quanti N] [--classi K]
  --da      primo seme (default 1)
  --quanti  quanti semi provare (default 20); ogni seme si prova due volte: normale e con docenti che entrano in
            ritardo (ora di ingresso 1, 2 o 3), quindi i casi sono il doppio
  --classi  quante classi ha ogni caso (default 8)

1. Fase 1: ogni caso gira al massimo 1 minuto (si ferma al primo orario trovato).
2. Fase 2: i casi non finiti girano di nuovo per al massimo 10 minuti.
3. Fase 3: i casi ancora non finiti vengono rilanciati con -b: il programma scrive in orario.txt l'orario parziale a
   ogni marcia indietro. Tutti i file delle marce indietro insieme occupano al massimo circa 20 GB (divisi in parti
   uguali tra i casi). I casi che hanno trovato un orario non si rilanciano.

Per ogni caso c'e' una cartella tests/risultati/seme_<N>/ (o seme_<N>_ritardi/) (cancellata a ogni esecuzione) con:
   dati.txt            descrizione del caso (classi, docenti, lezioni, codocenze)
   risultato.txt       esito di ogni fase, marce indietro, tempi, diagnosi
   orario.txt          il primo orario trovato (se la ricerca e' finita)
   marce_indietro/orario.txt   solo per i casi non finiti: orario parziale a ogni marcia indietro

Per ogni caso lento o non finito stampa una diagnosi: se i vicoli ciechi (marce indietro, o rami scartati dalla
potatura se sono di piu') sono concentrati in pochi slot (LOCALIZZATA) il caso e' probabilmente impossibile o ha un
blocco preciso; se sono sparsi (diffusa) e' solo difficile.

Controlla anche che i dati generati rispettino i vincoli (classi 29..36 ore, docenti 5..24 ore): esce con 1 se no."""
import argparse, os, re, resource, shutil, subprocess, sys, tempfile, time
from concurrent.futures import ThreadPoolExecutor

PRIMA = 60            # secondi massimi della fase 1
DOPO = 600            # secondi massimi della fase 2 (e della fase 3)
TOTALE_MARCE_MB = 20 * 1024  # spazio massimo di tutti i file delle marce indietro insieme
JOBS = max(1, (os.cpu_count() or 2) // 2)

AQUI = os.path.dirname(os.path.abspath(__file__))
RADICE = os.path.dirname(AQUI)
OUT = os.path.join(AQUI, 'risultati')
FILES = ['main.c', 'orario/orario.c', 'ricerca/ricerca.c', 'ricerca/vincoli.c', 'ricerca/diagnosi.c', 'ricerca/fattibilita.c',
         'data_initialization/database_initializers.c', 'data_initialization/generatore.c',
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


def diagnosi(out):
    """Legge la diagnosi stampata dal solver. Ritorna (riassunto breve, testo completo).

    Se le marce indietro sono quasi tutte in pochi slot (e la ricerca non avanza piu'), il problema e' localizzato:
    di solito il caso e' impossibile per costruzione, o la causa sta poco prima di quegli slot.
    Se sono sparse su molti slot, la ricerca e' solo difficile."""
    i = out.find('Diagnosi della ricerca')
    if i < 0:
        return '-', ''
    testo = out[i:].split('Totale generati')[0].strip()
    # i vicoli ciechi sono le marce indietro oppure i rami scartati dalla potatura: si guarda i piu' numerosi
    m = re.search(r'Marce indietro: (\d+) in (\d+) slot diversi', testo)
    s = re.search(r'Rami scartati: (\d+) dopo (\d+) slot diversi', testo)
    marce = int(m.group(1)) if m else 0
    rami = int(s.group(1)) if s else 0
    if not marce and not rami:
        return 'nessuna marcia indietro', testo
    if marce >= rami:
        cosa, n_slot = 'marce', m.group(2)
        quote = [float(x) for x in re.findall(r'marce \(([\d.]+)%\)', testo)]
        primo = re.search(r'Slot piu\' frequente 1: posizione \d+ \(giorno (\S+), ora (\S+), classe (\S+)\)', testo)
    else:
        cosa, n_slot = 'rami scartati', s.group(2)
        quote = [float(x) for x in re.findall(r'Rami scartati dopo lo slot \d: .*\(([\d.]+)%\)', testo)]
        primo = re.search(r'Rami scartati dopo lo slot 1: posizione \d+ \(giorno (\S+), ora (\S+), classe (\S+)\)', testo)
    ferma = re.search(r'raggiunta al nodo \d+ \(([\d.]+)% dei nodi\)', testo)
    top3 = sum(quote[:3])
    nome = '%s %s %s' % primo.groups() if primo else '?'
    stato = 'LOCALIZZATA' if top3 >= 80 else 'diffusa'
    senza_progressi = ferma and float(ferma.group(1)) < 5
    return '%s (%s): %s slot, top3 %.0f%%, piu\' frequente %s%s' % (
        stato, cosa, n_slot, top3, nome, ', senza avanzare' if senza_progressi else ''), testo


def lancia(exe, caso, classi, timeout, cartella, backtrack=False, max_mb=4096):
    """Esegue un caso nella sua cartella. Ritorna un dizionario con l'esito."""
    seme, ritardi = caso
    os.makedirs(cartella, exist_ok=True)
    dati = os.path.join(os.path.dirname(cartella) if backtrack else cartella, 'dati.txt')
    cmd = [exe, '--casuale', str(seme), '--classi', str(classi), '-n', '1', '--dati', dati,
           '--tempo', str(max(timeout - 1, 0.5)), '--motivi'] + (['--ritardi'] if ritardi else []) + \
          (['-b'] if backtrack else [])
    limite = max_mb * 2 ** 20 if max_mb > 0 else resource.RLIM_INFINITY
    t = time.time()
    try:
        r = subprocess.run(cmd, cwd=cartella, capture_output=True, text=True, timeout=timeout + 30,
                           preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_FSIZE, (limite, limite)))
        out, finito = r.stdout, r.returncode == 0
        errore = r.returncode != 0
    except subprocess.TimeoutExpired as e:
        out = e.stdout.decode() if isinstance(e.stdout, bytes) else (e.stdout or '')
        finito, errore = False, False
    impossibile = re.search(r'Impossibile \(verifica preliminare\): ([^\n]*)', out)
    interrotto = 'Interrotto per tempo' in out
    finito = finito and not interrotto
    m = re.search(r'Totale generati: (\d+)', out)
    marce = re.search(r'Marce indietro: (\d+)', out)
    esito = 'non finito' if not finito else ('trovato' if m and int(m.group(1)) > 0 else 'nessuna soluzione')
    if impossibile:
        esito, finito = 'impossibile', True
    elif errore:
        esito = 'interrotto (file troppo grande)' if backtrack else 'errore'
    errori, nc, nd = controlla_dati(out)
    if impossibile:
        d = ('impossibile: ' + impossibile.group(1), 'Impossibile (verifica preliminare): ' + impossibile.group(1))
    else:
        d = diagnosi(out)
    return {'caso': caso, 'esito': esito, 'finito': finito, 'marce': int(marce.group(1)) if marce else None,
            'sec': time.time() - t, 'classi': nc, 'docenti': nd, 'errori': errori, 'diagnosi': d[0],
            'dettaglio': d[1]}


def stampa(fase, r):
    print('[%s] %-15s: %2d classi, %3d docenti, %-18s marce %-9s %6.1fs %s' % (
        fase, nome_caso(r['caso']), r['classi'], r['docenti'], r['esito'], r['marce'] if r['marce'] is not None else '-',
        r['sec'], ('DATI NON VALIDI: ' + '; '.join(r['errori'])) if r['errori'] else ''), flush=True)
    if r['esito'] in ('non finito', 'impossibile') or (r['marce'] or 0) > 1000:
        print('         diagnosi:', r['diagnosi'], flush=True)


def nome_caso(caso):
    seme, ritardi = caso
    return 'seme %d%s' % (seme, ' ritardi' if ritardi else '')


def cartella_caso(caso):
    seme, ritardi = caso
    return os.path.join(OUT, 'seme_%d%s' % (seme, '_ritardi' if ritardi else ''))


def fase1(pool, exe, casi, classi, ris, storia):
    print('== Fase 1: al massimo %g s per caso, %d in parallelo' % (PRIMA, JOBS))
    for r in pool.map(lambda s: lancia(exe, s, classi, PRIMA, cartella_caso(s)), casi):
        ris[r['caso']] = r
        storia[r['caso']].append(('fase 1 (max %g s)' % PRIMA, r))
        stampa('1', r)


def fase2(pool, exe, casi, classi, ris, storia):
    non_finiti = [s for s in casi if not ris[s]['finito']]
    if not non_finiti:
        return
    print('\n== Fase 2: %d casi non finiti, al massimo %g s ciascuno' % (len(non_finiti), DOPO))
    for r in pool.map(lambda s: lancia(exe, s, classi, DOPO, cartella_caso(s)), non_finiti):
        ris[r['caso']] = r
        storia[r['caso']].append(('fase 2 (max %g s)' % DOPO, r))
        stampa('2', r)


def fase3(pool, exe, casi, classi, ris, storia):
    """Rilancia con -b i casi ancora non finiti. Lo spazio TOTALE_MARCE_MB si divide in parti uguali tra i casi."""
    da_ristampare = [s for s in casi if not ris[s]['finito']]
    if not da_ristampare:
        return
    per_file_mb = max(1, TOTALE_MARCE_MB // len(da_ristampare))
    print('\n== Fase 3: orario parziale a ogni marcia indietro per i %d casi non finiti (al massimo %d MB ciascuno, '
          '%d MB in tutto)' % (len(da_ristampare), per_file_mb, per_file_mb * len(da_ristampare)))
    os.makedirs(OUT, exist_ok=True)
    libero = shutil.disk_usage(OUT).free / 2 ** 20
    if per_file_mb * len(da_ristampare) > libero:
        print('ATTENZIONE: spazio libero %.0f MB, meno di quello che i file possono occupare' % libero)
    for r in pool.map(lambda s: lancia(exe, s, classi, DOPO, os.path.join(cartella_caso(s), 'marce_indietro'),
                                       backtrack=True, max_mb=per_file_mb), da_ristampare):
        stampa('3', r)
        storia[r['caso']].append(('fase 3, con -b (max %g s)' % DOPO, r))
        p = os.path.join(cartella_caso(r['caso']), 'marce_indietro', 'orario.txt')
        if os.path.exists(p):
            print('         -> %s (%.1f MB)' % (os.path.relpath(p, RADICE), os.path.getsize(p) / 1e6))


def scrivi_risultati(casi, classi, storia):
    """Un file di sintesi per ogni caso."""
    for s in casi:
        with open(os.path.join(cartella_caso(s), 'risultato.txt'), 'w') as fh:
            fh.write('Caso: --casuale %d --classi %d%s\n' % (s[0], classi, ' --ritardi' if s[1] else ''))
            for nome, r in storia[s]:
                fh.write('%s: %s, marce indietro %s, %.1f s\n' % (
                    nome, r['esito'], r['marce'] if r['marce'] is not None else 'non note', r['sec']))
                if r['dettaglio']:
                    fh.write('  ' + r['dettaglio'].replace('\n', '\n  ') + '\n')
                if r['errori']:
                    fh.write('  DATI NON VALIDI: %s\n' % '; '.join(r['errori']))


def riepilogo(casi, ris):
    esiti = {}
    for s in casi:
        esiti[ris[s]['esito']] = esiti.get(ris[s]['esito'], 0) + 1
    cattivi = sum(1 for s in casi if ris[s]['errori'])
    print('\nRiepilogo:', esiti, '| dati non validi:', cattivi)
    return cattivi


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--da', type=int, default=1, help='primo seme')
    ap.add_argument('--quanti', type=int, default=20, help='quanti casi provare')
    ap.add_argument('--classi', type=int, default=8, help='quante classi ha ogni caso')
    a = ap.parse_args()

    casi = [(s, ritardi) for s in range(a.da, a.da + a.quanti) for ritardi in (False, True)]
    shutil.rmtree(OUT, ignore_errors=True)
    ris, storia = {}, {s: [] for s in casi}  # esito dell'ultima fase e storia di tutte le fasi
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(JOBS) as pool:
        exe = os.path.join(tmp, 'solver')
        compila(exe)
        fase1(pool, exe, casi, a.classi, ris, storia)
        fase2(pool, exe, casi, a.classi, ris, storia)
        fase3(pool, exe, casi, a.classi, ris, storia)
        scrivi_risultati(casi, a.classi, storia)
        sys.exit(1 if riepilogo(casi, ris) else 0)


if __name__ == '__main__':
    main()
