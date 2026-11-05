"""Il solver visto da Python: compilarlo, lanciarlo su dei dati, leggere la sua traccia.

Il solver e' quello di main, con l'ingresso e l'uscita per il sito (ingresso_uscita/): legge i dati da un JSON
(--input), scrive la traccia della ricerca (--traccia, un JSON per riga: dati, backtrack, orari trovati, fine) e il
progresso su stderr (--progresso). La ricerca e i controlli sono quelli di main.

Il solver non ha un limite di tempo: lo applica il sito, fermando il processo. Un test fermato (dal pulsante o dal tempo)
non ha la riga "fine" nella traccia.

Le opzioni di un test:
    {'tempo': 60, 'orari': 1, 'max_backtrack': 1000}
    tempo           secondi massimi (assente = senza limite: si ferma col pulsante o quando ha finito)
    orari           quanti orari completi trovare prima di fermarsi (-n)
    max_backtrack   quanti backtrack registrare per esteso (gli altri si contano soltanto)
    max_nodi        quanti nodi della ricerca registrare (per scorrerla passo passo), default MAX_NODI
"""
import hashlib
import json
import os
import re
import subprocess
import threading
import time

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# ---- compilazione ----

def sorgenti():
    """I file .c del solver, presi da CMakeLists.txt (una sola fonte per CMake e per il sito)."""
    testo = open(os.path.join(RADICE, 'CMakeLists.txt')).read()
    blocco = re.search(r'add_executable\(\s*\S+(.*?)\)', testo, re.S).group(1)
    return [f for f in blocco.split() if f.endswith('.c')]


def impronta_sorgenti():
    """Impronta di tutti i sorgenti (.c e .h): cambia appena cambia una riga, anche senza commit."""
    h = hashlib.sha1()
    for cartella, _, files in sorted(os.walk(RADICE)):
        if any(p in cartella for p in ('/.git', '/sito', '/tests', '/dati_sito', '/cmake-build')):
            continue
        for f in sorted(files):
            if f.endswith(('.c', '.h')) or f == 'CMakeLists.txt':
                p = os.path.join(cartella, f)
                h.update(os.path.relpath(p, RADICE).encode())
                h.update(open(p, 'rb').read())
    return h.hexdigest()[:12]


_lock_compila = threading.Lock()


def compila(cartella_bin):
    """Compila il solver (solver-<impronta> in cartella_bin, solo se i sorgenti sono cambiati) e ne ritorna il percorso."""
    with _lock_compila:
        os.makedirs(cartella_bin, exist_ok=True)
        dest = os.path.join(cartella_bin, 'solver-' + impronta_sorgenti())
        if os.path.exists(dest):
            return dest
        cc = os.environ.get('CC', 'gcc')
        provvisorio = dest + '.tmp'
        cmd = [cc, '-O2', '-std=gnu11', '-w'] + [os.path.join(RADICE, f) for f in sorgenti()] + ['-o', provvisorio, '-lm']
        subprocess.run(cmd, check=True)
        os.replace(provvisorio, dest)
        return dest


def commit_attuale():
    """Commit del repository (con '+modifiche' se ci sono file modificati), o None se git non c'e'."""
    try:
        h = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], cwd=RADICE, capture_output=True, text=True,
                           check=True).stdout.strip()
        sporco = subprocess.run(['git', 'status', '--porcelain', '--untracked-files=no'], cwd=RADICE,
                                capture_output=True, text=True).stdout.strip()
        return h + ('+modifiche' if sporco else '')
    except (OSError, subprocess.CalledProcessError):
        return None


# ---- un'esecuzione ----

DATI, TRACCIA, USCITA = 'dati.json', 'traccia.jsonl', 'uscita.txt'
MAX_NODI = 1000000


def supporta_nodi():
    """Il solver sa registrare i nodi (--max-nodi)? Si guarda nei sorgenti, che sono quelli che il sito compila."""
    try:
        return '--max-nodi' in open(os.path.join(RADICE, 'main.c')).read()
    except OSError:
        return False


def argomenti(opzioni):
    """La riga di comando (senza l'eseguibile). I file stanno nella cartella dell'esecuzione."""
    return ['--input', DATI, '--traccia', TRACCIA, '-n', str(opzioni.get('orari', 1)),
            '--max-backtrack', str(opzioni.get('max_backtrack', 1000)), '--txt', os.devnull, '--progresso'] + \
        (['--max-nodi', str(opzioni.get('max_nodi', MAX_NODI))] if supporta_nodi() else [])


