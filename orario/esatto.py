"""Il metodo che fa l'orario: tutto il caso scritto come un sistema di conti con incognite 0/1, risolto da un risolutore
generico di programmi lineari interi (HiGHS, dentro scipy: e' compilato, il lavoro pesante lo fa lui).

Non riempie le celle una alla volta tornando indietro: guarda tutte le regole di tutta la settimana insieme. Da'
l'orario, oppure dimostra che non esiste.

LE INCOGNITE, tutte 0 o 1:
- x[v, g, h]      la voce v e' nell'ora h del giorno g (nella sua classe);
- b[k, g, h]      la classe k in quell'ora ha l'ora buca;
- c[t, k, g, h]   il docente t COMINCIA a insegnare nella classe k in quell'ora (nell'ora prima non c'era);
- p[t, g, h]      il docente t quel giorno ha GIA' cominciato: ha lezione in quell'ora o in una prima;
- d[t, g, h]      il docente t quel giorno DEVE ANCORA finire: ha lezione in quell'ora o in una dopo.

LE REGOLE (le nove di scuola.py), ognuna come una somma che deve stare tra due numeri:
1. ogni voce fa le sue ore: la somma delle sue x nella settimana fa le sue ore; e ogni cella ha una cosa sola: la
   somma delle x delle voci della classe, piu' b, fa 1;
2. un docente e' in una classe per volta: la somma delle x delle sue voci, in un'ora, fa al massimo 1;
3. non prima dell'ingresso: quelle x non possono valere 1 (il loro massimo e' 0);
4. al massimo N ore al giorno in una classe: la somma delle x del docente in quella classe quel giorno e' al massimo N;
5. di seguito: c >= (c'e' adesso) - (c'era prima), e in un giorno comincia al massimo una volta;
6. al massimo M classi al giorno: gli inizi del docente in un giorno, in tutte le classi, sono al massimo M
   (in ogni classe comincia al piu' una volta, quindi contare gli inizi e' contare le classi);
7. le prime ore hanno lezione: quelle b non possono valere 1;
8. dopo un'ora buca solo ore buche: b dell'ora dopo >= b di quest'ora;
9. al massimo B ore buche al giorno per un docente. Un'ora e' «dentro la sua giornata» se ha gia' cominciato e deve
   ancora finire (p e d valgono 1 tutti e due); le ore buche sono le ore dentro la giornata meno quelle di lezione.
   p >= lezione in quell'ora, p >= p dell'ora prima; d >= lezione in quell'ora, d >= d dell'ora dopo; e in un giorno
   la somma di (p + d - 1 - lezione) e' al massimo B. (p e d hanno solo un minimo: possono valere 1 anche senza
   motivo, ma questo farebbe solo contare piu' ore buche, quindi non aiuta a barare.)

Non c'e' niente da rendere massimo o minimo: si cerca solo una risposta che rispetti tutto.

I PASSAGGI (uno per funzione, nell'ordine in cui risolvi() li fa):
1. _Conti.__init__    si da' un posto a ogni incognita;
2. _Conti.regole      si scrive una riga per ogni regola;
3. _Conti.vietate     si segnano le incognite che devono valere 0 (regole 3 e 7);
4. _chiedi            si da' tutto al risolutore;
5. _Conti.orario      dalla sua risposta si legge l'orario.
risolvi() li fa due volte, se serve: prima senza la regola 9, poi con (il perche' e' scritto li').

ATTENZIONE: l'ordine delle incognite e delle righe non cambia la risposta (trovato o impossibile), ma cambia quanto
il risolutore ci mette, anche di molto.
"""
import time

import numpy as np
from scipy.optimize import Bounds, LinearConstraint, milp
from scipy.sparse import coo_matrix

from .scuola import BUCA, docenti_della_voce

TROVATO, IMPOSSIBILE, LIMITE_DI_TEMPO = 'trovato', 'impossibile', 'limite di tempo'


