"""La scuola: giorni, ore, classi, docenti e le lezioni da mettere in orario; e la lettura di un caso da file.

LE PAROLE
- cella: un posto dell'orario, cioe' una classe in un'ora di un giorno. Le celle sono numerate un'ora dopo l'altra, e
  dentro l'ora una classe dopo l'altra: (giorno * ore + ora) * classi + classe.
- voce: una lezione di una classe: un docente da solo, oppure una coppia (docente e codocente insieme), con le ore che
  devono fare in una settimana. In una cella va una voce, oppure un'ora buca.
- ingresso: l'ora (indice, da 0) da cui un docente e' a scuola; prima non puo' insegnare. Vale per tutti i giorni.
- ore obbligatorie: le prime ore di ogni giorno, che in ogni classe hanno sempre lezione.

LE REGOLE DI UN ORARIO (le controlla verifica.py; esatto.py le scrive come conti)
1. ogni voce fa esattamente le sue ore, nella sua classe;
2. un docente e' in una sola classe per volta;
3. un docente non insegna prima della sua ora di ingresso;
4. in una classe, in un giorno, un docente fa al massimo ORE_DI_SEGUITO ore;
5. e le fa di seguito, senza buchi;
6. in un giorno un docente insegna in al massimo CLASSI_AL_GIORNO classi;
7. in ogni classe le prime ore di ogni giorno (le obbligatorie) hanno lezione;
8. dopo un'ora buca la classe quel giorno non ha piu' lezione.
(Le regole 4 e 5 contano le ore di un docente in una classe sommando tutte le sue voci li', da solo e in coppia.)

LA SCUOLA e' un dizionario:
    {"giorni": [nomi], "ore": [nomi], "classi": [nomi], "obbligatorie": N,
     "ore_di_seguito": 3, "classi_al_giorno": 3,
     "docenti": [[nome, ingresso], ...],
     "voci": [[classe, docente, codocente o -1, ore], ...]}
Classi e docenti sono numerati nell'ordine in cui compaiono nel file. Le voci sono in ordine di classe, poi di
docente, poi di codocente; in una coppia il docente con il numero piu' basso viene per primo, cosi' la stessa coppia
nella stessa classe e' sempre la stessa voce.

IL FILE DI UN CASO: testo, una riga per cosa, con i campi separati da TAB (le righe vuote e quelle che cominciano con
# si saltano):
    giorni  Lun  Mar  ...            i nomi dei giorni, nell'ordine
    ore  08  09  ...                 i nomi delle ore
    obbligatorie  5                  quante prime ore hanno sempre lezione
    docente  NOME  INGRESSO          l'ingresso e' l'indice dell'ora (0: dalla prima)
    lezione  CLASSE  ORE  DOCENTE [CODOCENTE]
Una classe nasce la prima volta che compare in una lezione; i docenti di una lezione devono essere gia' stati
dichiarati; piu' righe con la stessa classe e gli stessi docenti si sommano in una voce sola.
"""

# quanto puo' essere grande una scuola: un caso che supera questi numeri viene rifiutato
MAX_GIORNI, MAX_ORE, MAX_CLASSI, MAX_DOCENTI, MAX_VOCI, MAX_NOME = 7, 10, 120, 250, 4000, 47
# le due regole della giornata di un docente
ORE_DI_SEGUITO, CLASSI_AL_GIORNO = 3, 3
BUCA = -1   # in una cella dell'orario: l'ora buca


def celle(scuola):
    """Quante celle ha l'orario della scuola."""
    return len(scuola['giorni']) * len(scuola['ore']) * len(scuola['classi'])


def docenti_della_voce(voce):
    """I docenti di una voce: uno, o i due della coppia."""
    return [t for t in voce[1:3] if t >= 0]


