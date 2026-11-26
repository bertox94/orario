"""I comandi del programma dell'orario:

    python3 -m orario genera SEME CLASSI RITARDI
        scrive su stdout un caso casuale (genera.py); RITARDI: 1 con i docenti che entrano tardi, 0 senza
    python3 -m orario risolvi CASO [--stato FILE] [--tempo SECONDI]
        cerca un orario per il caso (esatto.py) e ne scrive lo stato, in JSON: su stdout alla fine, oppure in FILE
        (subito «in corso», poi il risultato; scritto a parte e poi rinominato: chi legge trova sempre un JSON intero)
    python3 -m orario verifica CASO
        ricontrolla un orario fatto da altri (verifica.py): su stdin una voce per cella (-1: ora buca), nell'ordine
        delle celle, e poi un'aula per cella (-1: ora buca); scrive {"orario": [...], "aule": [...], "verifica": {...}}

Lo stato:
    {"in_corso": 0 | 1, "esito": "in corso" | "trovato" | "impossibile" | "limite di tempo",
     "secondi": S, "celle": C, "scuola": {...}                      (scuola.py)
     "orario": [voce | -1, ...]                                     con «trovato»: una voce per cella
     "aule": [aula | -1, ...]                                       con «trovato»: l'aula di ogni cella
     "verifica": {"ok": 0 | 1, "controlli": [[regola, 0 | 1, dove], ...]}}   con «trovato» (verifica.py)

Esce con 0, o con 2 se il comando non va o il caso non si legge o non si genera.
"""
import argparse
import json
import os
import sys
import time

from . import genera, scuola, verifica


def _scrivi(stato, file=None):
    """Lo stato su stdout, o nel file: scritto a parte e poi rinominato."""
    if not file:
        print(json.dumps(stato))
        return
    with open(file + '.tmp', 'w') as f:
        json.dump(stato, f)
    os.replace(file + '.tmp', file)


def _risolvi(caso, file_stato, tempo):
    """Il comando «risolvi»: legge il caso, cerca l'orario, lo verifica, scrive lo stato."""
    from . import esatto   # (qui, non in cima: carica scipy, che agli altri comandi non serve)
    # 1. il caso
    s = scuola.leggi(caso)
    partenza = time.time()
    stato = {'in_corso': 1, 'esito': 'in corso', 'secondi': 0, 'celle': scuola.celle(s), 'scuola': s}
    # 2. subito uno stato «in corso»: chi guarda il file (il sito) ha gia' la scuola da mostrare
    if file_stato:
        _scrivi(stato, file_stato)
    # 3. l'orario
    esito, orario, aule, _ = esatto.risolvi(s, tempo)
    stato.update(in_corso=0, esito=esito, secondi=round(time.time() - partenza, 2))
    # 4. l'orario trovato si ricontrolla subito, regola per regola
    if orario:
        stato.update(orario=orario, aule=aule, verifica=verifica.verifica(s, orario, aule))
    # 5. lo stato finale
    _scrivi(stato, file_stato)


def _verifica(caso):
    """Il comando «verifica»: l'orario arriva su stdin: una voce per cella, poi un'aula per cella."""
    s = scuola.leggi(caso)
    numeri, n = [int(x) for x in sys.stdin.read().split()], scuola.celle(s)
    orario, aule = numeri[:n], numeri[n:]
    if len(numeri) != 2 * n or max(orario) >= len(s['voci']):
        raise ValueError('quello che arriva non e\' un orario di questo caso, con le sue aule')
    _scrivi({'orario': orario, 'aule': aule, 'verifica': verifica.verifica(s, orario, aule)})


def main(argomenti=None):
    # i tre comandi e i loro argomenti (un comando scritto male: argparse lo dice ed esce con 2)
    ap = argparse.ArgumentParser(prog='python3 -m orario', description='Il programma dell\'orario scolastico.')
    comandi = ap.add_subparsers(dest='comando', required=True)
    g = comandi.add_parser('genera', help='scrive un caso casuale')
    g.add_argument('seme', type=int)
    g.add_argument('classi', type=int)
    g.add_argument('ritardi', type=int, help='1: con i docenti che entrano tardi; 0: senza')
    r = comandi.add_parser('risolvi', help='cerca un orario per un caso')
    r.add_argument('caso')
    r.add_argument('--stato', help='il file dove scrivere lo stato (senza: su stdout, alla fine)')
    r.add_argument('--tempo', type=float, default=300, help='i secondi a disposizione')
    v = comandi.add_parser('verifica', help='ricontrolla un orario, letto da stdin')
    v.add_argument('caso')
    a = ap.parse_args(argomenti)
    try:
        if a.comando == 'genera':
            sys.stdout.write(genera.genera(a.seme, a.classi, bool(a.ritardi)))
        elif a.comando == 'risolvi':
            _risolvi(a.caso, a.stato, a.tempo)
        else:
            _verifica(a.caso)
    except ValueError as e:   # il caso non si legge o non si genera, o l'orario non e' di quel caso
        sys.stderr.write('%s\n' % e)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
