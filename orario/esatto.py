"""Il metodo che fa l'orario: tutto il caso scritto come un sistema di conti con incognite 0/1, risolto da un risolutore
generico di programmi lineari interi (HiGHS, dentro scipy: e' compilato, il lavoro pesante lo fa lui).

Non riempie le celle una alla volta tornando indietro: guarda tutte le regole di tutta la settimana insieme. Da'
l'orario, oppure dimostra che non esiste.

LE INCOGNITE, tutte 0 o 1:
- x[v, g, h]      la voce v e' nell'ora h del giorno g (nella sua classe);
- b[k, g, h]      la classe k in quell'ora ha l'ora buca;
- c[t, k, g, h]   il docente t COMINCIA a insegnare nella classe k in quell'ora (nell'ora prima non c'era).

LE REGOLE (le otto di scuola.py), ognuna come una somma che deve stare tra due numeri:
1. ogni voce fa le sue ore: la somma delle sue x nella settimana fa le sue ore; e ogni cella ha una cosa sola: la
   somma delle x delle voci della classe, piu' b, fa 1;
2. un docente e' in una classe per volta: la somma delle x delle sue voci, in un'ora, fa al massimo 1;
3. non prima dell'ingresso: quelle x non possono valere 1 (il loro massimo e' 0);
4. al massimo N ore al giorno in una classe: la somma delle x del docente in quella classe quel giorno e' al massimo N;
5. di seguito: c >= (c'e' adesso) - (c'era prima), e in un giorno comincia al massimo una volta;
6. al massimo M classi al giorno: gli inizi del docente in un giorno, in tutte le classi, sono al massimo M
   (in ogni classe comincia al piu' una volta, quindi contare gli inizi e' contare le classi);
7. le prime ore hanno lezione: quelle b non possono valere 1;
8. dopo un'ora buca solo ore buche: b dell'ora dopo >= b di quest'ora.

Non c'e' niente da rendere massimo o minimo: si cerca solo una risposta che rispetti tutto.
"""
import time

import numpy as np
from scipy.optimize import Bounds, LinearConstraint, milp
from scipy.sparse import coo_matrix

TROVATO, IMPOSSIBILE, LIMITE_DI_TEMPO = 'trovato', 'impossibile', 'limite di tempo'


def risolvi(scuola, tempo=300):
    """Cerca un orario per la scuola (scuola.py), in al massimo `tempo` secondi. Ritorna (esito, orario, secondi):
    l'esito e' TROVATO, IMPOSSIBILE (dimostrato: un orario non esiste) o LIMITE_DI_TEMPO; l'orario, se c'e', ha una
    voce per cella (-1: ora buca); i secondi sono quelli del risolutore."""
    G, O, C = len(scuola['giorni']), len(scuola['ore']), len(scuola['classi'])
    voci, docenti, obbligatorie = scuola['voci'], scuola['docenti'], scuola['obbligatorie']
    di_seguito, classi_al_giorno = scuola['ore_di_seguito'], scuola['classi_al_giorno']
    V, T, H = len(voci), len(docenti), G * O   # H: le ore della settimana

    # i docenti di ogni voce, e per ogni coppia (docente, classe) le voci del docente in quella classe
    docenti_di = [[t for t in v[1:3] if t >= 0] for v in voci]
    in_classe = {}
    for v, voce in enumerate(voci):
        for t in docenti_di[v]:
            in_classe.setdefault((t, voce[0]), []).append(v)
    coppie = sorted(in_classe)

    # il posto di ogni incognita: prima le x, poi le b, poi le c
    def x(v, g, h): return v * H + g * O + h
    def b(k, g, h): return V * H + k * H + g * O + h
    inizio = {coppia: V * H + C * H + i * H for i, coppia in enumerate(coppie)}
    def c(coppia, g, h): return inizio[coppia] + g * O + h
    incognite = V * H + C * H + len(coppie) * H

    # le regole: una riga per regola, come elenco di (incognita, coefficiente), con il minimo e il massimo della somma
    righe, colonne, valori, minimi, massimi = [], [], [], [], []

    def regola(termini, minimo, massimo):
        riga = len(minimi)
        for incognita, coefficiente in termini:
            righe.append(riga), colonne.append(incognita), valori.append(coefficiente)
        minimi.append(minimo), massimi.append(massimo)

    voci_della_classe = [[v for v in range(V) if voci[v][0] == k] for k in range(C)]
    voci_del_docente = [[v for v in range(V) if t in docenti_di[v]] for t in range(T)]
    for g in range(G):
        for h in range(O):
            for k in range(C):
                regola([(x(v, g, h), 1) for v in voci_della_classe[k]] + [(b(k, g, h), 1)], 1, 1)       # 1
                if h + 1 < O:
                    regola([(b(k, g, h + 1), 1), (b(k, g, h), -1)], 0, 1)                               # 8
            for t in range(T):
                regola([(x(v, g, h), 1) for v in voci_del_docente[t]], 0, 1)                            # 2
    for v in range(V):
        regola([(x(v, g, h), 1) for g in range(G) for h in range(O)], voci[v][3], voci[v][3])           # 1
    for coppia in coppie:
        sue = in_classe[coppia]
        for g in range(G):
            regola([(x(v, g, h), 1) for v in sue for h in range(O)], 0, di_seguito)                     # 4
            regola([(c(coppia, g, h), 1) for h in range(O)], 0, 1)                                      # 5
            for h in range(O):
                regola([(c(coppia, g, h), 1)] + [(x(v, g, h), -1) for v in sue]
                       + ([(x(v, g, h - 1), 1) for v in sue] if h else []), 0, 1)                       # 5
    for t in range(T):
        sue = [coppia for coppia in coppie if coppia[0] == t]
        if len(sue) > classi_al_giorno:   # (se ha poche classi in tutto la regola non puo' essere violata)
            for g in range(G):
                regola([(c(coppia, g, h), 1) for coppia in sue for h in range(O)], 0, classi_al_giorno)  # 6

    # i massimi delle incognite: 1, oppure 0 dove la regola dice «qui no»
    massimo = np.ones(incognite)
    for v in range(V):
        ingresso = max(docenti[t][1] for t in docenti_di[v])
        for g in range(G):
            for h in range(min(ingresso, O)):
                massimo[x(v, g, h)] = 0                                                                 # 3
    for k in range(C):
        for g in range(G):
            for h in range(min(obbligatorie, O)):
                massimo[b(k, g, h)] = 0                                                                 # 7

    regole = coo_matrix((valori, (righe, colonne)), shape=(len(minimi), incognite)).tocsr()
    partenza = time.time()
    r = milp(np.zeros(incognite), constraints=LinearConstraint(regole, minimi, massimi),
             integrality=np.ones(incognite), bounds=Bounds(np.zeros(incognite), massimo), options={'time_limit': tempo})
    secondi = time.time() - partenza
    if r.status == 2:        # HiGHS: nessuna risposta rispetta tutte le regole
        return IMPOSSIBILE, None, secondi
    if r.status != 0 or r.x is None:
        return LIMITE_DI_TEMPO, None, secondi
    # dalla risposta all'orario: in ogni cella la voce la cui x vale 1, o l'ora buca
    risposta = np.round(r.x)
    orario = []
    for g in range(G):
        for h in range(O):
            for k in range(C):
                messa = [v for v in voci_della_classe[k] if risposta[x(v, g, h)] == 1]
                orario.append(messa[0] if messa else -1)
    return TROVATO, orario, secondi
