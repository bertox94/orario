"""Il solver visto da Python: compilarlo, lanciarlo su un caso, capire com'e' andata.

Usato dal sito (test in tempo reale e notturni) e da tests/casuali.py, cosi' la logica e' in un posto solo.

Un caso e' un dizionario:
    {'tipo': 'casuale', 'seme': 7, 'classi': 8, 'ritardi': True}
    {'tipo': 'input', 'file': '/percorso/dati.json'}        dati letti da file (per esempio quelli della scuola)
    {'tipo': 'programma'}                                    i dati scritti nel programma
e le opzioni della ricerca:
    {'tempo': 60, 'backtrack': False, 'motivi': True, 'max_mb': 0, 'ordine_classi': None, 'orari': 1}
"""
import hashlib
import json
import os
import re
import resource
import select
import shutil
import subprocess
import threading
import time

RADICE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# ---- compilazione ----

def sorgenti():
    """I file .c del solver, presi da CMakeLists.txt (una sola fonte per CMake, casuali.py e il sito)."""
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


def compila(dest=None, cartella_bin=None):
    """Compila il solver e ritorna il percorso dell'eseguibile.

    Con cartella_bin l'eseguibile si chiama solver-<impronta> e si ricompila solo se i sorgenti sono cambiati;
    con dest si compila sempre li'."""
    with _lock_compila:
        if dest is None:
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

def argomenti(caso, opzioni, progresso=False):
    """La riga di comando (senza l'eseguibile) per un caso con le sue opzioni. I file escono nella cartella corrente:
    dati.txt, risultato.json e orario.txt (orario.txt.gz con -b)."""
    a = []
    if caso['tipo'] == 'casuale':
        a += ['--casuale', str(caso['seme']), '--classi', str(caso.get('classi', 8))]
        if caso.get('ritardi'):
            a.append('--ritardi')
    elif caso['tipo'] == 'input':
        a += ['--input', caso['file']]
    a += ['-n', str(opzioni.get('orari', 1)), '--dati', opzioni.get('file_dati', 'dati.txt'), '--json', 'risultato.json']
    if opzioni.get('tempo'):
        a += ['--tempo', str(opzioni['tempo'])]
    for chiave, flag in (('ore_obbligatorie', '--ore-obbligatorie'), ('ore_prof_classe', '--ore-prof-classe'),
                         ('classi_giorno', '--classi-giorno')):
        if opzioni.get(chiave) is not None:
            a += [flag, str(opzioni[chiave])]
    if opzioni.get('motivi', True):
        a.append('--motivi')
    if opzioni.get('ordine_classi') is not None:
        a.append(str(opzioni['ordine_classi']))
    if opzioni.get('backtrack') or opzioni.get('passi'):
        a += ['-b', '--orario', 'orario.txt.gz']
        if opzioni.get('passi'):
            a += ['--passi', str(opzioni['passi'])]
        if opzioni.get('max_mb'):
            a += ['--max-mb', str(opzioni['max_mb'])]
    if opzioni.get('solo_verifica'):
        a.append('--solo-verifica')
    if progresso:
        a.append('--progresso')
    return a


