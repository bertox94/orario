"""Le scuole scritte a mano: i dati di una scuola vera, inseriti dalla pagina, da cui far fare l'orario.

Ogni scuola sta in lavoro/scuole/<numero>.json: {"numero", "nome", "modificata", "dati"}. I dati sono quelli che la
pagina mostra e modifica, con i nomi scritti per esteso (non i numeri che usa il programma):

    {"giorni": [nomi], "ore": [nomi], "obbligatorie": N,
     "materie": [[nome, tipo di aula], ...], "aule": [[nome, tipo], ...],
     "docenti": [[nome, ingresso, uscita, giorno libero], ...]
         ingresso e uscita sono indici di ore (insegna nelle ore da `ingresso` a `uscita - 1`); il giorno libero e' il
         nome di un giorno, o "qualunque" (lo sceglie il programma): ogni docente ne ha uno
     "lezioni": [[classe, materia, docente, codocente o "", ore], ...]}

Il percorso:
1. nuova(): una scuola vuota, oppure con i dati di un caso casuale da cui partire;
2. salva(): la pagina manda i dati cosi' come sono; si salvano sempre, anche incompleti (si lavora un po' alla volta);
3. problemi(): quello che ancora non va nei dati, detto con parole (la pagina lo mostra dopo ogni salvataggio);
4. testo(): i dati come file di un caso (orario/scuola.py), per il programma. Solo se non ci sono problemi.
"""
import os
import time

from orario import scuola as _scuola

from . import archivio

GIORNI, ORE, OBBLIGATORIE = ['Lun', 'Mar', 'Mer', 'Gio', 'Ven', 'Sab'], ['08', '09', '10', '11', '12', '13'], 5


def _file(numero):
    return archivio.percorso('scuole', '%d.json' % numero)


def vuota():
    """I dati di una scuola nuova: la settimana di sei giorni di sei ore, e nient'altro."""
    return {'giorni': list(GIORNI), 'ore': list(ORE), 'obbligatorie': OBBLIGATORIE, 'materie': [], 'aule': [],
            'docenti': [], 'lezioni': []}


def da_testo(testo):
    """I dati di una scuola dal testo di un caso (per partire da un caso casuale)."""
    s = _scuola.leggi_testo(testo)
    giorno = {_scuola.QUALUNQUE: 'qualunque', **dict(enumerate(s['giorni']))}
    nome = [d[0] for d in s['docenti']]
    return {'giorni': s['giorni'], 'ore': s['ore'], 'obbligatorie': s['obbligatorie'], 'materie': s['materie'],
            'aule': s['aule'], 'docenti': [[d[0], d[1], d[2], giorno[d[3]]] for d in s['docenti']],
            'lezioni': [[s['classi'][k], s['materie'][m][0], nome[a], nome[b] if b >= 0 else '', ore]
                        for k, a, b, ore, m in s['voci']]}


def _pulisci(dati):
    """I dati arrivati dalla pagina, messi in forma: solo i campi previsti, i nomi senza spazi intorno e senza TAB o
    a capo (nel file di un caso separano i campi), i numeri come numeri. ValueError se non sono fatti come devono."""
    def nome(x):
        return ' '.join(str(x).split())

    def intero(x):
        try:
            return int(x)
        except (TypeError, ValueError):
            return 0

    try:
        return {'giorni': [nome(g) for g in dati['giorni']], 'ore': [nome(o) for o in dati['ore']],
                'obbligatorie': intero(dati['obbligatorie']),
                'materie': [[nome(m), nome(t)] for m, t in dati['materie']],
                'aule': [[nome(a), nome(t)] for a, t in dati['aule']],
                'docenti': [[nome(d), intero(entra), intero(esce), nome(libero)]
                            for d, entra, esce, libero in dati['docenti']],
                'lezioni': [[nome(k), nome(m), nome(a), nome(b), intero(ore)] for k, m, a, b, ore in dati['lezioni']]}
    except (KeyError, TypeError, ValueError):
        raise ValueError('I dati della scuola non sono scritti come devono.')


