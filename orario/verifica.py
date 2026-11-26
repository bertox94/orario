"""La verifica di un orario finito, indipendente dal metodo che lo ha fatto: legge solo la scuola e le celle e
ricontrolla da capo ogni regola di scuola.py. Se il metodo sbagliasse, qui si vedrebbe.

Come guarda: un giorno alla volta, e dentro il giorno un'ora alla volta e una classe alla volta. Le regole che
riguardano una cella (prime ore, ora buca, ingresso, una classe per volta) si controllano li'; quelle sulla giornata
di un docente (ore al giorno, di seguito, classi al giorno, ore buche) alla fine di ogni giorno; le ore di ogni voce alla fine.
"""
from .scuola import docenti_della_voce

# le regole, nell'ordine in cui compaiono nel risultato
(ORE_GIUSTE, UNA_CLASSE_PER_VOLTA, DOPO_L_INGRESSO, ORE_AL_GIORNO, DI_SEGUITO, CLASSI_AL_GIORNO, PRIME_ORE_PIENE,
 DOPO_LA_BUCA, BUCHE_DEL_DOCENTE) = range(9)
REGOLE = ('ogni docente e ogni coppia fanno in ogni classe le loro ore',
          'un docente e\' in una sola classe per volta',
          'nessuna lezione prima dell\'ingresso del docente',
          'in una classe, in un giorno, un docente fa al massimo le ore consentite',
          'le ore di un docente in una classe, in un giorno, sono di seguito',
          'un docente insegna in al massimo le classi consentite al giorno',
          'le prime ore di ogni giorno hanno lezione in ogni classe',
          'dopo un\'ora buca la classe quel giorno non ha piu\' lezione',
          'un docente ha al massimo le ore buche consentite al giorno')


def verifica(scuola, orario):
    """Ricontrolla l'orario (una voce per cella; un numero negativo e' l'ora buca). Ritorna
    {"ok": 0 | 1, "controlli": [[regola, 0 | 1, dove], ...]}: per ogni regola se e' rispettata e, se non lo e', il
    primo punto in cui non lo e'."""
    giorni, ore, classi, docenti, voci = (scuola[x] for x in ('giorni', 'ore', 'classi', 'docenti', 'voci'))
    G, O, C, T = len(giorni), len(ore), len(classi), len(docenti)
    controlli = [[regola, 1, ''] for regola in REGOLE]

    def non_rispettata(regola, dove):
        # si scrive solo il primo punto in cui la regola non e' rispettata
        if controlli[regola][1]:
            controlli[regola][2] = dove
        controlli[regola][1] = 0

    fatte = [0] * len(voci)   # le ore di ogni voce nell'orario
    # PASSO 1: un giorno alla volta
    for g in range(G):
        in_classe = {}        # nel giorno, per (docente, classe): le ore in cui c'e'
        # PASSO 1a: cella per cella, le regole che si vedono guardando una cella
        for o in range(O):
            dove_e = {}       # in quest'ora: la classe dove ogni docente insegna
            for k in range(C):
                v = orario[(g * O + o) * C + k]
                buca_prima = o > 0 and orario[(g * O + o - 1) * C + k] < 0
                qui = '%s %s, %s' % (giorni[g], ore[o], classi[k])
                if v < 0:     # un'ora buca: non deve essere una delle prime ore
                    if o < scuola['obbligatorie']:
                        non_rispettata(PRIME_ORE_PIENE, '%s: non c\'e\' lezione' % qui)
                    continue
                # una lezione: deve essere una voce che esiste, e di questa classe
                if v >= len(voci) or voci[v][0] != k:
                    non_rispettata(ORE_GIUSTE, '%s: una lezione che non e\' di questa classe' % qui)
                    continue
                fatte[v] += 1
                if buca_prima:
                    non_rispettata(DOPO_LA_BUCA, '%s: c\'e\' lezione dopo un\'ora buca' % qui)
                # i docenti della lezione: uno alla volta in una classe sola, non prima dell'ingresso; e si segna
                # l'ora per i conti di fine giornata
                for t in docenti_della_voce(voci[v]):
                    if t in dove_e:
                        non_rispettata(UNA_CLASSE_PER_VOLTA, '%s %s: %s e\' in %s e in %s' % (
                            giorni[g], ore[o], docenti[t][0], classi[dove_e[t]], classi[k]))
                    dove_e[t] = k
                    if o < docenti[t][1]:
                        non_rispettata(DOPO_L_INGRESSO, '%s: %s c\'e\', ma entra alle %s' % (
                            qui, docenti[t][0], ore[docenti[t][1]]))
                    in_classe.setdefault((t, k), []).append(o)
        # PASSO 1b: a giorno finito, la giornata di ogni docente: classe per classe quante ore e se sono di seguito,
        # poi quante classi e quante ore buche
        for t in range(T):
            quante_classi = 0
            sue_ore = set()   # le ore del giorno in cui insegna, in qualunque classe
            for k in range(C):
                sue = sorted(set(in_classe.get((t, k), ())))
                if not sue:
                    continue
                quante_classi += 1
                sue_ore.update(sue)
                if len(sue) > scuola['ore_di_seguito']:
                    non_rispettata(ORE_AL_GIORNO, '%s in %s, %s: %d ore, il massimo e\' %d' % (
                        docenti[t][0], classi[k], giorni[g], len(sue), scuola['ore_di_seguito']))
                # di seguito: tra la prima e l'ultima ci sono solo loro
                if sue[-1] - sue[0] + 1 != len(sue):
                    non_rispettata(DI_SEGUITO, '%s in %s, %s: %d ore tra le %s e le %s, non di seguito' % (
                        docenti[t][0], classi[k], giorni[g], len(sue), ore[sue[0]], ore[sue[-1]]))
            if quante_classi > scuola['classi_al_giorno']:
                non_rispettata(CLASSI_AL_GIORNO, '%s, %s: %d classi, il massimo e\' %d' % (
                    docenti[t][0], giorni[g], quante_classi, scuola['classi_al_giorno']))
            # le ore buche: tra la prima e l'ultima lezione del giorno, le ore in cui non insegna
            buche = max(sue_ore) - min(sue_ore) + 1 - len(sue_ore) if sue_ore else 0
            if buche > scuola['buche_al_giorno']:
                non_rispettata(BUCHE_DEL_DOCENTE, '%s, %s: %d ore buche, il massimo e\' %d' % (
                    docenti[t][0], giorni[g], buche, scuola['buche_al_giorno']))
    # PASSO 2: a settimana finita, ogni voce: nell'orario ci sono esattamente le sue ore
    for v, (k, a, b, quante) in enumerate(voci):
        if fatte[v] != quante:
            chi = docenti[a][0] + (' con ' + docenti[b][0] if b >= 0 else '')
            non_rispettata(ORE_GIUSTE, '%s in %s: %d ore nell\'orario, devono essere %d' % (
                chi, classi[k], fatte[v], quante))
    return {'ok': int(all(c[1] for c in controlli)), 'controlli': controlli}