class Esecuzione:
    """Un processo del solver in corso. Il sito ne tiene uno per ogni test in tempo reale (per il progresso e per
    fermarlo); casuali.py usa direttamente esegui()."""

    def __init__(self, exe, args, cartella, timeout=None, limite_mb=0, su_progresso=None, su_gz=None, limite_gz_mb=0):
        """su_gz: funzione che riceve a pezzi le marce indietro compresse (orario.txt.gz), invece di scriverle sul
        disco: il file e' una pipe con nome e i byte vanno direttamente a chi li conserva (il database). Il limite
        di spazio (limite_gz_mb) lo controlla questa classe, perche' sul disco non c'e' nessun file da misurare."""
        os.makedirs(cartella, exist_ok=True)
        self.su_gz = su_gz
        self.limite_gz = int(limite_gz_mb * 2 ** 20)
        self.byte_gz = 0
        self.troppo_grande = False
        self.errore_gz = None
        self.thread_gz = None
        if su_gz and '-b' in args:
            os.mkfifo(os.path.join(cartella, 'orario.txt.gz'))
        self.cartella = cartella
        self.timeout = timeout
        self.su_progresso = su_progresso
        self.righe = []
        self.ultimo_progresso = None
        self.inizio = time.time()
        # Riserva: limite del sistema alla dimensione dei file, un po' piu' largo di --max-mb (il solver si ferma da
        # solo prima; questo scatta solo se non lo facesse).
        limite = int(limite_mb * 2 ** 20) if limite_mb > 0 else resource.RLIM_INFINITY
        self.processo = subprocess.Popen(
            [exe] + args, cwd=cartella, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1,
            preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_FSIZE, (limite, limite)))
        if su_gz and '-b' in args:
            self.thread_gz = threading.Thread(target=self._leggi_gz, args=(os.path.join(cartella, 'orario.txt.gz'),),
                                              daemon=True)
            self.thread_gz.start()

    def _leggi_gz(self, percorso):
        fd = os.open(percorso, os.O_RDONLY | os.O_NONBLOCK)  # non blocca: il solver lo apre quando serve
        try:
            while True:
                pronti = select.select([fd], [], [], 0.5)[0]
                if pronti:
                    pezzo = os.read(fd, 1 << 20)
                    if not pezzo:
                        break  # il solver ha chiuso il file
                    self._pezzo_gz(pezzo)
                elif self.processo.poll() is not None:
                    break  # il solver e' finito senza (piu') scrivere
        finally:
            os.close(fd)

    def _pezzo_gz(self, pezzo):
        prima = self.byte_gz
        self.byte_gz += len(pezzo)
        # dopo il limite il solver si ferma da solo (SIGTERM) e chiude il file: si tiene ancora un po' di margine
        # perche' il file resti valido; oltre il margine i byte si leggono e si scartano (senza bloccare gzip)
        if self.errore_gz or (self.limite_gz and prima > self.limite_gz * 1.05 + 8 * 2 ** 20):
            return
        try:
            self.su_gz(pezzo)
        except Exception as ex:  # noqa: BLE001 - per esempio il database non risponde
            self.errore_gz = str(ex)
            self.ferma()
            return
        if self.limite_gz and self.byte_gz > self.limite_gz and not self.troppo_grande:
            self.troppo_grande = True
            self.ferma()

    def ferma(self):
        """Chiede al solver di fermarsi (SIGTERM): si ferma in modo ordinato, con diagnosi e JSON."""
        if self.processo.poll() is None:
            self.processo.terminate()

    def attendi(self):
        """Legge l'uscita fino alla fine e ritorna il risultato (vedi interpreta)."""
        scaduto = False
        if self.timeout:
            # se il solver non si ferma da solo (--tempo) entro un margine, lo si ferma e poi lo si uccide
            def sorveglia():
                try:
                    self.processo.wait(self.timeout + 30)
                except subprocess.TimeoutExpired:
                    nonlocal scaduto
                    scaduto = True
                    self.processo.terminate()
                    try:
                        self.processo.wait(30)
                    except subprocess.TimeoutExpired:
                        self.processo.kill()
            threading.Thread(target=sorveglia, daemon=True).start()
        for riga in self.processo.stdout:
            if riga.startswith('PROGRESSO '):
                try:
                    self.ultimo_progresso = json.loads(riga[10:])
                except ValueError:
                    continue
                if self.su_progresso:
                    self.su_progresso(self.ultimo_progresso)
            else:
                self.righe.append(riga)
        codice = self.processo.wait()
        if self.thread_gz:
            self.thread_gz.join(120)
        out = ''.join(self.righe)
        # gli stessi messaggi che scrive il solver quando e' lui a misurare il file
        if self.troppo_grande:
            out += '\nInterrotto per spazio: marce indietro oltre il limite\n'
        if self.errore_gz:
            out += '\nErrore di scrittura delle marce indietro: %s\n' % self.errore_gz
        risultato = None
        p = os.path.join(self.cartella, 'risultato.json')
        if os.path.exists(p) and os.path.getmtime(p) >= self.inizio - 1:
            try:
                risultato = json.load(open(p))
            except ValueError:
                risultato = None
        r = interpreta(out, codice, risultato)
        if scaduto and r['esito'] == 'errore':
            r['esito'] = 'non finito'
        r['secondi'] = time.time() - self.inizio
        r['uscita'] = out
        r['json'] = risultato
        return r


