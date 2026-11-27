"""La scuola: giorni, ore, classi, docenti, materie, aule e le lezioni da mettere in orario; e la lettura di un caso
da file.

LE PAROLE
- cella: un posto dell'orario, cioe' una classe in un'ora di un giorno. Le celle sono numerate un'ora dopo l'altra, e
  dentro l'ora una classe dopo l'altra: (giorno * ore + ora) * classi + classe.
- voce: una lezione di una classe: una materia, fatta da un docente da solo oppure da una coppia (docente e codocente
  insieme), con le ore che devono fare in una settimana. In una cella va una voce, oppure un'ora buca.
- materia: quello che si insegna in una lezione. Un docente puo' insegnare piu' materie: nella stessa classe sono voci
  separate. Ogni materia chiede un tipo di aula (normale, laboratorio, palestra...: un nome qualunque).
- aula: ogni lezione si fa in un'aula, del tipo che la sua materia chiede. Tutte le aule della scuola sono contate,
  anche quelle normali.
- ingresso: l'ora (indice, da 0) da cui un docente e' a scuola; prima non puo' insegnare. Vale per tutti i giorni.
- uscita: l'ora (indice) da cui un docente non e' piu' a scuola: insegna solo nelle ore prima. Vale per tutti i
  giorni. Chi resta fino alla fine ha come uscita il numero delle ore.
- giorno libero: un giorno in cui il docente non insegna. Ogni docente ne ha uno: un giorno preciso, oppure, quando
  non e' specificato, «uno qualunque»: quale sia lo sceglie il programma.
- ore obbligatorie: le prime ore di ogni giorno, che in ogni classe hanno sempre lezione.

LE REGOLE DI UN ORARIO (le controlla verifica.py; esatto.py le scrive come conti). Sono otto:
1. LE ORE: ogni voce fa esattamente le sue ore, nella sua classe;
2. UNA CLASSE PER VOLTA: un docente e' in una sola classe per volta;
3. A SCUOLA: un docente insegna solo quando e' a scuola: non prima della sua ora di ingresso, non dalla sua ora di
   uscita in poi, non nel suo giorno libero. Ogni docente ha un giorno libero: chi non ne ha uno preciso ha almeno
   un giorno senza lezioni, uno qualunque;
4. IN UNA CLASSE: in una classe, in un giorno, un docente fa al massimo ORE_DI_SEGUITO ore, e le fa di seguito, senza
   buchi (contando insieme tutte le sue voci li': da solo, in coppia, di tutte le sue materie);
5. LE CLASSI: in un giorno un docente insegna in al massimo CLASSI_AL_GIORNO classi;
6. LA GIORNATA DEL DOCENTE: in un giorno un docente insegna al massimo LEZIONI_AL_GIORNO ore, in tutte le sue classi
   insieme; e le sue ore buche (ore senza lezione tra la sua prima e la sua ultima lezione del giorno: quelle prima
   della prima e dopo l'ultima non contano) sono al massimo BUCHE_AL_GIORNO, e nessuna se quel giorno insegna meno di
   ORE_PER_UNA_BUCA ore (vedi buche_consentite);
7. LA GIORNATA DELLA CLASSE: in ogni classe le prime ore di ogni giorno (le obbligatorie) hanno lezione, e dopo
   un'ora buca la classe quel giorno non ha piu' lezione;
8. LE AULE: ogni lezione si fa in un'aula del tipo che la sua materia chiede, e in un'aula c'e' una classe per volta.
(Fino al 27 novembre 2026 erano scritte come tredici regole: le stesse, una per frase. Sono state raggruppate su
richiesta di Alb; gli orari non cambiano.)

LA SCUOLA e' un dizionario:
    {"giorni": [nomi], "ore": [nomi], "classi": [nomi], "obbligatorie": N,
     "ore_di_seguito": 3, "classi_al_giorno": 3, "buche_al_giorno": 1, "ore_per_una_buca": 4,
     "lezioni_al_giorno": 5,
     "docenti": [[nome, ingresso, uscita, giorno libero], ...],     giorno libero: un giorno, o QUALUNQUE
     "materie": [[nome, tipo di aula], ...], "aule": [[nome, tipo], ...],
     "voci": [[classe, docente, codocente o -1, ore, materia], ...]}
Classi, docenti, materie e aule sono numerati nell'ordine in cui compaiono nel file. Le voci sono in ordine di classe,
poi di docente, di codocente e di materia; in una coppia il docente con il numero piu' basso viene per primo, cosi' la stessa coppia
nella stessa classe, con la stessa materia, e' sempre la stessa voce.

IL FILE DI UN CASO: testo, una riga per cosa, con i campi separati da TAB (le righe vuote e quelle che cominciano con
# si saltano):
    giorni  Lun  Mar  ...            i nomi dei giorni, nell'ordine
    ore  08  09  ...                 i nomi delle ore
    obbligatorie  5                  quante prime ore hanno sempre lezione
    materia  NOME  TIPO              il tipo di aula che la materia chiede
    aula  NOME  TIPO
    docente  NOME  INGRESSO [USCITA [LIBERO]]
                                     ingresso e uscita sono indici di ore (ingresso 0: dalla prima; senza uscita:
                                     fino alla fine); LIBERO e' il nome di un giorno, o «qualunque» (anche se manca)
    lezione  CLASSE  ORE  MATERIA  DOCENTE [CODOCENTE]
Una classe nasce la prima volta che compare in una lezione; la materia e i docenti di una lezione devono essere gia'
stati dichiarati; piu' righe con la stessa classe, la stessa materia e gli stessi docenti si sommano in una voce sola.
Per ogni materia usata in una lezione ci deve essere almeno un'aula del suo tipo.
"""