def _numero(testo):
    """Il numero intero con cui il testo comincia (0 se non comincia con un numero)."""
    testo = testo.strip()
    fine = 1 if testo[:1] in '+-' else 0
    while fine < len(testo) and testo[fine].isdigit():
        fine += 1
    try:
        return int(testo[:fine])
    except ValueError:
        return 0


def _posto(nomi, nome, massimo, aggiungi):
    """Il numero di un nome in un elenco, aggiungendolo in fondo se manca (-1: non c'e', l'elenco e' pieno, o il nome
    e' troppo lungo)."""
    if nome in nomi:
        return nomi.index(nome)
    if not aggiungi or len(nomi) == massimo or len(nome.encode()) > MAX_NOME:
        return -1
    nomi.append(nome)
    return len(nomi) - 1


def leggi(percorso):
    """Legge un caso da file e ritorna la scuola. ValueError, con il motivo, se il file non si legge o non e' scritto
    bene."""
    try:
        with open(percorso, encoding='utf-8') as f:
            righe = f.read().split('\n')
    except OSError:
        raise ValueError('non riesco a leggere %s' % percorso)
    giorni, ore, classi, nomi_docenti, ingresso, obbligatorie = [], [], [], [], [], 0
    voci = {}   # (classe, docente, codocente) -> ore
    for numero, riga in enumerate(righe, 1):
        campi = [c for c in riga.replace('\r', '\t').split('\t') if c]   # i campi, saltando quelli vuoti
        if not campi or campi[0].startswith('#'):
            continue
        n, ok = len(campi), True
        if campi[0] == 'giorni':
            ok = all(_posto(giorni, c, MAX_GIORNI, True) >= 0 for c in campi[1:])
        elif campi[0] == 'ore':
            ok = all(_posto(ore, c, MAX_ORE, True) >= 0 for c in campi[1:])
        elif campi[0] == 'obbligatorie' and n == 2:
            obbligatorie = _numero(campi[1])
        elif campi[0] == 'docente' and n == 3:
            t = _posto(nomi_docenti, campi[1], MAX_DOCENTI, True)
            ok = t >= 0
            if ok:
                if t == len(ingresso):
                    ingresso.append(0)
                ingresso[t] = _numero(campi[2])
        elif campi[0] == 'lezione' and n in (4, 5):
            # classe, ore, docente e forse codocente: i due di una coppia devono essere diversi
            k = _posto(classi, campi[1], MAX_CLASSI, True)
            a = _posto(nomi_docenti, campi[3], MAX_DOCENTI, False)
            b = _posto(nomi_docenti, campi[4], MAX_DOCENTI, False) if n == 5 else -1
            quante = _numero(campi[2])
            ok = k >= 0 and a >= 0 and (n == 4 or (b >= 0 and b != a)) and quante > 0
            if ok:
                if 0 <= b < a:   # la coppia in ordine di numero
                    a, b = b, a
                if (k, a, b) not in voci and len(voci) == MAX_VOCI:
                    ok = False
                else:
                    voci[(k, a, b)] = voci.get((k, a, b), 0) + quante
        else:
            ok = False
        if not ok:
            raise ValueError('%s, riga %d: non la capisco, o supera i limiti di scuola.py' % (percorso, numero))
    if not giorni or not ore or not classi or not voci:
        raise ValueError('%s: mancano giorni, ore o lezioni' % percorso)
    for t, nome in enumerate(nomi_docenti):
        if not 0 <= ingresso[t] < len(ore):
            raise ValueError('%s: l\'ingresso di %s non e\' un\'ora della giornata' % (percorso, nome))
    return {'giorni': giorni, 'ore': ore, 'classi': classi, 'obbligatorie': max(0, min(obbligatorie, len(ore))),
            'ore_di_seguito': ORE_DI_SEGUITO, 'classi_al_giorno': CLASSI_AL_GIORNO,
            'docenti': [[nome, ingresso[t]] for t, nome in enumerate(nomi_docenti)],
            'voci': [[k, a, b, quante] for (k, a, b), quante in sorted(voci.items())]}
