"""Il programma dell'orario (la cartella orario/), come lo usa il sito. Due sole cose: generare un caso, e lanciare la
ricerca del suo orario in un processo a parte (cosi' il sito resta libero mentre il programma lavora, e lo si puo'
fermare chiudendo il processo). Le usano sia le prove sia i test notturni."""
import os
import subprocess
import sys

from orario import genera as _genera

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # la cartella dove sta orario/


def genera(seme, classi, ritardi, file):
    """Scrive in `file` il caso casuale (seme, classi, con o senza ritardi). ValueError, con il messaggio da
    mostrare, se i numeri non vanno o il caso non si riesce a generare."""
    testo = _genera.genera(seme, classi, ritardi)
    with open(file, 'w', encoding='utf-8') as f:
        f.write(testo)


def risolvi(caso, stato, tempo):
    """Lancia il programma sul caso e ritorna il processo. Lo stato va nel file `stato`: subito «in corso», poi il
    risultato (vedi orario/__main__.py). `tempo`: i secondi che ha a disposizione."""
    comando = [sys.executable, '-m', 'orario', 'risolvi', caso, '--stato', stato, '--tempo', str(tempo)]
    return subprocess.Popen(comando, cwd=RADICE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def vivo(processo):
    """Il processo c'e' e sta ancora girando."""
    return processo is not None and processo.poll() is None