class Esecuzione:
    """Un processo del solver in corso: il sito ne tiene uno per test (per il progresso, i backtrack e per fermarlo).

    su_record(record) riceve, mentre la ricerca va avanti, ogni riga della traccia (dati, backtrack, orario, fine)."""

    def __init__(self, exe, dati, opzioni, cartella, su_record=None):
        os.makedirs(cartella, exist_ok=True)
        with open(os.path.join(cartella, DATI), 'w') as f:
            json.dump(dati, f, ensure_ascii=False)
        self.cartella = cartella
        self.timeout = opzioni.get('tempo')
        self.su_record = su_record
        self.ultimo_progresso = None
        self.fermato_a_mano = False
        self.scaduto = False
        self.inizio = time.time()
        self._uscita = open(os.path.join(cartella, USCITA), 'w')
        self.processo = subprocess.Popen([exe] + argomenti(opzioni), cwd=cartella, stdout=self._uscita,
                                         stderr=subprocess.PIPE, text=True, bufsize=1)
        self._lettore = threading.Thread(target=self._leggi_traccia, daemon=True)
        self._lettore.start()

    def _leggi_traccia(self):
        """Segue il file della traccia mentre il solver lo scrive (una riga per record, scaricata subito)."""
        percorso = os.path.join(self.cartella, TRACCIA)
        while not os.path.exists(percorso) and self.processo.poll() is None:
            time.sleep(0.05)
        if not os.path.exists(percorso):
            return
        resto = ''
        with open(percorso) as f:
            while True:
                pezzo = f.read()
                if pezzo:
                    resto += pezzo
                    *righe, resto = resto.split('\n')
                    for riga in righe:
                        if riga.strip() and self.su_record:
                            try:
                                self.su_record(json.loads(riga))
                            except ValueError:
                                pass
                elif self.processo.poll() is not None:
                    pezzo = f.read()  # le ultime righe scritte prima di finire
                    if not pezzo:
                        break
                    resto += pezzo
                else:
                    time.sleep(0.1)

    def ferma(self, a_mano=True):
        """Ferma il solver (SIGTERM). La traccia resta come era: senza la riga "fine"."""
        if self.processo.poll() is None:
            self.fermato_a_mano = self.fermato_a_mano or a_mano
            self.processo.terminate()

    def attendi(self):
        """Aspetta la fine (o il limite di tempo) e ritorna il risultato (vedi interpreta)."""
        if self.timeout:
            def sorveglia():
                try:
                    self.processo.wait(self.timeout)
                except subprocess.TimeoutExpired:
                    self.scaduto = True
                    self.ferma(a_mano=False)
            threading.Thread(target=sorveglia, daemon=True).start()
        for riga in self.processo.stderr:
            if riga.startswith('{'):
                try:
                    self.ultimo_progresso = json.loads(riga)
                except ValueError:
                    pass
        codice = self.processo.wait()
        self._lettore.join(60)
        self._uscita.close()
        r = interpreta(os.path.join(self.cartella, TRACCIA), codice, self.fermato_a_mano, self.scaduto)
        r['secondi'] = time.time() - self.inizio
        return r


# ---- com'e' andata ----

def interpreta(percorso_traccia, codice, fermato_a_mano=False, scaduto=False):
    """L'esito di un'esecuzione dalla sua traccia:
        esito: 'trovato', 'nessuna soluzione' (la ricerca ha provato tutto), 'non finito' (limite di tempo),
               'fermato' (pulsante), 'errore nei dati', 'errore'
        finito: la ricerca e' arrivata a una conclusione (trovato o nessuna soluzione)
        dati, orari (i primi orari trovati), fine (la riga finale), nodi, backtrack, errore_dati"""
    dati, orari, fine, errore, progresso = None, [], None, None, None
    try:
        with open(percorso_traccia) as f:
            for riga in f:
                try:
                    x = json.loads(riga)
                except ValueError:
                    continue  # l'ultima riga di un solver fermato a meta' scrittura
                tipo = x.get('tipo')
                if tipo == 'dati':
                    dati = x
                elif tipo == 'orario':
                    orari.append(x['orario'])
                elif tipo == 'fine':
                    fine = x
                elif tipo == 'errore':
                    errore = x['messaggio']
                elif tipo == 'backtrack':
                    progresso = x
    except FileNotFoundError:
        pass
    if errore:
        esito = 'errore nei dati'
    elif fine:
        esito = fine['esito']
    elif fermato_a_mano:
        esito = 'fermato'
    elif scaduto:
        esito = 'non finito'
    else:
        esito = 'errore'
    r = {'esito': esito, 'finito': esito in ('trovato', 'nessuna soluzione'), 'dati': dati, 'orari': orari,
         'fine': fine, 'errore_dati': errore, 'codice': codice,
         'trovati': fine['orari'] if fine else len(orari),
         'nodi': fine['nodi'] if fine else None,
         'backtrack': fine['backtrack'] if fine else (progresso['n'] if progresso else 0)}
    return r


def valida(exe, dati, cartella):
    """Fa leggere i dati al solver senza cercare (-n 0). Ritorna None se li accetta, altrimenti il motivo."""
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, DATI), 'w') as f:
        json.dump(dati, f, ensure_ascii=False)
    subprocess.run([exe, '--input', DATI, '--traccia', TRACCIA, '-n', '0', '--txt', os.devnull], cwd=cartella,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=60)
    r = interpreta(os.path.join(cartella, TRACCIA), 0)
    if r['errore_dati']:
        return r['errore_dati']
    if not r['dati']:
        return 'il solver non ha letto i dati'
    return None
