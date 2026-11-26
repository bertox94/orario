"""Casi casuali, riproducibili: lo stesso seme, lo stesso numero di classi e la stessa scelta sui ritardi danno sempre
lo stesso caso. (Sono gli stessi casi del generatore in C che c'era prima: controllato caso per caso.)

Sei giorni di sei ore, le prime cinque con lezione. Ogni classe ha da 30 a 36 ore, divise in lezioni da 1 a 6 ore;
una parte delle lezioni ha anche un codocente, per alcune delle sue ore. I docenti sono tanti da avere in media tra
10 e 20 ore l'uno, e ogni lezione va al meno carico fra tre presi a caso. Con i ritardi alcuni docenti entrano alla
seconda, alla terza o alla quarta ora: un caso con i ritardi puo' non avere nessun orario.

Le materie e le aule (dal 26 novembre 2026, richiesta di Alb) si aggiungono SOPRA al caso, con numeri casuali a parte:
classi, docenti, lezioni e ritardi di ogni seme sono quelli di prima. Ogni docente ha una cattedra, di una o due
materie; ogni lezione prende una delle materie del suo docente. Ogni classe ha la sua aula normale; laboratori e
palestre sono tanti da essere occupati, in media, per ORE_AULA_SPECIALE ore la settimana: possono non bastare.
Allo stesso modo, con altri numeri casuali a parte, i giorni liberi (in tutti i casi) e le uscite prima della fine
(solo nei casi con i ritardi): vedi _giorni_liberi_e_uscite.

ATTENZIONE: i test rigenerano i casi dai semi. Cambiare qualunque cosa qui (anche solo l'ordine in cui si chiedono i
numeri casuali: una chiamata in piu' o in meno, o spostata) cambia tutti i casi.
"""
from .nomi import NOMI_DOCENTI
from .scuola import MAX_CLASSI, MAX_DOCENTI, NESSUNO, QUALUNQUE

GIORNI, ORE, OBBLIGATORIE = 6, 6, 5
NOMI_GIORNI = ('Lun', 'Mar', 'Mer', 'Gio', 'Ven', 'Sab')
ORE_CLASSE_MIN, ORE_CLASSE_MAX = 30, 36    # le ore di una classe in una settimana
ORE_DOCENTE_MIN, ORE_DOCENTE_MAX = 5, 24   # le ore di un docente in una settimana
INGRESSO_MAX = 3                           # l'ingresso piu' tardo: la quarta ora
MAX_LEZIONI = 4000
TENTATIVI = 20000                          # quante volte si riprova prima di dire che il caso non si riesce a generare
_64 = (1 << 64) - 1                        # i conti sui numeri casuali si fanno su 64 bit

# le cattedre: le materie che un docente puo' insegnare, una o due. Una cattedra scritta due volte esce il doppio.
CATTEDRE = (('Italiano', 'Storia'), ('Italiano', 'Storia'), ('Matematica', 'Fisica'), ('Matematica',), ('Inglese',),
            ('Scienze', 'Chimica'), ('Scienze motorie',), ('Informatica',), ('Diritto', 'Economia'), ('Francese',),
            ('Arte',), ('Religione',), ('Storia', 'Filosofia'), ('Tecnologia',))
# il tipo di aula delle materie che non si fanno in un'aula normale, e il nome delle aule di ogni tipo
TIPO_AULA = {'Scienze': 'laboratorio', 'Chimica': 'laboratorio', 'Informatica': 'laboratorio',
             'Tecnologia': 'laboratorio', 'Scienze motorie': 'palestra'}
NOME_AULA = {'normale': 'A%02d', 'laboratorio': 'Lab %d', 'palestra': 'Palestra %d'}
ORE_AULA_SPECIALE = 24                     # un laboratorio o una palestra ogni tante ore di lezione che li chiedono
# giorni liberi e uscite: su cento docenti quanti hanno un giorno libero preciso, quanti uno qualunque, e (nei casi
# con i ritardi, tra chi entra alla prima ora) quanti escono prima della fine
CON_GIORNO_PRECISO, CON_GIORNO_QUALUNQUE, CON_USCITA = 25, 25, 30
MARGINE = 6                                # le ore che a un docente devono avanzare, oltre alle sue, nel tempo che ha


