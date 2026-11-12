"""Il solver: si compila da solo (gcc) la prima volta e quando cambiano i sorgenti; un test lo lancia con
./untitled caso SEME CLASSI RITARDI LIMITE FILE (vedi caso.h nella radice)."""
import hashlib
import os
import subprocess

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CARTELLE = ['.', 'ricerca', 'ricerca/coda', 'dati', 'verifiche', 'comandi']  # dove stanno i sorgenti del solver


def _sorgenti():
    for d in CARTELLE:
        for nome in sorted(os.listdir(os.path.join(RADICE, d))):
            if nome.endswith(('.c', '.h')):
                yield os.path.join(d, nome)


def compila(cartella_bin):
    """Il percorso dell'eseguibile, compilato se manca (il nome dipende dai sorgenti)."""
    h = hashlib.sha256()
    for s in _sorgenti():
        h.update(s.encode())
        with open(os.path.join(RADICE, s), 'rb') as f:
            h.update(f.read())
    os.makedirs(cartella_bin, exist_ok=True)
    exe = os.path.join(cartella_bin, 'untitled-' + h.hexdigest()[:12])
    if os.path.exists(exe):
        return exe
    c = [s for s in _sorgenti() if s.endswith('.c')]
    r = subprocess.run(['gcc', '-O2', '-std=gnu11', '-w'] + c + ['-o', exe + '.tmp', '-lm'], cwd=RADICE,
                       capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError('il solver non compila:\n' + r.stderr[-2000:])
    os.replace(exe + '.tmp', exe)
    return exe
