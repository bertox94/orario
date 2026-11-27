"""Il metodo che fa l'orario: tutto il caso scritto come un sistema di conti con incognite 0/1, risolto da un risolutore
generico di programmi lineari interi (HiGHS, dentro scipy: e' compilato, il lavoro pesante lo fa lui).

Non riempie le celle una alla volta tornando indietro: guarda tutte le regole di tutta la settimana insieme. Da'
l'orario, oppure dimostra che non esiste.

LE INCOGNITE, tutte 0 o 1:
- x[v, g, h]      la voce v e' nell'ora h del giorno g (nella sua classe);
- b[k, g, h]      la classe k in quell'ora ha l'ora buca;
- c[t, k, g, h]   il docente t COMINCIA a insegnare nella classe k in quell'ora (nell'ora prima non c'era);
- f[t, g]         il giorno g e' il giorno libero del docente t (solo per chi non ne ha uno preciso).

LE REGOLE (le otto di scuola.py), ognuna come somme che devono stare tra due numeri:
1. le ore: la somma delle x di una voce nella settimana fa le sue ore; e ogni cella ha una cosa sola: la somma delle
   x delle voci della classe, piu' b, fa 1;
2. una classe per volta: la somma delle x delle voci di un docente, in un'ora, fa al massimo 1;
3. a scuola: le x di una voce prima dell'ingresso dei suoi docenti, dalla loro uscita in poi e nel loro giorno libero
   preciso non possono valere 1 (il loro massimo e' 0). Chi ha un giorno libero qualunque: la somma delle sue f nella
   settimana fa 1 (ne segna uno), e in ogni giorno (lezioni del giorno) + L * f del giorno e' al massimo L: nel
   giorno segnato come libero non puo' avere lezione (e' la riga delle ore di lezione della regola 6, con f in piu');
4. in una classe: la somma delle x del docente in quella classe quel giorno e' al massimo N; e sono di seguito:
   c >= (c'e' adesso) - (c'era prima), e in un giorno comincia al massimo una volta;
5. le classi: gli inizi del docente in un giorno, in tutte le classi, sono al massimo M (in ogni classe comincia al
   piu' una volta, quindi contare gli inizi e' contare le classi);
6. la giornata del docente. Le ore di lezione: la somma delle x delle sue voci in un giorno e' al massimo L. Le ore
   buche: al massimo B, e chi insegna L ore deve averne B (la pausa). Due richieste:
   a. mai piu' di B: per ogni coppia di ore a < b con in mezzo piu' di B ore, se ha lezione in tutte e due in mezzo
      deve avere almeno (ore in mezzo - B) lezioni:
      (lezioni in mezzo) >= (ore in mezzo - B) * (lezione in a + lezione in b - 1);
   b. chi fa L ore ha la pausa: in L ore di seguito le lezioni sono al massimo L - 1.
   Non servono incognite in piu';
7. la giornata della classe: le b delle prime ore non possono valere 1; e dopo un'ora buca solo ore buche: b dell'ora
   dopo >= b di quest'ora;
8. le aule: in ogni ora le lezioni che chiedono un tipo di aula sono al massimo quante le aule di quel tipo. Quale
   aula tocca a ogni lezione non serve deciderlo nei conti: se le aule bastano, in ogni ora le si distribuisce dopo
   (_Conti.aule).

Non c'e' niente da rendere massimo o minimo: si cerca solo una risposta che rispetti tutto.

I PASSAGGI (uno per funzione, nell'ordine in cui risolvi() li fa):
1. _Conti.__init__    si da' un posto a ogni incognita;
2. _Conti.regole      si scrive una riga per ogni regola;
3. _Conti.vietate     si segnano le incognite che devono valere 0 (regole 3 e 7);
4. _chiedi            si da' tutto al risolutore;
5. _Conti.orario      dalla sua risposta si legge l'orario;
6. _Conti.aule        a ogni lezione dell'orario si da' la sua aula.
risolvi() li fa due volte, se serve: prima senza le ore buche dei docenti (regola 6), poi con (il perche' e' scritto
li').

ATTENZIONE: l'ordine delle incognite e delle righe non cambia la risposta (trovato o impossibile), ma cambia quanto
il risolutore ci mette, anche di molto.
"""
import time

import numpy as np
from scipy.optimize import Bounds, LinearConstraint, milp
from scipy.sparse import coo_matrix

from .scuola import BUCA, QUALUNQUE, buche_consentite, docenti_della_voce

TROVATO, IMPOSSIBILE, LIMITE_DI_TEMPO = 'trovato', 'impossibile', 'limite di tempo'


