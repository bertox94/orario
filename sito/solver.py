"""Il solver (la cartella solver/): il sito lo compila da solo la prima volta e quando i sorgenti cambiano, e lo lancia.
I comandi sono in solver/main.c."""
import hashlib
import json
import os
import subprocess

from . import archivio

SORGENTI = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'solver')


def programma():
    """Il percorso dell'eseguibile, compilato se manca: il suo nome dipende dai sorgenti."""
    nomi = sorted(n for n in os.listdir(SORGENTI) if n.endswith(('.c', '.h')))
    # l'impronta di tutti i sorgenti: se anche uno solo cambia, cambia il nome dell'eseguibile, e si ricompila
    impronta = hashlib.sha256()
    for n in nomi:
        with open(os.path.join(SORGENTI, n), 'rb') as f:
            impronta.update(n.encode() + f.read())
    exe = archivio.percorso('bin', 'orario-' + impronta.hexdigest()[:12])
    if not os.path.exists(exe):
        os.makedirs(os.path.dirname(exe), exist_ok=True)
        r = subprocess.run(['gcc', '-O2', '-std=gnu11', '-w'] + [n for n in nomi if n.endswith('.c')]
                           + ['-o', exe + '.tmp'], cwd=SORGENTI, capture_output=True, text=True)
        if r.returncode:
            raise RuntimeError('il solver non compila:\n' + r.stderr[-2000:])
        os.replace(exe + '.tmp', exe)   # compilato a parte e poi rinominato: non si lancia mai un eseguibile a meta'
    return exe


def genera(seme, classi, ritardi, file):
    """Scrive in `file` il caso casuale (seme, classi, con o senza ritardi). ValueError se non si riesce a
    generarlo."""
    comando = [programma(), 'genera', str(seme), str(classi), '1' if ritardi else '0']
    with open(file, 'w') as f:
        if subprocess.run(comando, stdout=f).returncode:
            raise ValueError('Con questo seme e queste classi il caso non si riesce a generare.')


SEGNI = (None, 'fatta', 'puo', 'no', 'serve')   # i numeri dei segni (solver/ricerca.h, SEGNO_)


def nodo(caso, nodo):
    """Quello che il solver dice di un nodo (solver/main.c, il comando «nodo»): le celle di cui parla il conto che
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


def risolvi(caso, limite, stato, passi=None, cartella=None, continua=False, da=None):
    """Lancia la ricerca sul caso e ritorna il processo: lo stato va nel file `stato`, i passi (se chiesti) in
    `passi`. Con `continua` riprende la ricerca che ha scritto quei passi, fermata dal limite di nodi, e va avanti
    fino al limite nuovo. Con `da` (un file con una strada) la ricerca parte da quella strada, scelta a mano."""
    comando = [programma(), 'risolvi', caso, '--limite', str(limite), '--stato', stato]
    return subprocess.Popen(comando + (['--passi', passi] if passi else []) + (['--continua'] if continua else [])
                            + (['--da', da] if da else []), cwd=cartella,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
