"""Il programma dell'orario (la cartella orario/), come lo usa il sito: generare un caso, e lanciare la ricerca di un
orario in un processo a parte (cosi' il sito resta libero, e la si puo' fermare)."""
import os
import subprocess
import sys

from orario import genera as _genera

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def genera(seme, classi, ritardi, file):
    """Scrive in `file` il caso casuale (seme, classi, con o senza ritardi). ValueError se non si riesce a
    generarlo."""
    testo = _genera.genera(seme, classi, ritardi)
    with open(file, 'w', encoding='utf-8') as f:
        f.write(testo)


def risolvi(caso, stato, tempo):
    """Lancia il programma sul caso e ritorna il processo: lo stato va nel file `stato` (subito «in corso», poi il
    risultato). `tempo`: i secondi che ha a disposizione."""
    comando = [sys.executable, '-m', 'orario', 'risolvi', caso, '--stato', stato, '--tempo', str(tempo)]
    return subprocess.Popen(comando, cwd=RADICE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