class _Conti:
    """Il caso scritto come conti: il posto di ogni incognita, le righe delle regole, le incognite vietate."""

    def __init__(self, scuola, con_le_buche=True):
        """PASSO 1: un posto (un numero, da 0) per ogni incognita. Prima tutte le x, poi le b, le c e le f.
        `con_le_buche`: False per lasciare fuori le ore buche dei docenti (regola 6): vedi risolvi()."""
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
        # il tipo di aula che ogni voce chiede (quello della sua materia), e le aule di ogni tipo
        self.tipo_della_voce = [scuola['materie'][voce[4]][1] for voce in self.voci]
        self.aule_del_tipo = {}
        for a, (_, tipo) in enumerate(scuola['aule']):
            self.aule_del_tipo.setdefault(tipo, []).append(a)
        self.incognite = (self.V + self.C + len(self.coppie)) * self.H
        # i docenti con un giorno libero qualunque, e dove cominciano le loro f (in fondo a tutte le altre)
        self.con_un_libero = [t for t in range(self.T) if scuola['docenti'][t][3] == QUALUNQUE]
        self.inizio_f = {t: self.incognite + i * self.G for i, t in enumerate(self.con_un_libero)}
        self.incognite += len(self.con_un_libero) * self.G

    # il posto di un'incognita
    def x(self, v, g, h): return v * self.H + g * self.O + h                  # la voce v in quell'ora
    def b(self, k, g, h): return (self.V + k) * self.H + g * self.O + h       # l'ora buca della classe k
    def c(self, coppia, g, h): return self.inizio_c[coppia] + g * self.O + h  # il docente comincia nella classe
    def f(self, t, g): return self.inizio_f[t] + g                            # il giorno libero del docente

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

        T = self.T
        voci_del_docente = [[v for v in range(self.V) if t in self.docenti_di[v]] for t in range(T)]
        # ora per ora
        for g in range(G):
            for h in range(O):
                for k in range(C):
                    # ogni cella ha una cosa sola: una voce della classe, oppure l'ora buca
                    regola([(x(v, g, h), 1) for v in self.voci_della_classe[k]] + [(b(k, g, h), 1)], 1, 1)
                    # regola 7: se quest'ora e' buca lo e' anche la prossima
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
                # regola 4, di seguito: comincia al massimo una volta al giorno...
                regola([(c(coppia, g, h), 1) for h in range(O)], 0, 1)
                # ...e «comincia» vuol dire: c'e' adesso e non c'era nell'ora prima (c >= adesso - prima)
                for h in range(O):
                    regola([(c(coppia, g, h), 1)] + [(x(v, g, h), -1) for v in sue]
                           + ([(x(v, g, h - 1), 1) for v in sue] if h else []), 0, 1)
        # regola 5: in un giorno il docente comincia, in tutte le sue classi, al massimo tante volte
        for t in range(T):
            sue = [coppia for coppia in self.coppie if coppia[0] == t]
            if len(sue) > self.scuola['classi_al_giorno']:   # (con poche classi in tutto non puo' superarle)
                for g in range(G):
                    regola([(c(coppia, g, h), 1) for coppia in sue for h in range(O)],
                           0, self.scuola['classi_al_giorno'])
        # regola 6, le ore di lezione di un docente in un giorno, in tutte le classi: al massimo `lezioni`. E regola 3,
        # il giorno libero qualunque, nella stessa riga: se f dice che il giorno e' libero, le lezioni sono zero
        # (lezioni del giorno + lezioni * f <= lezioni), e di giorni liberi il docente ne segna uno solo.
        # (Una riga per giorno invece di una per ora: sono meno righe, e il risolutore non va peggio.)
        lezioni = min(self.scuola['lezioni_al_giorno'], O)
        for t in range(T):
            libero = t in self.inizio_f
            if lezioni < O or libero:   # (con giornate corte e senza giorno da scegliere non c'e' niente da chiedere)
                for g in range(G):
                    regola([(x(v, g, h), 1) for v in voci_del_docente[t] for h in range(O)]
                           + ([(self.f(t, g), lezioni)] if libero else []), 0, lezioni)
            if libero:
                regola([(self.f(t, g), 1) for g in range(G)], 1, 1)
        # regola 6: le ore buche di un docente in un giorno. `lezione[h]` sono i termini della somma «ha lezione
        # nell'ora h». Due richieste (vedi in cima):
        buche, massime = self.scuola['buche_al_giorno'], self.scuola['lezioni_al_giorno']
        for t in range(T if self.con_le_buche else 0):
            for g in range(G):
                lezione = [[(x(v, g, h), 1) for v in voci_del_docente[t]] for h in range(O)]
                # a. mai piu' di `buche` ore buche: per ogni coppia di ore a < b con in mezzo piu' ore delle buche
                #    consentite, se ha lezione in tutte e due in mezzo ne puo' saltare solo `buche`
                for a in range(O):
                    for b in range(a + buche + 2, O):
                        troppe = b - a - 1 - buche
                        in_mezzo = [termine for h in range(a + 1, b) for termine in lezione[h]]
                        regola(in_mezzo + [(i, -troppe * k) for i, k in lezione[a] + lezione[b]], -troppe, O)
                # b. chi fa il massimo delle ore deve avere la pausa: mai `massime` ore tutte di seguito
                if buche:
                    for a in range(O - massime + 1):
                        regola([termine for h in range(a, a + massime) for termine in lezione[h]], 0, massime - 1)
        # regola 8: in ogni ora, per ogni tipo di aula, le lezioni che lo chiedono non sono piu' delle aule
        for tipo, quante in sorted(self.aule_del_tipo.items()):
            sue = [v for v in range(self.V) if self.tipo_della_voce[v] == tipo]
            if len({self.voci[v][0] for v in sue}) > len(quante):   # (se le classi che lo usano sono poche, bastano)
                for g in range(G):
                    for h in range(O):
                        regola([(x(v, g, h), 1) for v in sue], 0, len(quante))
        tabella = coo_matrix((valori, (righe, colonne)), shape=(len(minimi), self.incognite)).tocsr()
        return tabella, minimi, massimi

    def vietate(self):
        """PASSO 3: il massimo di ogni incognita. Di solito 1; 0 dove una regola dice «qui no»."""
        massimo = np.ones(self.incognite)
        # regola 3: nessuna voce prima dell'ingresso dei suoi docenti, ne' dalla loro uscita in poi (per una
        # coppia: di chi entra dopo, e di chi esce prima), ne' nel giorno libero di uno di loro
        for v in range(self.V):
            suoi = [self.scuola['docenti'][t] for t in self.docenti_di[v]]
            ingresso, uscita = max(d[1] for d in suoi), min(d[2] for d in suoi)
            for g in range(self.G):
                libero = any(d[3] == g for d in suoi)
                for h in range(self.O):
                    if h < ingresso or h >= uscita or libero:
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

    def aule(self, orario):
        """PASSO 6: l'aula di ogni lezione dell'orario (una per cella; -1 per l'ora buca). In ogni ora le aule di ogni
        tipo bastano (regola 8): qui si decide solo quale tocca a chi, cercando di non far girare le classi. In
        ordine: l'aula dove la classe era l'ora prima; la «sua» aula (quella con il numero della classe, tra le aule
        di quel tipo); la prima libera."""
        aule = [BUCA] * len(orario)
        for ora in range(self.H):
            libere = {tipo: list(sue) for tipo, sue in self.aule_del_tipo.items()}
            da_fare = []   # le lezioni di quest'ora che non restano dov'erano: (cella, classe, tipo)
            for k in range(self.C):
                cella = ora * self.C + k
                if orario[cella] == BUCA:
                    continue
                tipo = self.tipo_della_voce[orario[cella]]
                prima = aule[cella - self.C] if ora % self.O else BUCA   # l'aula dell'ora prima, nello stesso giorno
                if prima in libere[tipo]:
                    aule[cella] = prima
                    libere[tipo].remove(prima)
                else:
                    da_fare.append((cella, k, tipo))
            for passata in (1, 2):   # prima chi trova libera la sua aula, poi gli altri
                for cella, k, tipo in da_fare:
                    sua = self.aule_del_tipo[tipo][k % len(self.aule_del_tipo[tipo])]
                    if aule[cella] == BUCA and (passata == 2 or sua in libere[tipo]):
                        aule[cella] = sua if sua in libere[tipo] else libere[tipo][0]
                        libere[tipo].remove(aule[cella])
        return aule


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
    """L'orario rispetta la regola sulle ore buche dei docenti? Per ogni docente e giorno: tra la prima e l'ultima ora in cui insegna, le ore
    senza lezione sono quelle consentite (non troppe, e la pausa di chi fa il massimo delle ore)."""
    G, O, C = len(scuola['giorni']), len(scuola['ore']), len(scuola['classi'])
    ore = {}   # (docente, giorno) -> le ore in cui insegna
    for cella, v in enumerate(orario):
        if v != BUCA:
            for t in docenti_della_voce(scuola['voci'][v]):
                ore.setdefault((t, cella // C // O), set()).add(cella // C % O)
    def a_posto(sue):
        poche, tante = buche_consentite(scuola, len(sue))
        return poche <= max(sue) - min(sue) + 1 - len(sue) <= tante
    return all(a_posto(sue) for sue in ore.values())


def risolvi(scuola, tempo=300):
    """Cerca un orario per la scuola (scuola.py), in al massimo `tempo` secondi. Ritorna (esito, orario, aule,
    secondi): l'esito e' TROVATO, IMPOSSIBILE (dimostrato: un orario non esiste) o LIMITE_DI_TEMPO; l'orario, se c'e',
    ha una voce per cella (-1: ora buca) e `aule` l'aula di ogni cella (-1: ora buca); i secondi sono quelli del
    risolutore.

    In due tempi, perche' la regola sulle ore buche dei docenti rende molto piu' lento dimostrare che un caso e'
    impossibile:
    A. prima senza quella regola. Se gia' cosi' un orario non esiste, non esiste nemmeno con una regola in piu': e' gia'
       la risposta. Se l'orario trovato la rispetta per caso, e' gia' la risposta.
    B. altrimenti da capo, con tutte le regole."""
    partenza = time.time()
    conti = _Conti(scuola, con_le_buche=False)
    esito, orario = _chiedi(conti, tempo)                                                   # A
    if esito == TROVATO and not _buche_a_posto(scuola, orario):
        conti = _Conti(scuola)
        esito, orario = _chiedi(conti, tempo - (time.time() - partenza))                    # B
    aule = conti.aule(orario) if orario else None                                           # passo 6
    return esito, orario, aule, time.time() - partenza