class _Conti:
    """Il caso scritto come conti: il posto di ogni incognita, le righe delle regole, le incognite vietate."""

    def __init__(self, scuola, con_le_buche=True):
        """PASSO 1: un posto (un numero, da 0) per ogni incognita. Prima tutte le x, poi le b, le c, le p e le d.
        `con_le_buche`: False per lasciare fuori la regola 9 (e le sue incognite p e d): vedi risolvi()."""
        self.con_le_buche = con_le_buche
        self.scuola = scuola
        self.G, self.O, self.C = len(scuola['giorni']), len(scuola['ore']), len(scuola['classi'])
        self.H = self.G * self.O                       # le ore della settimana
        self.voci = scuola['voci']
        self.V = len(self.voci)
        # le voci di ogni classe, e i docenti di ogni voce
        self.voci_della_classe = [[v for v in range(self.V) if self.voci[v][0] == k] for k in range(self.C)]
        self.docenti_di = [docenti_della_voce(voce) for voce in self.voci]
        # per ogni coppia (docente, classe) in cui il docente insegna: le sue voci li' (da solo e in coppia)
        self.in_classe = {}
        for v, voce in enumerate(self.voci):
            for t in self.docenti_di[v]:
                self.in_classe.setdefault((t, voce[0]), []).append(v)
        self.coppie = sorted(self.in_classe)
        # dove cominciano le c di ogni coppia (docente, classe), e quante incognite ci sono in tutto
        self.inizio_c = {coppia: (self.V + self.C + i) * self.H for i, coppia in enumerate(self.coppie)}
        self.T = len(scuola['docenti'])
        self.inizio_p = (self.V + self.C + len(self.coppie)) * self.H   # dove cominciano le p; le d subito dopo
        self.incognite = self.inizio_p + (2 * self.T * self.H if con_le_buche else 0)

    # il posto di un'incognita
    def x(self, v, g, h): return v * self.H + g * self.O + h                  # la voce v in quell'ora
    def b(self, k, g, h): return (self.V + k) * self.H + g * self.O + h       # l'ora buca della classe k
    def c(self, coppia, g, h): return self.inizio_c[coppia] + g * self.O + h  # il docente comincia nella classe
    def p(self, t, g, h): return self.inizio_p + t * self.H + g * self.O + h  # il docente ha gia' cominciato
    def d(self, t, g, h): return self.p(t, g, h) + self.T * self.H            # il docente deve ancora finire

    def regole(self):
        """PASSO 2: le regole. Ogni regola e' una riga: una somma di incognite (ognuna con il suo coefficiente) che
        deve stare tra un minimo e un massimo. Ritorna (la tabella dei coefficienti, i minimi, i massimi)."""
        G, O, C, x, b, c = self.G, self.O, self.C, self.x, self.b, self.c
        righe, colonne, valori, minimi, massimi = [], [], [], [], []

        def regola(termini, minimo, massimo):
            # una riga in piu': i suoi termini sono (incognita, coefficiente)
            for incognita, coefficiente in termini:
                righe.append(len(minimi)), colonne.append(incognita), valori.append(coefficiente)
            minimi.append(minimo), massimi.append(massimo)

        T, p, d = self.T, self.p, self.d
        voci_del_docente = [[v for v in range(self.V) if t in self.docenti_di[v]] for t in range(T)]
        # ora per ora
        for g in range(G):
            for h in range(O):
                for k in range(C):
                    # ogni cella ha una cosa sola: una voce della classe, oppure l'ora buca
                    regola([(x(v, g, h), 1) for v in self.voci_della_classe[k]] + [(b(k, g, h), 1)], 1, 1)
                    # regola 8: se quest'ora e' buca lo e' anche la prossima
                    if h + 1 < O:
                        regola([(b(k, g, h + 1), 1), (b(k, g, h), -1)], 0, 1)
                for t in range(T):
                    # regola 2: il docente e' in una classe per volta
                    regola([(x(v, g, h), 1) for v in voci_del_docente[t]], 0, 1)
        # regola 1: ogni voce fa le sue ore nella settimana
        for v in range(self.V):
            regola([(x(v, g, h), 1) for g in range(G) for h in range(O)], self.voci[v][3], self.voci[v][3])
        # la giornata di un docente in una classe
        for coppia in self.coppie:
            sue = self.in_classe[coppia]
            for g in range(G):
                # regola 4: al massimo tante ore al giorno
                regola([(x(v, g, h), 1) for v in sue for h in range(O)], 0, self.scuola['ore_di_seguito'])
                # regola 5: comincia al massimo una volta al giorno...
                regola([(c(coppia, g, h), 1) for h in range(O)], 0, 1)
                # ...e «comincia» vuol dire: c'e' adesso e non c'era nell'ora prima (c >= adesso - prima)
                for h in range(O):
                    regola([(c(coppia, g, h), 1)] + [(x(v, g, h), -1) for v in sue]
                           + ([(x(v, g, h - 1), 1) for v in sue] if h else []), 0, 1)
        # regola 6: in un giorno il docente comincia, in tutte le sue classi, al massimo tante volte
        for t in range(T):
            sue = [coppia for coppia in self.coppie if coppia[0] == t]
            if len(sue) > self.scuola['classi_al_giorno']:   # (con poche classi in tutto non puo' superarle)
                for g in range(G):
                    regola([(c(coppia, g, h), 1) for coppia in sue for h in range(O)],
                           0, self.scuola['classi_al_giorno'])
        # regola 9: le ore buche di un docente in un giorno
        buche = self.scuola['buche_al_giorno']
        for t in range(T if self.con_le_buche else 0):
            for g in range(G):
                for h in range(O):
                    lezione = [(x(v, g, h), -1) for v in voci_del_docente[t]]
                    regola([(p(t, g, h), 1)] + lezione, 0, 1)      # ha gia' cominciato: se ha lezione adesso...
                    regola([(d(t, g, h), 1)] + lezione, 0, 1)      # deve ancora finire: se ha lezione adesso...
                    if h:
                        regola([(p(t, g, h), 1), (p(t, g, h - 1), -1)], 0, 1)   # ...o aveva gia' cominciato prima
                    if h + 1 < O:
                        regola([(d(t, g, h), 1), (d(t, g, h + 1), -1)], 0, 1)   # ...o deve finire piu' tardi
                # le ore dentro la giornata (p + d - 1) meno quelle di lezione: al massimo `buche`. Il -1 di ogni ora
                # sta nel massimo (+ O). In un giorno senza lezioni la somma puo' scendere fino a -O.
                regola([(q(t, g, h), 1) for h in range(O) for q in (p, d)]
                       + [(x(v, g, h), -1) for h in range(O) for v in voci_del_docente[t]], -O, buche + O)
        tabella = coo_matrix((valori, (righe, colonne)), shape=(len(minimi), self.incognite)).tocsr()
        return tabella, minimi, massimi

    def vietate(self):
        """PASSO 3: il massimo di ogni incognita. Di solito 1; 0 dove una regola dice «qui no»."""
        massimo = np.ones(self.incognite)
        # regola 3: nessuna voce prima dell'ingresso dei suoi docenti (per una coppia, di chi entra dopo)
        for v in range(self.V):
            ingresso = max(self.scuola['docenti'][t][1] for t in self.docenti_di[v])
            for g in range(self.G):
                for h in range(min(ingresso, self.O)):
                    massimo[self.x(v, g, h)] = 0
        # regola 7: niente ora buca nelle prime ore
        for k in range(self.C):
            for g in range(self.G):
                for h in range(min(self.scuola['obbligatorie'], self.O)):
                    massimo[self.b(k, g, h)] = 0
        return massimo

    def orario(self, risposta):
        """PASSO 5: dalla risposta del risolutore all'orario: in ogni cella la voce la cui x vale 1, o l'ora buca."""
        risposta = np.round(risposta)   # (il risolutore puo' dare 0,9999999 al posto di 1)
        orario = []
        for g in range(self.G):
            for h in range(self.O):
                for k in range(self.C):
                    messa = [v for v in self.voci_della_classe[k] if risposta[self.x(v, g, h)] == 1]
                    orario.append(messa[0] if messa else BUCA)
        return orario


