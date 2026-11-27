"""Il secondo metodo che fa l'orario: lo stesso caso chiesto a un altro risolutore, CP-SAT di OR-Tools
(pip install ortools). Le regole sono le otto di scuola.py, le stesse di esatto.py; cambia chi le risolve.

CP-SAT e' una ricerca che impara dai vicoli ciechi: fissa delle scelte, ne tira le conseguenze, e quando arriva a una
contraddizione ricava quali poche scelte l'hanno causata e non le rifa' piu' insieme. HiGHS (esatto.py) invece fa i
conti su tutta la settimana insieme. I due inciampano su casi diversi: per questo girano tutti e due (insieme.py).
Qui le regole si scrivono in modo piu' diretto che in esatto.py:
- «esattamente uno», «al massimo uno», «se questo allora quello» sono richieste gia' pronte;
- la giornata di un docente (regola 6) e' una tabella: l'elenco delle giornate consentite, e la sua deve essere una
  di quelle.

Misurato il 27 novembre 2026 sul Raspberry, 480 casi con 60 secondi: risponde in 472 (HiGHS, su una macchina piu'
veloce, in 476), mai in contraddizione con HiGHS; insieme rispondono in 479.

Le incognite, tutte si'/no:
- x[v, g, h]      la voce v e' nell'ora h del giorno g (solo dove i suoi docenti sono a scuola: le altre non esistono);
- b[k, g, h]      la classe k in quell'ora ha l'ora buca (solo dopo le ore obbligatorie);
- y[t, g, h]      il docente t ha lezione in quell'ora (in una classe qualunque);
- q[t, k, g, h]   il docente t ha lezione nella classe k in quell'ora;
- e[t, k, g]      il docente t quel giorno entra nella classe k;
- w[t, g]         il docente t quel giorno insegna.
"""
import itertools
import os
import time

from ortools.sat.python import cp_model

from .scuola import BUCA, QUALUNQUE, docenti_della_voce

TROVATO, IMPOSSIBILE, LIMITE_DI_TEMPO = 'trovato', 'impossibile', 'limite di tempo'