class _Casuale:
    """Una serie di numeri casuali, decisa dal seme. `altro` cambia la serie: con lo stesso seme, due valori diversi
    danno due serie che non c'entrano l'una con l'altra."""

    def __init__(self, seme, altro=0x1234567):
        # Si parte dal seme: lo stato non puo' essere zero (resterebbe zero per sempre), e i primi numeri si buttano
        # perche' con semi vicini si somigliano.
        self.stato = (seme * 0x9E3779B97F4A7C15 + altro) & _64 or 1
        for _ in range(8):
            self.casuale()

    def casuale(self):
        """Il prossimo numero casuale (xorshift64*): tre spostamenti di bit sullo stato, e una moltiplicazione."""
        x = self.stato
        x ^= x >> 12
        x ^= (x << 25) & _64
        x ^= x >> 27
        self.stato = x
        return (x * 2685821657736338717) & _64

    def tra(self, da, a):
        """Un numero tra `da` e `a`, compresi."""
        return da + self.casuale() % (a - da + 1)


class _Caso(_Casuale):
    """Un caso in costruzione: i suoi numeri casuali, le lezioni, i docenti e il loro carico di ore."""

    def __init__(self, seme):
        super().__init__(seme)
        self.lezioni, self.docenti, self.carico = [], 0, []

    def il_meno_carico(self, ore, escluso):
        """Il meno carico fra tre docenti presi a caso, tra quelli che con queste ore non superano il massimo (-1:
        nessuno). `escluso` e' il docente della stessa lezione, quando si cerca il codocente."""
        scelto = -1
        for _ in range(3):
            d = self.tra(0, self.docenti - 1)
            if d == escluso or self.carico[d] + ore > ORE_DOCENTE_MAX:
                continue
            if scelto < 0 or self.carico[d] < self.carico[scelto]:
                scelto = d
        return scelto

    def tenta(self, classi):
        """Prova a generare un caso: le lezioni di ogni classe, poi quanti docenti, poi un docente per ogni lezione.
        False se il tentativo non riesce (un docente con troppe o troppo poche ore, o niente lezioni in coppia o da
        soli). Una lezione e' [classe, docente, ore, codocente, ore insieme]."""
        percento_in_coppia = self.tra(15, 60)   # quante lezioni su cento hanno anche un codocente
        ore_in_tutto = 0                        # le ore di tutti i docenti, codocenti compresi
        self.lezioni = []
        # le lezioni di ogni classe: si tagliano le sue ore in pezzi da 1 a 6, finche' non restano ore
        for k in range(classi):
            resto = self.tra(ORE_CLASSE_MIN, ORE_CLASSE_MAX)
            while resto > 0:
                if len(self.lezioni) == MAX_LEZIONI:
                    return False
                ore = min(self.tra(1, 6), resto)
                resto -= ore
                lezione = [k, -1, ore, -1, 0]
                # in coppia: per alcune delle sue ore (da una a tutte) c'e' anche il codocente, che si sceglie dopo
                if self.tra(1, 100) <= percento_in_coppia:
                    lezione[4] = self.tra(1, ore)
                self.lezioni.append(lezione)
                ore_in_tutto += ore + lezione[4]
        # i docenti: tanti da avere in media tra 10 e 20 ore, e comunque tra il minimo e il massimo
        pochi = (ore_in_tutto + ORE_DOCENTE_MAX - 1) // ORE_DOCENTE_MAX
        tanti = ore_in_tutto // ORE_DOCENTE_MIN
        da, a = max((ore_in_tutto + 19) // 20, pochi), min(ore_in_tutto // 10, tanti)
        if da > a:
            da, a = pochi, tanti
        if da > a or a >= MAX_DOCENTI:
            return False
        self.docenti = self.tra(da, a)
        self.carico = [0] * self.docenti
        # le lezioni in ordine casuale: quella al posto i si scambia con una a caso tra lei e l'ultima
        for i in range(len(self.lezioni)):
            j = self.tra(i, len(self.lezioni) - 1)
            self.lezioni[i], self.lezioni[j] = self.lezioni[j], self.lezioni[i]
        # un docente per ogni lezione (e un codocente, diverso, per quelle in coppia)
        in_coppia = da_soli = False
        for lezione in self.lezioni:
            lezione[1] = self.il_meno_carico(lezione[2], -1)
            if lezione[1] < 0:
                return False
            self.carico[lezione[1]] += lezione[2]
            if not lezione[4]:
                da_soli = True
                continue
            lezione[3] = self.il_meno_carico(lezione[4], lezione[1])
            if lezione[3] < 0:
                return False
            self.carico[lezione[3]] += lezione[4]
            in_coppia = True
        # il tentativo vale se nessun docente ha troppo poche ore, e ci sono lezioni di tutti e due i tipi
        return all(c >= ORE_DOCENTE_MIN for c in self.carico) and in_coppia and da_soli

    def ritardi(self):
        """Le ore di ingresso: quanti docenti entrano tardi (da uno a tutti) e quali; ognuno alla seconda, terza o
        quarta ora, anticipata finche' le sue ore stanno nelle ore che gli restano nella settimana."""
        ordine, tardi, ingresso = list(range(self.docenti)), [False] * self.docenti, [0] * self.docenti
        quanti = self.tra(1, self.docenti)
        # i primi `quanti` di un ordine casuale: a ogni posto si scambia con uno a caso tra quelli che restano
        for i in range(quanti):
            j = self.tra(i, self.docenti - 1)
            ordine[i], ordine[j] = ordine[j], ordine[i]
            tardi[ordine[i]] = True
        for d in range(self.docenti):
            if tardi[d]:
                ingresso[d] = self.tra(1, INGRESSO_MAX)
                while ingresso[d] > 0 and self.carico[d] > (ORE - ingresso[d]) * GIORNI:
                    ingresso[d] -= 1
        return ingresso


def _nome(d):
    """Il nome del docente d: dall'elenco di nomi.py, o D e il suo numero se l'elenco e' finito."""
    return NOMI_DOCENTI[d] if d < len(NOMI_DOCENTI) else 'D%03d' % (d + 1)


def _materie_e_aule(seme, caso, classi):
    """Le materie delle lezioni e le aule della scuola, per un caso gia' fatto. Usa una serie di numeri casuali sua:
    cosi' il resto del caso (e i ritardi) non cambia. Ritorna (la materia di ogni lezione, nell'ordine di
    caso.lezioni; le materie usate, in ordine di nome; le aule, come [nome, tipo])."""
    c = _Casuale(seme, 0x7A5C3E1)
    # 1. una cattedra per docente
    cattedra = [CATTEDRE[c.tra(0, len(CATTEDRE) - 1)] for _ in range(caso.docenti)]
    # 2. ogni lezione prende una delle materie della cattedra del suo docente (in coppia: del primo dei due)
    della_lezione = []
    for lezione in caso.lezioni:
        sue = cattedra[lezione[1]]
        della_lezione.append(sue[c.tra(0, len(sue) - 1)])
    # 3. le aule: una normale per classe; laboratori e palestre secondo le ore di lezione che li chiedono
    ore = {}   # tipo di aula -> ore di lezione la settimana
    for lezione, materia in zip(caso.lezioni, della_lezione):
        tipo = TIPO_AULA.get(materia, 'normale')
        ore[tipo] = ore.get(tipo, 0) + lezione[2]
    aule = [[NOME_AULA['normale'] % (k + 1), 'normale'] for k in range(classi)]
    for tipo in sorted(t for t in ore if t != 'normale'):
        quante = (ore[tipo] + ORE_AULA_SPECIALE - 1) // ORE_AULA_SPECIALE
        aule += [[NOME_AULA[tipo] % (i + 1), tipo] for i in range(quante)]
    return della_lezione, sorted(set(della_lezione)), aule


def _giorni_liberi_e_uscite(seme, caso, ingresso, con_ritardi):
    """Il giorno libero e l'ora di uscita di ogni docente, per un caso gia' fatto. Usa una serie di numeri casuali sua.
    Un docente li ha solo se il tempo che gli resta basta con un margine: le sue ore piu' MARGINE devono stare nelle
    ore in cui e' a scuola. Ritorna (i giorni liberi, le uscite)."""
    c = _Casuale(seme, 0x51F15E7)
    liberi, uscite = [], []
    for d in range(caso.docenti):
        # quattro numeri per docente, sempre, anche quando non servono: cosi' la serie non dipende dalle scelte
        sorte, giorno, sorte_uscita, uscita = c.tra(1, 100), c.tra(0, GIORNI - 1), c.tra(1, 100), c.tra(4, ORE - 1)
        al_giorno = ORE - ingresso[d]   # le ore di un giorno in cui e' a scuola
        # il giorno libero: se le sue ore stanno, con il margine, in un giorno di meno
        libero = NESSUNO
        if sorte <= CON_GIORNO_PRECISO + CON_GIORNO_QUALUNQUE and \
                caso.carico[d] + MARGINE <= al_giorno * (GIORNI - 1):
            libero = giorno if sorte <= CON_GIORNO_PRECISO else QUALUNQUE
        # l'uscita: solo con i ritardi, solo per chi entra alla prima ora; ritardata finche' le sue ore ci stanno
        giorni = GIORNI - (libero != NESSUNO)
        if not (con_ritardi and ingresso[d] == 0 and sorte_uscita <= CON_USCITA):
            uscita = ORE
        while uscita < ORE and caso.carico[d] + MARGINE > uscita * giorni:
            uscita += 1
        liberi.append(libero)
        uscite.append(uscita)
    return liberi, uscite


def genera(seme, classi, con_ritardi):
    """Il testo di un caso casuale, nel formato che scuola.leggi legge. ValueError se non si riesce a generarlo: si
    tenta finche' un tentativo riesce (ogni tentativo continua con i numeri casuali dove il precedente si e'
    fermato)."""
    if not 1 <= classi <= MAX_CLASSI or seme < 0:
        raise ValueError('Seme da 0 in su, classi da 1 a %d.' % MAX_CLASSI)
    # Con tante classi i docenti sarebbero sempre troppi (almeno 30 ore per classe, quindi almeno 3 docenti per
    # classe: da 84 classi in su si supera il massimo) e nessun tentativo puo' riuscire: lo si dice subito, senza
    # farne ventimila. Il risultato e' lo stesso.
    if classi * ORE_CLASSE_MIN // 10 >= MAX_DOCENTI:
        raise ValueError('Con questo seme e queste classi il caso non si riesce a generare.')
    c = _Caso(seme)
    if not any(c.tenta(classi) for _ in range(TENTATIVI)):
        raise ValueError('Con questo seme e queste classi il caso non si riesce a generare.')
    ingresso = c.ritardi() if con_ritardi else [0] * c.docenti
    righe = ['# seme %d, %d classi, %s ritardi' % (seme, classi, 'con' if con_ritardi else 'senza'),
             'giorni\t' + '\t'.join(NOMI_GIORNI), 'ore\t08\t09\t10\t11\t12\t13', 'obbligatorie\t%d' % OBBLIGATORIE]
    materia_di, materie, aule = _materie_e_aule(seme, c, classi)
    righe += ['materia\t%s\t%s' % (m, TIPO_AULA.get(m, 'normale')) for m in materie]
    righe += ['aula\t%s\t%s' % (nome, tipo) for nome, tipo in aule]
    liberi, uscite = _giorni_liberi_e_uscite(seme, c, ingresso, con_ritardi)
    nome_del_giorno = {NESSUNO: 'nessuno', QUALUNQUE: 'qualunque', **dict(enumerate(NOMI_GIORNI))}
    righe += ['docente\t%s\t%d\t%d\t%s' % (_nome(d), ingresso[d], uscite[d], nome_del_giorno[liberi[d]])
              for d in range(c.docenti)]
    # classe per classe; di ogni lezione le ore da solo e quelle in coppia, con la sua materia
    for k in range(classi):
        for (classe, docente, ore, codocente, insieme), materia in zip(c.lezioni, materia_di):
            if classe != k:
                continue
            if ore > insieme:
                righe.append('lezione\tC%02d\t%d\t%s\t%s' % (k + 1, ore - insieme, materia, _nome(docente)))
            if insieme:
                righe.append('lezione\tC%02d\t%d\t%s\t%s\t%s'
                             % (k + 1, insieme, materia, _nome(docente), _nome(codocente)))
    return '\n'.join(righe) + '\n'