# quanto puo' essere grande una scuola: un caso che supera questi numeri viene rifiutato
MAX_GIORNI, MAX_ORE, MAX_CLASSI, MAX_DOCENTI, MAX_VOCI, MAX_NOME = 7, 10, 120, 250, 4000, 47
MAX_MATERIE, MAX_AULE = 60, 300
# le regole della giornata di un docente: ore di seguito in una classe, classi, ore buche (al massimo una, e solo
# nei giorni in cui insegna almeno quattro ore: con una, due o tre ore di lezione, nessuna), ore di lezione in tutto
ORE_DI_SEGUITO, CLASSI_AL_GIORNO, BUCHE_AL_GIORNO, ORE_PER_UNA_BUCA, LEZIONI_AL_GIORNO = 3, 3, 1, 4, 5
BUCA = -1   # in una cella dell'orario: l'ora buca
QUALUNQUE = -2   # il giorno libero di un docente, quando non e' un giorno preciso: lo sceglie il programma


def celle(scuola):
    """Quante celle ha l'orario della scuola."""
    return len(scuola['giorni']) * len(scuola['ore']) * len(scuola['classi'])


def buche_consentite(scuola, ore_di_lezione):
    """Quante ore buche puo' avere un docente in un giorno in cui insegna `ore_di_lezione` ore (regola 6)."""
    return scuola['buche_al_giorno'] if ore_di_lezione >= scuola['ore_per_una_buca'] else 0


def docenti_della_voce(voce):
    """I docenti di una voce: uno, o i due della coppia."""
    return [t for t in voce[1:3] if t >= 0]


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
            testo = f.read()
    except OSError:
        raise ValueError('non riesco a leggere %s' % percorso)
    return leggi_testo(testo, percorso)


