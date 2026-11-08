"""Verificatore indipendente di un orario: tutte le regole, scritte qui senza usare il solver. Ritorna l'elenco delle
regole violate (vuoto = valido). d e' il record "dati" della traccia (con "ordine" per i solver che decidono le celle in
un altro ordine), orario l'elenco [docente1, docente2] per cella, nell'ordine della ricerca."""


def verifica(d, orario):
    """Le regole violate dall'orario (vuoto = valido). orario: nell'ordine della ricerca; d['ordine'] le celle."""
    G, O, C = len(d['giorni']), len(d['ore']), len(d['classi'])
    M = d['parametri']['ore_obbligatorie']
    MAXO, MAXC = d['parametri']['max_ore_prof_in_una_classe'], d['parametri']['max_classi_diverse_al_giorno']
    ordine = d.get('ordine') or list(range(G * O * C))
    cella = {}
    for i, x in enumerate(orario): cella[ordine[i]] = tuple(x)
    errori = []
    if len(cella) != G * O * C: errori.append('celle mancanti: %d su %d' % (len(cella), G * O * C))
    pos = lambda g, o, c: (g * O + o) * C + c
    ore, solo, coppie = {}, {}, {}
    for g in range(G):
        for o in range(O):
            visti = set()
            for c in range(C):
                p1, p2 = cella.get(pos(g, o, c), (-1, -1))
                for p in (p1, p2):
                    if p < 0: continue
                    if p in visti: errori.append('docente %d in due classi: g%d o%d' % (p, g, o))
                    visti.add(p)
                    ore[c, p] = ore.get((c, p), 0) + 1
                    if o < d['docenti'][p]['ingresso']: errori.append('ingresso: docente %d g%d o%d' % (p, g, o))
                if p1 >= 0 and p2 >= 0:
                    k = (c, min(p1, p2), max(p1, p2)); coppie[k] = coppie.get(k, 0) + 1
                elif p1 >= 0 or p2 >= 0:
                    p = max(p1, p2); solo[c, p] = solo.get((c, p), 0) + 1
        for c in range(C):
            buca = False
            for o in range(O):
                p1, p2 = cella.get(pos(g, o, c), (-1, -1))
                vuota = p1 < 0 and p2 < 0
                if vuota and o < M: errori.append('ora obbligatoria vuota: g%d o%d c%d' % (g, o, c))
                if not vuota and buca: errori.append('lezione dopo un\'ora buca: g%d o%d c%d' % (g, o, c))
                buca |= vuota
            # ore del docente nella classe: al massimo MAXO, contigue
            per = {}
            for o in range(O):
                for p in cella.get(pos(g, o, c), (-1, -1)):
                    if p >= 0: per.setdefault(p, []).append(o)
            for p, hh in per.items():
                if len(hh) > MAXO: errori.append('più di %d ore: docente %d g%d c%d' % (MAXO, p, g, c))
                if hh != list(range(hh[0], hh[0] + len(hh))): errori.append('ore non contigue: docente %d g%d c%d' % (p, g, c))
        classi_oggi = {}
        for o in range(O):
            for c in range(C):
                for p in cella.get(pos(g, o, c), (-1, -1)):
                    if p >= 0: classi_oggi.setdefault(p, set()).add(c)
        for p, cc in classi_oggi.items():
            if len(cc) > MAXC: errori.append('più di %d classi: docente %d g%d' % (MAXC, p, g))
    for x in d['cattedre']:
        c, p = x['classe'], x['docente']
        if ore.get((c, p), 0) != x['ore']: errori.append('ore c%d d%d: %d invece di %d' % (c, p, ore.get((c, p), 0), x['ore']))
        if solo.get((c, p), 0) != x['da_solo']: errori.append('da solo c%d d%d: %d invece di %d' % (c, p, solo.get((c, p), 0), x['da_solo']))
        for k in x['codocenze']:
            if p < k['con'] and coppie.get((c, p, k['con']), 0) != k['ore']:
                errori.append('insieme c%d d%d+%d: %d invece di %d' % (c, p, k['con'], coppie.get((c, p, k['con']), 0), k['ore']))
    previste = {(x['classe'], x['docente']) for x in d['cattedre']}
    for k in ore:
        if k not in previste: errori.append('docente %d in classe %d senza cattedra' % (k[1], k[0]))
    return errori
