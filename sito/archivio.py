"""Dove il sito tiene le sue cose: la cartella di lavoro (sito/lavoro, o quella di SITO_LAVORO) e i suoi file JSON."""
import json
import os

# la cartella di lavoro: quella della variabile SITO_LAVORO, se c'e'; se no «lavoro», accanto a questo file
LAVORO = os.environ.get('SITO_LAVORO') or os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lavoro')


def percorso(*pezzi):
    return os.path.join(LAVORO, *[str(p) for p in pezzi])


def leggi(file, senza=None):
    """Il contenuto di un file JSON, o `senza` se manca o non si legge."""
    try:
        with open(file) as f:
            return json.load(f)
    except (OSError, ValueError):
        return senza


def scrivi(file, valore):
    """Scrive a parte e poi rinomina: chi legge trova sempre un file intero."""
    os.makedirs(os.path.dirname(file), exist_ok=True)
    with open(file + '.tmp', 'w') as f:
        json.dump(valore, f)
    os.replace(file + '.tmp', file)


def numeri(cartella):
    """I numeri delle cose (prove, giri) che stanno in una cartella, in ordine: file «3.json» o cartelle «3»."""
    try:
        nomi = os.listdir(cartella)
    except OSError:
        return []
    # il numero e' quello che c'e' prima del punto; i file provvisori (.tmp) non contano
    return sorted(int(n.split('.')[0]) for n in nomi if n.split('.')[0].isdigit() and not n.endswith('.tmp'))