def problemi(dati):
    """Quello che nei dati non va, come elenco di frasi (vuoto: si puo' fare l'orario). Prima i controlli che si
    possono dire con parole chiare; alla fine il programma stesso prova a leggere il caso (i limiti di grandezza)."""
    p = []
    giorni, ore = dati['giorni'], dati['ore']
    if not giorni or not ore or '' in giorni or '' in ore:
        p.append('Servono almeno un giorno e un\'ora, e ognuno deve avere un nome.')
    # i nomi: nessuno vuoto, nessuno ripetuto
    for che, elenco in (('giorno', giorni), ('ora', ore), ('materia', [m[0] for m in dati['materie']]),
                        ('aula', [a[0] for a in dati['aule']]), ('docente', [d[0] for d in dati['docenti']])):
        if '' in elenco and che not in ('giorno', 'ora'):
            p.append('C\'è un nome vuoto tra: %s.' % {'materia': 'le materie', 'aula': 'le aule',
                                                      'docente': 'i docenti'}[che])
        for ripetuto in sorted({x for x in elenco if x and elenco.count(x) > 1}):
            p.append('«%s» compare due volte (%s).' % (ripetuto, che))
    # le materie: ognuna con il suo tipo di aula, e almeno un'aula di quel tipo
    tipi = {t for _, t in dati['aule']}
    for materia, tipo in dati['materie']:
        if materia and tipo not in tipi:
            p.append('%s chiede un\'aula di tipo «%s», e non ce n\'è nessuna.' % (materia, tipo or '?'))
    # i docenti: l'uscita dopo l'ingresso, e il giorno libero che sia un giorno
    docenti = {d[0] for d in dati['docenti']}
    for nome, entra, esce, libero in dati['docenti']:
        if not 0 <= entra < esce <= len(ore):
            p.append('%s: l\'uscita deve venire dopo l\'entrata.' % (nome or 'Un docente'))
        if libero not in giorni and libero != 'qualunque':
            p.append('%s: il giorno libero «%s» non è un giorno.' % (nome or 'Un docente', libero))
    # le lezioni: classe, materia e docenti che esistono, due docenti diversi, almeno un'ora
    materie = {m[0] for m in dati['materie']}
    for i, (classe, materia, docente, codocente, quante) in enumerate(dati['lezioni'], 1):
        dove = 'Lezione %d (%s)' % (i, ', '.join(x for x in (classe, materia, docente) if x) or 'vuota')
        if not classe:
            p.append('%s: manca la classe.' % dove)
        if materia not in materie:
            p.append('%s: manca la materia.' % dove)
        if docente not in docenti:
            p.append('%s: manca il docente.' % dove)
        if codocente and (codocente not in docenti or codocente == docente):
            p.append('%s: il codocente deve essere un altro docente dell\'elenco.' % dove)
        if quante < 1:
            p.append('%s: le ore devono essere almeno 1.' % dove)
    if not dati['lezioni']:
        p.append('Non c\'è ancora nessuna lezione.')
    # se fin qui va tutto, la parola al programma: legge il caso come lo leggera' per fare l'orario
    if not p:
        try:
            _scuola.leggi_testo(testo(dati), 'la scuola')
        except ValueError as e:
            p.append('Il programma non accetta questi dati: %s.' % e)
    return p[:30]   # (trenta alla volta bastano)


def testo(dati):
    """I dati come file di un caso, nel formato di orario/scuola.py."""
    righe = ['giorni\t' + '\t'.join(dati['giorni']), 'ore\t' + '\t'.join(dati['ore']),
             'obbligatorie\t%d' % dati['obbligatorie']]
    righe += ['materia\t%s\t%s' % tuple(m) for m in dati['materie']]
    righe += ['aula\t%s\t%s' % tuple(a) for a in dati['aule']]
    righe += ['docente\t%s\t%d\t%d\t%s' % tuple(d) for d in dati['docenti']]
    righe += ['\t'.join(['lezione', classe, str(quante), materia, docente] + ([codocente] if codocente else []))
              for classe, materia, docente, codocente, quante in dati['lezioni']]
    return '\n'.join(righe) + '\n'


# ---- l'elenco delle scuole ----

def scuola(numero):
    """Una scuola ({"numero", "nome", "modificata", "dati"}), o None se non c'e'."""
    s = archivio.leggi(_file(numero))
    if s:
        # una scuola salvata quando un docente poteva non avere il giorno libero: «nessuno» diventa «qualunque»
        for docente in s['dati']['docenti']:
            if docente[3] == 'nessuno':
                docente[3] = 'qualunque'
    return s


def tutte():
    """Tutte le scuole, dalla piu' recente."""
    return [s for s in (scuola(n) for n in reversed(archivio.numeri(archivio.percorso('scuole')))) if s]


def nuova(nome, dati):
    """Crea una scuola con questi dati e ritorna il suo numero."""
    numero = max(archivio.numeri(archivio.percorso('scuole')), default=0) + 1
    salva(numero, nome, dati)
    return numero


def salva(numero, nome, dati):
    """Salva i dati della scuola, cosi' come sono (messi in forma). Ritorna i problemi che hanno ancora."""
    dati = _pulisci(dati)
    nome = ' '.join(str(nome).split()) or 'Scuola %d' % numero
    archivio.scrivi(_file(numero), {'numero': numero, 'nome': nome, 'modificata': time.time(), 'dati': dati})
    return problemi(dati)


def elimina(numero):
    try:
        os.remove(_file(numero))
    except OSError:
        pass
