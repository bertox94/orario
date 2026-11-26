"""Il comando del metodo esatto:

    python3 -m esatto CASO --programma ORARIO [--stato FILE] [--tempo SECONDI]

CASO e' un file come quelli che legge il programma in C; ORARIO e' il programma in C compilato. Scrive lo stato, in
JSON, nello stesso formato di «orario risolvi» (programma/stato.h), con in piu' "metodo": "esatto": su stdout, o in
FILE (subito «in corso», poi il risultato). Gli esiti: «trovato» (con l'orario e la sua verifica, fatta dal programma
in C), «impossibile» (dimostrato: un orario non esiste), «limite di tempo». Non ci sono nodi ne' ritorni.
"""
import argparse
import json
import os
import subprocess
import sys
import time

from . import modello


def scrivi(stato, file):
    """Lo stato su stdout, o nel file: a parte e poi rinominato, cosi' chi legge trova sempre un file intero."""
    if not file:
        json.dump(stato, sys.stdout)
        print()
        return
    with open(file + '.tmp', 'w') as f:
        json.dump(stato, f)
    os.replace(file + '.tmp', file)


def main():
    ap = argparse.ArgumentParser(prog='python3 -m esatto', description='L\'orario con il metodo esatto.')
    ap.add_argument('caso')
    ap.add_argument('--programma', required=True, help='il programma in C compilato (per la scuola e la verifica)')
    ap.add_argument('--stato', help='il file dove scrivere lo stato (se manca: su stdout, alla fine)')
    ap.add_argument('--tempo', type=float, default=300, help='dopo tanti secondi si ferma (300)')
    a = ap.parse_args()
    # la scuola come la vede il programma in C: stessi numeri per docenti, voci e celle
    r = subprocess.run([a.programma, 'scuola', a.caso], capture_output=True, text=True)
    if r.returncode:
        sys.stderr.write(r.stderr)
        return 2
    caso = json.loads(r.stdout)
    partenza = time.time()
    stato = {'metodo': 'esatto', 'a_mano': 0, 'in_corso': 1, 'esito': 'in corso', 'nodi': 0, 'secondi': 0,
             'profondita_massima': 0, 'celle': caso['celle'], 'scuola': caso['scuola']}
    if a.stato:
        scrivi(stato, a.stato)
    esito, orario, _ = modello.risolvi(caso['scuola'], a.tempo)
    stato.update(in_corso=0, esito=esito, secondi=round(time.time() - partenza, 2))
    if orario:
        # l'orario trovato lo ricontrolla il programma in C, regola per regola
        v = subprocess.run([a.programma, 'verifica', a.caso], input=' '.join(map(str, orario)), capture_output=True,
                           text=True)
        if v.returncode:
            sys.stderr.write('la verifica non accetta l\'orario trovato\n')
            return 2
        stato.update(json.loads(v.stdout), profondita_massima=caso['celle'])
    scrivi(stato, a.stato)
    return 0


if __name__ == '__main__':
    sys.exit(main())