def leggi_testo(testo, nome='il caso'):
    """La scuola scritta in un testo (il contenuto di un file di un caso). ValueError, con il motivo, se non e'
    scritto bene; `nome` serve solo ai messaggi."""
    giorni, ore, classi, docenti, ingresso, obbligatorie = [], [], [], [], [], 0
    uscita, libero = [], []   # (l'uscita None: fino alla fine; il giorno libero ancora come e' scritto)
    materie, tipo_materia, aule, tipo_aula = [], [], [], []
    voci = {}   # (classe, docente, codocente, materia) -> ore

    # PASSO 1: una riga alla volta. La prima parola dice che cosa c'e' nella riga.
    for numero, riga in enumerate(testo.split('\n'), 1):
        campi = [c for c in riga.replace('\r', '\t').split('\t') if c]   # i campi, saltando quelli vuoti
        if not campi or campi[0].startswith('#'):
            continue
        cosa, n = campi[0], len(campi)
        try:
            if cosa == 'giorni':
                ok = all(_posto(giorni, c, MAX_GIORNI, True) >= 0 for c in campi[1:])
            elif cosa == 'ore':
                ok = all(_posto(ore, c, MAX_ORE, True) >= 0 for c in campi[1:])
            elif cosa == 'obbligatorie' and n == 2:
                obbligatorie, ok = int(campi[1]), True
            elif cosa in ('materia', 'aula') and n == 3:
                # una materia o un'aula, con il suo tipo (ridichiararla cambia il tipo)
                nomi, tipi, massimo = (materie, tipo_materia, MAX_MATERIE) if cosa == 'materia' else \
                                      (aule, tipo_aula, MAX_AULE)
                i = _posto(nomi, campi[1], massimo, True)
                ok = i >= 0 and len(campi[2].encode()) <= MAX_NOME
                if ok:
                    tipi[i:i + 1] = [campi[2]]
            elif cosa == 'docente' and n in (3, 4, 5):
                # un docente nuovo, oppure uno gia' visto a cui si cambiano ingresso, uscita e giorno libero
                t = _posto(docenti, campi[1], MAX_DOCENTI, True)
                ok = t >= 0
                if ok:
                    ingresso[t:t + 1] = [int(campi[2])]
                    uscita[t:t + 1] = [int(campi[3]) if n > 3 else None]
                    libero[t:t + 1] = [campi[4] if n > 4 else 'qualunque']
            elif cosa == 'lezione' and n in (5, 6):
                # classe, ore, materia, docente e forse codocente: i due di una coppia devono essere diversi
                k = _posto(classi, campi[1], MAX_CLASSI, True)
                m = _posto(materie, campi[3], MAX_MATERIE, False)
                a = _posto(docenti, campi[4], MAX_DOCENTI, False)
                b = _posto(docenti, campi[5], MAX_DOCENTI, False) if n == 6 else -1
                quante = int(campi[2])
                ok = k >= 0 and m >= 0 and a >= 0 and (n == 5 or (b >= 0 and b != a)) and quante > 0
                if ok:
                    if 0 <= b < a:   # la coppia in ordine di numero: e' sempre la stessa voce
                        a, b = b, a
                    ok = (k, a, b, m) in voci or len(voci) < MAX_VOCI
                    if ok:
                        voci[(k, a, b, m)] = voci.get((k, a, b, m), 0) + quante   # righe uguali si sommano
            else:
                ok = False
        except ValueError:   # dove ci voleva un numero c'e' altro
            ok = False
        if not ok:
            raise ValueError('%s, riga %d: non la capisco, o supera i limiti di scuola.py' % (nome, numero))

    # PASSO 2: il caso deve essere intero, ogni ingresso deve essere un'ora della giornata, e per ogni materia che si
    # insegna ci deve essere almeno un'aula del suo tipo
    if not giorni or not ore or not classi or not voci:
        raise ValueError('%s: mancano giorni, ore o lezioni' % nome)
    for t, docente in enumerate(docenti):
        if not 0 <= ingresso[t] < len(ore):
            raise ValueError('%s: l\'ingresso di %s non e\' un\'ora della giornata' % (nome, docente))
        # l'uscita: dopo l'ingresso, e al piu' la fine della giornata
        if uscita[t] is None:
            uscita[t] = len(ore)
        if not ingresso[t] < uscita[t] <= len(ore):
            raise ValueError('%s: l\'uscita di %s deve venire dopo il suo ingresso' % (nome, docente))
        # il giorno libero: dal nome al numero
        if libero[t] not in giorni and libero[t] != 'qualunque':
            raise ValueError('%s: il giorno libero di %s («%s») non e\' un giorno' % (nome, docente, libero[t]))
        libero[t] = QUALUNQUE if libero[t] == 'qualunque' else giorni.index(libero[t])

    for m in sorted({m for (_, _, _, m) in voci}):
        if tipo_materia[m] not in tipo_aula:
            raise ValueError('%s: per %s serve un\'aula di tipo «%s», e non ce n\'e\' nessuna'
                             % (nome, materie[m], tipo_materia[m]))

    # PASSO 3: la scuola, con le voci in ordine di classe, docente, codocente, materia
    return {'giorni': giorni, 'ore': ore, 'classi': classi, 'obbligatorie': max(0, min(obbligatorie, len(ore))),
            'ore_di_seguito': ORE_DI_SEGUITO, 'classi_al_giorno': CLASSI_AL_GIORNO, 'buche_al_giorno': BUCHE_AL_GIORNO,
            'ore_per_una_buca': ORE_PER_UNA_BUCA, 'lezioni_al_giorno': LEZIONI_AL_GIORNO,
            'docenti': [[docente, ingresso[t], uscita[t], libero[t]] for t, docente in enumerate(docenti)],
            'materie': [list(x) for x in zip(materie, tipo_materia)], 'aule': [list(x) for x in zip(aule, tipo_aula)],
            'voci': [[k, a, b, quante, m] for (k, a, b, m), quante in sorted(voci.items())]}


def scrivi(scuola):
    """Il contrario di leggi_testo: il testo del caso di una scuola. Rileggendolo si ottiene la stessa scuola."""
    giorni, docenti = scuola['giorni'], scuola['docenti']
    righe = ['giorni\t' + '\t'.join(giorni), 'ore\t' + '\t'.join(scuola['ore']), 'obbligatorie\t%d' % scuola['obbligatorie']]
    righe += ['materia\t%s\t%s' % tuple(m) for m in scuola['materie']]
    righe += ['aula\t%s\t%s' % tuple(a) for a in scuola['aule']]
    for nome, ingresso, uscita, libero in docenti:
        giorno = 'qualunque' if libero == QUALUNQUE else giorni[libero]
        righe.append('docente\t%s\t%d\t%d\t%s' % (nome, ingresso, uscita, giorno))
    for k, a, b, ore, m in scuola['voci']:
        righe.append('\t'.join(['lezione', scuola['classi'][k], str(ore), scuola['materie'][m][0], docenti[a][0]]
                               + ([docenti[b][0]] if b >= 0 else [])))
    return '\n'.join(righe) + '\n'
