"""Il programma in C (le cartelle scuola, generatore, solver, verifica, passi, spiegazioni, programma): il sito lo
compila da solo la prima volta e quando i sorgenti cambiano, e lo lancia. I comandi sono in programma/main.c."""
import hashlib
import importlib.util
import json
import os
import subprocess
import sys

from . import archivio

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# le cartelle del programma in C (come nel Makefile): la scuola, il generatore, la ricerca, la verifica, i passi, le
# spiegazioni per il sito, e i comandi
CARTELLE = ('scuola', 'generatore', 'solver', 'verifica', 'passi', 'spiegazioni', 'programma')


def programma():
    """Il percorso dell'eseguibile, compilato se manca: il suo nome dipende dai sorgenti."""
    nomi = sorted(os.path.join(c, n) for c in CARTELLE for n in os.listdir(os.path.join(RADICE, c))
                  if n.endswith(('.c', '.h')))
    # l'impronta di tutti i sorgenti: se anche uno solo cambia, cambia il nome dell'eseguibile, e si ricompila
    impronta = hashlib.sha256()
    for n in nomi:
        with open(os.path.join(RADICE, n), 'rb') as f:
            impronta.update(n.encode() + f.read())
    exe = archivio.percorso('bin', 'orario-' + impronta.hexdigest()[:12])
    if not os.path.exists(exe):
        os.makedirs(os.path.dirname(exe), exist_ok=True)
        # tutti i .c insieme, senza avvisi: quelli si guardano con make, che li mostra
        r = subprocess.run(['gcc', '-O2', '-std=gnu11', '-w'] + [n for n in nomi if n.endswith('.c')]
                           + ['-o', exe + '.tmp'], cwd=RADICE, capture_output=True, text=True)
        if r.returncode:
            raise RuntimeError('il solver non compila:\n' + r.stderr[-2000:])
        os.replace(exe + '.tmp', exe)   # compilato a parte e poi rinominato: non si lancia mai un eseguibile a meta'
    return exe


def genera(seme, classi, ritardi, file):
    """Scrive in `file` il caso casuale (seme, classi, con o senza ritardi). ValueError se non si riesce a
    generarlo."""
    comando = [programma(), 'genera', str(seme), str(classi), '1' if ritardi else '0']
    # il caso esce su stdout: va dritto nel file
    with open(file, 'w') as f:
        if subprocess.run(comando, stdout=f).returncode:
            raise ValueError('Con questo seme e queste classi il caso non si riesce a generare.')


SEGNI = (None, 'fatta', 'puo', 'no', 'serve')   # i numeri dei segni (spiegazioni/spiega.h, SEGNO_)


def nodo(caso, nodo):
    """Quello che il solver dice di un nodo (programma/main.c, il comando «nodo»): le celle di cui parla il conto che
    non torna, il conto in dettaglio, e i conti di ogni docente:
    {"giorno", "ora", "segni": [[cella, nome], ...], "conto": {...}, "docenti": [...]}.
    `nodo` e' il testo di passi.nodo_per_il_solver. None se il solver non lo accetta."""
    r = subprocess.run([programma(), 'nodo', caso], input=nodo, capture_output=True, text=True)
    if r.returncode:
        return None
    x = json.loads(r.stdout)
    return dict(x, segni=[[cella, SEGNI[segno]] for cella, segno in x['segni']])


def scelte(caso, strada):
    """Le scelte che la ricerca ha in fondo a una strada (il testo di passi.strada_per_il_solver):
    {"cella": quella che li' si decide, "scelte": [voce | -1, ...]} nell'ordine in cui le proverebbe. None se la
    strada non e' possibile."""
    r = subprocess.run([programma(), 'scelte', caso], input=strada, capture_output=True, text=True)
    return None if r.returncode else json.loads(r.stdout)


def esatto_disponibile():
    """Il metodo esatto (la cartella esatto/) ha bisogno di scipy: c'e'?"""
    return importlib.util.find_spec('scipy') is not None


def esatto(caso, stato, tempo, cartella):
    """Lancia il metodo esatto sul caso (esatto/, un altro modo di fare l'orario: tutte le regole insieme, come un
    sistema di conti) e ritorna il processo. Scrive lo stato nel file `stato`, nello stesso formato della ricerca,
    con "metodo": "esatto"; niente passi. `caso` e `stato` sono nomi di file dentro `cartella`."""
    comando = [sys.executable, '-m', 'esatto', os.path.join(cartella, caso), '--programma', programma(),
               '--stato', os.path.join(cartella, stato), '--tempo', str(tempo)]
    return subprocess.Popen(comando, cwd=RADICE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def risolvi(caso, limite, stato, passi=None, cartella=None, continua=False, da=None):
    """Lancia la ricerca sul caso e ritorna il processo: lo stato va nel file `stato`, i passi (se chiesti) in
    `passi`. Con `continua` riprende la ricerca che ha scritto quei passi, fermata dal limite di nodi, e va avanti
    fino al limite nuovo. Con `da` (un file con una strada) la ricerca parte da quella strada, scelta a mano."""
    comando = [programma(), 'risolvi', caso, '--limite', str(limite), '--stato', stato]
    return subprocess.Popen(comando + (['--passi', passi] if passi else []) + (['--continua'] if continua else [])
                            + (['--da', da] if da else []), cwd=cartella,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