def _chiedi(conti, tempo):
    """PASSO 4: i conti al risolutore. Niente da rendere minimo (tutti zeri), tutte le incognite intere, tra 0 e il
    loro massimo. Ritorna (esito, orario)."""
    tabella, minimi, massimi = conti.regole()   # passo 2
    massimo = conti.vietate()                   # passo 3
    n = conti.incognite
    r = milp(np.zeros(n), constraints=LinearConstraint(tabella, minimi, massimi), integrality=np.ones(n),
             bounds=Bounds(np.zeros(n), massimo), options={'time_limit': max(tempo, 1)})
    if r.status == 2:                  # nessuna risposta rispetta tutte le regole: e' dimostrato
        return IMPOSSIBILE, None
    if r.status != 0 or r.x is None:   # non ha finito in tempo
        return LIMITE_DI_TEMPO, None
    return TROVATO, conti.orario(r.x)  # passo 5


def _buche_a_posto(scuola, orario):
    """L'orario rispetta la regola 9? Per ogni docente e giorno: tra la prima e l'ultima ora in cui insegna, le ore
    senza lezione sono al massimo quelle consentite."""
    G, O, C = len(scuola['giorni']), len(scuola['ore']), len(scuola['classi'])
    ore = {}   # (docente, giorno) -> le ore in cui insegna
    for cella, v in enumerate(orario):
        if v != BUCA:
            for t in docenti_della_voce(scuola['voci'][v]):
                ore.setdefault((t, cella // C // O), set()).add(cella // C % O)
    return all(max(sue) - min(sue) + 1 - len(sue) <= scuola['buche_al_giorno'] for sue in ore.values())


def risolvi(scuola, tempo=300):
    """Cerca un orario per la scuola (scuola.py), in al massimo `tempo` secondi. Ritorna (esito, orario, secondi):
    l'esito e' TROVATO, IMPOSSIBILE (dimostrato: un orario non esiste) o LIMITE_DI_TEMPO; l'orario, se c'e', ha una
    voce per cella (-1: ora buca); i secondi sono quelli del risolutore.

    In due tempi, perche' la regola 9 (le ore buche dei docenti) rende molto piu' lento dimostrare che un caso e'
    impossibile:
    A. prima senza la regola 9. Se gia' cosi' un orario non esiste, non esiste nemmeno con una regola in piu': e' gia'
       la risposta. Se l'orario trovato rispetta per caso anche la regola 9, e' gia' la risposta.
    B. altrimenti da capo, con tutte le regole."""
    partenza = time.time()
    esito, orario = _chiedi(_Conti(scuola, con_le_buche=False), tempo)                      # A
    if esito == TROVATO and not _buche_a_posto(scuola, orario):
        esito, orario = _chiedi(_Conti(scuola), tempo - (time.time() - partenza))           # B
    return esito, orario, time.time() - partenza
