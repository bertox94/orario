"""Il solver (la cartella solver/): il sito lo compila da solo la prima volta e quando i sorgenti cambiano, e lo lancia.
I comandi sono in solver/main.c."""
import hashlib
import os
import subprocess

from . import archivio

SORGENTI = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'solver')


def programma():
    """Il percorso dell'eseguibile, compilato se manca: il suo nome dipende dai sorgenti."""
    nomi = sorted(n for n in os.listdir(SORGENTI) if n.endswith(('.c', '.h')))
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
        os.replace(exe + '.tmp', exe)
    return exe


def genera(seme, classi, ritardi, file):
    """Scrive in `file` il caso casuale (seme, classi, con o senza ritardi). ValueError se non si riesce a
    generarlo."""
    comando = [programma(), 'genera', str(seme), str(classi), '1' if ritardi else '0']
    with open(file, 'w') as f:
        if subprocess.run(comando, stdout=f).returncode:
            raise ValueError('Con questo seme e queste classi il caso non si riesce a generare.')


def risolvi(caso, limite, stato, passi=None, cartella=None):
    """Lancia la ricerca sul caso e ritorna il processo: lo stato va nel file `stato`, i passi (se chiesti) in
    `passi`."""
    comando = [programma(), 'risolvi', caso, '--limite', str(limite), '--stato', stato]
    return subprocess.Popen(comando + (['--passi', passi] if passi else []), cwd=cartella,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