def modello(scuola, cp=cp_model):
    """Il caso scritto per CP-SAT. Ritorna (il modello, le x, le b): x e b sono dizionari, e una cella che manca vuol
    dire «qui no». (`cp` si puo' sostituire nelle prove con un finto risolutore che controlla solo le regole.)"""
    G, O, C = len(scuola['giorni']), len(scuola['ore']), len(scuola['classi'])
    voci, docenti = scuola['voci'], scuola['docenti']
    V, T = len(voci), len(docenti)
    m = cp.CpModel()
    docenti_di = [docenti_della_voce(voce) for voce in voci]

    # ---- le x: solo dove tutti i docenti della voce sono a scuola (regola 3: ingresso, uscita, giorno preciso) ----
    x = {}
    for v in range(V):
        suoi = [docenti[t] for t in docenti_di[v]]
        ingresso, uscita = max(d[1] for d in suoi), min(d[2] for d in suoi)
        for g in range(G):
            if any(d[3] == g for d in suoi):
                continue
            for h in range(ingresso, min(uscita, O)):
                x[v, g, h] = m.NewBoolVar('x_%d_%d_%d' % (v, g, h))
    # ---- le b: solo dopo le ore obbligatorie (regola 7) ----
    b = {(k, g, h): m.NewBoolVar('b_%d_%d_%d' % (k, g, h))
         for k in range(C) for g in range(G) for h in range(scuola['obbligatorie'], O)}

    voci_della_classe = [[v for v in range(V) if voci[v][0] == k] for k in range(C)]
    voci_del_docente = [[v for v in range(V) if t in docenti_di[v]] for t in range(T)]

    def uguale_alla_somma(nome, termini):
        """Un'incognita si'/no che vale quanto la somma dei termini (che e' al massimo 1). None se non ce ne sono."""
        if not termini:
            return None
        if len(termini) == 1:
            return termini[0]
        nuova = m.NewBoolVar(nome)
        m.Add(nuova == sum(termini))
        return nuova

    # ---- regola 1 (le ore) e regola 7 (la giornata della classe) ----
    for k in range(C):
        for g in range(G):
            for h in range(O):
                # in ogni cella una cosa sola: una voce della classe, o l'ora buca
                m.AddExactlyOne([x[v, g, h] for v in voci_della_classe[k] if (v, g, h) in x]
                                + ([b[k, g, h]] if (k, g, h) in b else []))
                # dopo un'ora buca solo ore buche
                if (k, g, h) in b and h + 1 < O:
                    m.AddImplication(b[k, g, h], b[k, g, h + 1])
    for v in range(V):
        # ogni voce fa le sue ore nella settimana (se non ha nessun posto dove stare, il caso e' impossibile)
        posti = [x[v, g, h] for g in range(G) for h in range(O) if (v, g, h) in x]
        if posti:
            m.Add(sum(posti) == voci[v][3])
        else:
            m.AddBoolOr([])

    # ---- regola 6: le giornate che un docente puo' avere (non troppe ore, non troppe ore buche) ----
    giornate = []
    for quante in range(min(O, scuola['lezioni_al_giorno']) + 1):
        for ore in itertools.combinations(range(O), quante):
            if not ore or ore[-1] - ore[0] + 1 - quante <= scuola['buche_al_giorno']:
                giornate.append(tuple(int(h in ore) for h in range(O)))

    for t in range(T):
        classi_sue = sorted({voci[v][0] for v in voci_del_docente[t]})
        giorni_di_lezione = []
        for g in range(G):
            # y: in ogni ora la somma delle x delle sue voci; al massimo una (regola 2: una classe per volta)
            y = []
            for h in range(O):
                termini = [x[v, g, h] for v in voci_del_docente[t] if (v, g, h) in x]
                if len(termini) > 1:
                    m.AddAtMostOne(termini)
                y.append(uguale_alla_somma('y_%d_%d_%d' % (t, g, h), termini))
            if not any(i is not None for i in y):
                continue   # quel giorno non puo' insegnare
            # regola 6: la sua giornata e' una di quelle consentite (le ore in cui non puo' insegnare valgono 0)
            presenti = [h for h in range(O) if y[h] is not None]
            m.AddAllowedAssignments([y[h] for h in presenti],
                                    sorted({tuple(gi[h] for h in presenti) for gi in giornate
                                            if not any(gi[h] for h in range(O) if h not in presenti)}))
            # w: quel giorno insegna (per il giorno libero qualunque)
            w = m.NewBoolVar('w_%d_%d' % (t, g))
            m.AddMaxEquality(w, [y[h] for h in presenti])
            giorni_di_lezione.append(w)
            # regola 4 (in una classe: al massimo tante ore, di seguito) e regola 5 (le classi del giorno)
            entra = []
            for k in classi_sue:
                q = [uguale_alla_somma('q_%d_%d_%d_%d' % (t, k, g, h),
                                       [x[v, g, h] for v in voci_del_docente[t] if voci[v][0] == k and (v, g, h) in x])
                     for h in range(O)]
                ci_sono = [i for i in q if i is not None]
                if not ci_sono:
                    continue
                m.Add(sum(ci_sono) <= scuola['ore_di_seguito'])
                # di seguito: se c'e' in due ore c'e' anche in tutte quelle in mezzo
                for a in range(O):
                    for c in range(a + 2, O):
                        if q[a] is not None and q[c] is not None:
                            for h in range(a + 1, c):
                                if q[h] is None:
                                    m.AddBoolOr([q[a].Not(), q[c].Not()])
                                else:
                                    m.AddBoolOr([q[a].Not(), q[c].Not(), q[h]])
                e = m.NewBoolVar('e_%d_%d_%d' % (t, k, g))
                m.AddMaxEquality(e, ci_sono)
                entra.append(e)
            if len(entra) > scuola['classi_al_giorno']:
                m.Add(sum(entra) <= scuola['classi_al_giorno'])
        # regola 3, il giorno libero qualunque: almeno un giorno senza lezioni
        if docenti[t][3] == QUALUNQUE:
            m.Add(sum(giorni_di_lezione) <= G - 1)

    # ---- regola 8: in ogni ora le lezioni di un tipo di aula non sono piu' delle aule di quel tipo ----
    quante_aule = {}
    for _, tipo in scuola['aule']:
        quante_aule[tipo] = quante_aule.get(tipo, 0) + 1
    for tipo, quante in sorted(quante_aule.items()):
        sue = [v for v in range(V) if scuola['materie'][voci[v][4]][1] == tipo]
        if len({voci[v][0] for v in sue}) > quante:
            for g in range(G):
                for h in range(O):
                    insieme = [x[v, g, h] for v in sue if (v, g, h) in x]
                    if len(insieme) > quante:
                        m.Add(sum(insieme) <= quante)
    return m, x, b


def risolvi(scuola, tempo=300, fili=None):
    """Come esatto.risolvi: ritorna (esito, orario, aule, secondi). `fili`: quanti processori usa (se non e' detto:
    fino a quattro, lasciandone uno libero per l'altro metodo, che gira insieme a questo)."""
    from . import esatto   # (solo per dare le aule all'orario trovato: e' lo stesso passaggio)
    G, O, C = len(scuola['giorni']), len(scuola['ore']), len(scuola['classi'])
    partenza = time.time()
    m, x, _ = modello(scuola)
    risolutore = cp_model.CpSolver()
    risolutore.parameters.max_time_in_seconds = float(tempo)
    risolutore.parameters.num_search_workers = fili or max(1, min(4, (os.cpu_count() or 2) - 1))
    stato = risolutore.Solve(m)
    secondi = time.time() - partenza
    if stato == cp_model.INFEASIBLE:
        return IMPOSSIBILE, None, None, secondi
    if stato not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return LIMITE_DI_TEMPO, None, None, secondi
    # dalla risposta all'orario: in ogni cella la voce la cui x vale 1, o l'ora buca
    voci_della_classe = [[v for v in range(len(scuola['voci'])) if scuola['voci'][v][0] == k] for k in range(C)]
    orario = []
    for g in range(G):
        for h in range(O):
            for k in range(C):
                messa = [v for v in voci_della_classe[k] if (v, g, h) in x and risolutore.Value(x[v, g, h])]
                orario.append(messa[0] if messa else BUCA)
    return TROVATO, orario, esatto._Conti(scuola).aule(orario), secondi