def esegui(exe, caso, opzioni, cartella, su_progresso=None):
    """Esegue un caso fino alla fine e ritorna il risultato (vedi interpreta)."""
    tempo = opzioni.get('tempo')
    max_mb = opzioni.get('max_mb') if opzioni.get('backtrack') else 0
    e = Esecuzione(exe, argomenti(caso, opzioni, progresso=su_progresso is not None), cartella,
                   timeout=tempo, limite_mb=(max_mb * 1.1 + 16) if max_mb else 0, su_progresso=su_progresso)
    return e.attendi()


def analizza_slot(exe, caso, opzioni, cartella, pos, slot):
    """La coda dei docenti dello slot pos (indice nella ricerca) e perche' ognuno e' o non e' utilizzabile, dati i primi
    pos slot dell'orario parziale (slot: elenco di "i", "i+j" o "-"). Il caso e le opzioni sono quelli
    dell'esecuzione (dati e ordine delle classi); ritorna il dizionario del solver o None."""
    base = {k: opzioni[k] for k in ('ordine_classi', 'ore_obbligatorie', 'ore_prof_classe', 'classi_giorno')
            if opzioni.get(k) is not None}
    os.makedirs(cartella, exist_ok=True)
    with open(os.path.join(cartella, 'analisi.txt'), 'w') as f:
        f.write('%d\n%s\n' % (pos, ' '.join(slot[:pos])))
    args = argomenti(caso, {**base, 'orari': 1, 'motivi': False}) + ['--analizza-slot', 'analisi.txt']
    try:
        p = subprocess.run([exe] + args, cwd=cartella, capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired:
        return None
    for riga in p.stdout.splitlines():
        if riga.startswith('ANALISI '):
            return json.loads(riga[8:])
    return None


# ---- com'e' andata ----

def interpreta(out, codice, risultato_json=None):
    """Dall'uscita del solver (e dal suo JSON, se c'e') l'esito di un'esecuzione:
        esito: 'trovato', 'nessuna soluzione', 'impossibile', 'non finito', 'interrotto (file troppo grande)',
               'interrotto (errore di scrittura)', 'fermato', 'errore nei dati', 'errore'
        finito: la ricerca e' arrivata a una conclusione (trovato, nessuna soluzione o impossibile)
        marce, trovati, impossibile (motivo), errore_dati (motivo), diagnosi (riassunto), diagnosi_testo"""
    errore_dati = re.search(r'^Errore nei dati: (.*)$', out, re.M)
    impossibile = re.search(r'Impossibile \(verifica preliminare\): ([^\n]*)', out)
    trovati = re.search(r'Totale generati: (\d+)', out)
    marce = re.search(r'^Marce indietro: (\d+)$', out, re.M)
    r = {'marce': int(marce.group(1)) if marce else None, 'trovati': int(trovati.group(1)) if trovati else 0,
         'impossibile': impossibile.group(1) if impossibile else None,
         'errore_dati': errore_dati.group(1) if errore_dati else None}
    per_tempo = 'Interrotto per tempo' in out
    per_spazio = 'Interrotto per spazio' in out
    scrittura = 'Errore di scrittura' in out
    richiesta = 'Interrotto su richiesta' in out
    if errore_dati:
        esito = 'errore nei dati'
    elif impossibile:
        esito = 'impossibile'
    elif 'Verifica preliminare superata' in out:
        esito = 'verifica superata'
    elif per_spazio:
        esito = 'interrotto (file troppo grande)'
    elif scrittura:
        esito = 'interrotto (errore di scrittura)'
    elif richiesta:
        esito = 'fermato'
    elif codice != 0 or not trovati:
        # ucciso dal sistema (file troppo grande senza --max-mb) o terminato male
        esito = 'errore'
    elif per_tempo:
        esito = 'non finito' if r['trovati'] == 0 else 'trovato'
    else:
        esito = 'trovato' if r['trovati'] > 0 else 'nessuna soluzione'
    r['esito'] = esito
    r['finito'] = esito in ('trovato', 'nessuna soluzione', 'impossibile', 'verifica superata')
    if impossibile:
        r['diagnosi'] = 'impossibile: ' + impossibile.group(1)
        r['diagnosi_testo'] = 'Impossibile (verifica preliminare): ' + impossibile.group(1)
    elif errore_dati:
        r['diagnosi'], r['diagnosi_testo'] = 'errore nei dati: ' + errore_dati.group(1), errore_dati.group(0)
    else:
        r['diagnosi'], r['diagnosi_testo'] = diagnosi(out)
    return r


POCHI_BLOCCHI = 1000  # sotto questo numero di blocchi la diagnosi non classifica (vedi diagnosi)


def diagnosi(out):
    """Legge la diagnosi stampata dal solver. Ritorna (riassunto breve, testo completo).

    Se le marce indietro sono quasi tutte in pochi slot (e la ricerca non avanza piu'), il problema e' localizzato:
    di solito il caso e' impossibile per costruzione, o la causa sta poco prima di quegli slot.
    Se sono sparse su molti slot, la ricerca e' solo difficile."""
    i = out.find('Diagnosi della ricerca')
    if i < 0:
        return '-', ''
    testo = out[i:].split('Totale generati')[0].strip()
    # i vicoli ciechi sono le marce indietro oppure i rami scartati dalla potatura: si guarda i piu' numerosi
    m = re.search(r'Marce indietro: (\d+) in (\d+) slot diversi', testo)
    s = re.search(r'Rami scartati: (\d+) dopo (\d+) slot diversi', testo)
    marce = int(m.group(1)) if m else 0
    rami = int(s.group(1)) if s else 0
    if not marce and not rami:
        return 'nessuna marcia indietro', testo
    # con pochi blocchi la ricerca e' andata liscia: "localizzata" o "diffusa" non dicono niente (2 rami scartati in
    # 2 slot farebbero "LOCALIZZATA, top3 100%")
    if marce + rami < POCHI_BLOCCHI:
        return 'pochi blocchi: %d marce indietro, %d rami scartati' % (marce, rami), testo
    if marce >= rami:
        cosa, n_slot = 'marce', m.group(2)
        quote = [float(x) for x in re.findall(r'marce \(([\d.]+)%\)', testo)]
        primo = re.search(r'Slot piu\' frequente 1: posizione \d+ \(giorno (\S+), ora (\S+), classe (\S+)\)', testo)
    else:
        cosa, n_slot = 'rami scartati', s.group(2)
        quote = [float(x) for x in re.findall(r'Rami scartati dopo lo slot \d: .*\(([\d.]+)%\)', testo)]
        primo = re.search(r'Rami scartati dopo lo slot 1: posizione \d+ \(giorno (\S+), ora (\S+), classe (\S+)\)', testo)
    ferma = re.search(r'raggiunta al nodo \d+ \(([\d.]+)% dei nodi\)', testo)
    top3 = sum(quote[:3])
    nome = '%s %s %s' % primo.groups() if primo else '?'
    stato = 'LOCALIZZATA' if top3 >= 80 else 'diffusa'
    senza_progressi = ferma and float(ferma.group(1)) < 5
    return '%s (%s): %s slot, top3 %.0f%%, piu\' frequente %s%s' % (
        stato, cosa, n_slot, top3, nome, ', senza avanzare' if senza_progressi else ''), testo


