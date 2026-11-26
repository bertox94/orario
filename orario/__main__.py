"""I comandi del programma dell'orario:

    python3 -m orario genera SEME CLASSI RITARDI
        scrive su stdout un caso casuale (genera.py); RITARDI: 1 con i docenti che entrano tardi, 0 senza
    python3 -m orario risolvi CASO [--stato FILE] [--tempo SECONDI]
        cerca un orario per il caso (esatto.py) e ne scrive lo stato, in JSON: su stdout alla fine, oppure in FILE
        (subito «in corso», poi il risultato; scritto a parte e poi rinominato: chi legge trova sempre un JSON intero)
    python3 -m orario verifica CASO
        ricontrolla un orario fatto da altri (verifica.py): su stdin una voce per cella (-1: ora buca), nell'ordine
        delle celle; scrive {"orario": [...], "verifica": {...}}

Lo stato:
    {"in_corso": 0 | 1, "esito": "in corso" | "trovato" | "impossibile" | "limite di tempo",
     "secondi": S, "celle": C, "scuola": {...}                      (scuola.py)
     "orario": [voce | -1, ...]                                     con «trovato»: una voce per cella
     "verifica": {"ok": 0 | 1, "controlli": [[regola, 0 | 1, dove], ...]}}   con «trovato» (verifica.py)

Esce con 0, o con 2 se il comando non va o il caso non si legge o non si genera.
"""
import json
import os
import sys
import time

from . import genera, scuola, verifica

USO = 'Uso:\n  python3 -m orario genera SEME CLASSI RITARDI\n  python3 -m orario risolvi CASO [--stato FILE] ' \
      '[--tempo SECONDI]\n  python3 -m orario verifica CASO < ORARIO\n'


def _scrivi(stato, file):
    """Lo stato su stdout, o nel file: a parte e poi rinominato."""
    if not file:
        json.dump(stato, sys.stdout)
        print()
        return
    with open(file + '.tmp', 'w') as f:
        json.dump(stato, f)
    os.replace(file + '.tmp', file)


def _risolvi(argomenti):
    from . import esatto   # (qui, non in cima: carica scipy, che agli altri comandi non serve)
    file_stato, tempo = None, 300.0
    # le opzioni: ognuna con il suo valore dopo
    for i in range(1, len(argomenti), 2):
        if i + 1 == len(argomenti) or argomenti[i] not in ('--stato', '--tempo'):
            raise ValueError(USO)
        if argomenti[i] == '--stato':
            file_stato = argomenti[i + 1]
        else:
            tempo = float(argomenti[i + 1])
    s = scuola.leggi(argomenti[0])
    partenza = time.time()
    stato = {'in_corso': 1, 'esito': 'in corso', 'secondi': 0, 'celle': scuola.celle(s), 'scuola': s}
    if file_stato:
        _scrivi(stato, file_stato)   # subito uno stato «in corso»: il sito ha gia' la scuola da mostrare
    esito, orario, _ = esatto.risolvi(s, tempo)
    stato.update(in_corso=0, esito=esito, secondi=round(time.time() - partenza, 2))
    if orario:
        # l'orario trovato si ricontrolla subito, regola per regola
        stato.update(orario=orario, verifica=verifica.verifica(s, orario))
    _scrivi(stato, file_stato)


def main(argomenti):
    try:
        if len(argomenti) == 4 and argomenti[0] == 'genera':
            sys.stdout.write(genera.genera(int(argomenti[1]), int(argomenti[2]), argomenti[3] not in ('0', '')))
        elif len(argomenti) >= 2 and argomenti[0] == 'risolvi':
            _risolvi(argomenti[1:])
        elif len(argomenti) == 2 and argomenti[0] == 'verifica':
            s = scuola.leggi(argomenti[1])
            orario = [int(x) for x in sys.stdin.read().split()]
            if len(orario) != scuola.celle(s) or max(orario) >= len(s['voci']):
                raise ValueError('quello che arriva non e\' un orario di questo caso')
            json.dump({'orario': orario, 'verifica': verifica.verifica(s, orario)}, sys.stdout)
            print()
        else:
            raise ValueError(USO)
    except ValueError as e:
        sys.stderr.write(str(e).rstrip('\n') + '\n')
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
