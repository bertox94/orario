"""I passi della ricerca scritti dal solver (./untitled caso ... PASSI, vedi passi.h): quanti sono e l'orario a un nodo
qualunque, ricostruito dall'istantanea prima e dai record dopo (al massimo ISTANTANEA)."""
import array
import os

ISTANTANEA = 16384  # PASSI_ISTANTANEA in passi.h
RECORD = 6          # tre interi a 16 bit


def conta(percorso):
    """(nodi scritti, byte delle istantanee): mentre la ricerca gira i file crescono."""
    try:
        return os.path.getsize(percorso) // RECORD, os.path.getsize(percorso + '.ist')
    except OSError:
        return 0, 0


def nodo(percorso, k, celle):
    """Il nodo k (da 1): (pos, pos del nodo prima o None, orario), con orario = [[docente, codocente], ...] per le celle
    prima di pos, gia' decise (indici dei docenti, -1 nessuno). None se il nodo non c'e' (ancora)."""
    n, ist = conta(percorso)
    if not 1 <= k <= n:
        return None
    j = min(k // ISTANTANEA, ist // (4 * celle))  # l'istantanea j e' l'orario dopo il nodo j * ISTANTANEA
    if j:
        with open(percorso + '.ist', 'rb') as f:
            f.seek((j - 1) * 4 * celle)
            orario = array.array('h')
            orario.frombytes(f.read(4 * celle))
    else:
        orario = array.array('h', [-1] * (2 * celle))
    primo = j * ISTANTANEA + 1          # il primo record da applicare
    da = min(primo, max(1, k - 1))      # si legge anche il nodo prima, per la sua posizione
    with open(percorso, 'rb') as f:
        f.seek((da - 1) * RECORD)
        r = array.array('h')
        r.frombytes(f.read((k - da + 1) * RECORD))
    for i in range(primo - da, k - da + 1):
        pos, a, b = r[3 * i], r[3 * i + 1], r[3 * i + 2]
        if pos > 0:
            orario[2 * (pos - 1)], orario[2 * (pos - 1) + 1] = a, b
    pos = r[3 * (k - da)]
    prima = r[3 * (k - 1 - da)] if k > 1 else None
    return pos, prima, [[orario[2 * i], orario[2 * i + 1]] for i in range(pos)]
