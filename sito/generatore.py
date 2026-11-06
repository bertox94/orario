"""Dati di prova casuali, riproducibili da un seme: stesso seme, stessi dati, su qualunque computer (i numeri casuali
vengono da xorshift64*, non da random di Python).

Come si generano. Ogni classe ha da 30 a 36 ore, divise in lezioni da 1 a 6 ore; una parte delle lezioni (tra il 15% e
il 60%, diversa a ogni tentativo) ha anche un codocente per alcune di quelle ore. Il numero di docenti si sceglie
perche' in media abbiano tra 10 e 20 ore; ogni lezione va al meno carico fra tre docenti presi a caso. Se un docente
resta con meno di 5 ore o ne avrebbe piu' di 24, o se mancano lezioni da soli o in coppia, si riprova da capo.

Con i ritardi, circa un docente su cinque entra alla seconda, terza o quarta ora, abbassata finche' le sue ore stanno
ancora nelle ore che gli restano nella settimana. Nessun altro controllo: un caso con i ritardi puo' anche essere
impossibile, e lo dice la ricerca. Con e senza ritardi i dati di un seme sono gli stessi, tranne le ore di ingresso.

Il risultato e' nel formato di --input del solver (ingresso_uscita/dati_json.h): giorni, ore, lezioni e ore di
ingresso. Cambiare questo file cambia i dati di ogni seme, e quindi i riferimenti dei test notturni.
"""
from .nomi import NOMI

GIORNI = ['Lun', 'Mar', 'Mer', 'Gio', 'Ven', 'Sab']
ORE = ['08', '09', '10', '11', '12', '13']
ORE_OBBLIGATORIE = 5
ORE_CLASSE_MIN, ORE_CLASSE_MAX = 29, 36
ORE_DOCENTE_MIN, ORE_DOCENTE_MAX = 5, 24
ORA_INGRESSO_MAX = 3
MAX_LEZIONI, MAX_TENTATIVI = 4000, 20000
INSEGNANTE_SZ = 250
M64 = (1 << 64) - 1


class _Casuale:
    """xorshift64*: la stessa sequenza di numeri su ogni computer."""

    def __init__(self, seme):
        self.s = (seme * 0x9E3779B97F4A7C15 + 0x1234567) & M64 or 1
        for _ in range(8):
            self.prossimo()

    def prossimo(self):
        s = self.s
        s ^= s >> 12
        s ^= (s << 25) & M64
        s ^= s >> 27
        self.s = s
        return (s * 2685821657736338717) & M64

    def tra(self, lo, hi):
        return lo + self.prossimo() % (hi - lo + 1)


def _nome_docente(d):
    return NOMI[d] if d < len(NOMI) else 'D%03d' % (d + 1)


def _tenta(r, n_classi):
    min_classe = max(ORE_CLASSE_MIN, ORE_OBBLIGATORIE * 6)
    lezioni = []  # [classe, prof, ore, cod, ore_cod]  (cod: -1 senza, None da assegnare)
    p_cod = r.tra(15, 60)
    ore_totali = 0
    for c in range(n_classi):
        resto = r.tra(min_classe, ORE_CLASSE_MAX)
        while resto > 0:
            if len(lezioni) == MAX_LEZIONI:
                return None
            ore = min(r.tra(1, 6), resto)
            resto -= ore
            l = [c, -1, ore, -1, 0]
            lezioni.append(l)
            ore_totali += ore
            if r.tra(1, 100) <= p_cod:
                l[4] = r.tra(1, ore)
                ore_totali += l[4]
                l[3] = None
    lo, hi = (ore_totali + 19) // 20, ore_totali // 10
    abs_lo = (ore_totali + ORE_DOCENTE_MAX - 1) // ORE_DOCENTE_MAX
    abs_hi = ore_totali // ORE_DOCENTE_MIN
    lo, hi = max(lo, abs_lo), min(hi, abs_hi)
    if lo > hi:
        lo, hi = abs_lo, abs_hi
    if lo > hi or hi >= INSEGNANTE_SZ:
        return None
    n_doc = r.tra(lo, hi)
    carico = [0] * n_doc

    def scegli(ore, escluso):
        best = -1
        for _ in range(3):
            d = r.tra(0, n_doc - 1)
            if d == escluso or carico[d] + ore > ORE_DOCENTE_MAX:
                continue
            if best < 0 or carico[d] < carico[best]:
                best = d
        return best

    for i in range(len(lezioni)):
        j = r.tra(i, len(lezioni) - 1)
        lezioni[i], lezioni[j] = lezioni[j], lezioni[i]
    for l in lezioni:
        l[1] = scegli(l[2], -1)
        if l[1] < 0:
            return None
        carico[l[1]] += l[2]
        if l[3] is None:
            l[3] = scegli(l[4], l[1])
            if l[3] < 0:
                return None
            carico[l[3]] += l[4]
    if not any(l[3] >= 0 for l in lezioni) or not any(l[3] < 0 for l in lezioni):
        return None
    if any(c < ORE_DOCENTE_MIN for c in carico):
        return None
    return lezioni, carico


def genera(seme, n_classi=8, ritardi=False):
    """I dati di un caso casuale, nel formato di --input."""
    if not 1 <= n_classi <= 120:
        raise ValueError('classi: da 1 a 120')
    r = _Casuale(seme)
    for _ in range(MAX_TENTATIVI):
        piano = _tenta(r, n_classi)
        if piano:
            break
    else:
        raise ValueError('impossibile generare dati validi per %d classi' % n_classi)
    lezioni, carico = piano
    uscita = []
    for c in range(n_classi):
        for cl, prof, ore, cod, ore_cod in lezioni:
            if cl != c:
                continue
            l = {'classe': 'C%02d' % (c + 1), 'docente': _nome_docente(prof), 'ore': ore}
            if cod >= 0:
                l.update(codocente=_nome_docente(cod), ore_codocenza=ore_cod)
            uscita.append(l)
    ingressi = []
    if ritardi:
        for d in range(len(carico)):
            if r.tra(1, 100) > 20:
                continue
            ingresso = r.tra(1, ORA_INGRESSO_MAX)
            while ingresso > 0 and carico[d] > (len(ORE) - ingresso) * len(GIORNI):
                ingresso -= 1
            if ingresso > 0:
                ingressi.append({'docente': _nome_docente(d), 'ora': ingresso})
    return {'giorni': list(GIORNI), 'ore': list(ORE), 'lezioni': uscita, 'ingressi': ingressi}
