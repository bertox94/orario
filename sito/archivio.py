"""I file delle esecuzioni, nel database (tabella file_esecuzione) invece che sul disco del sito."""
from . import db

PEZZO = 4 * 2 ** 20


def salva(url, eid, nome, dati, n0=0):
    """Salva dati (bytes) come file `nome` dell'esecuzione, a pezzi di 4 MB. Ritorna i pezzi scritti."""
    if not dati:
        return 0
    with db.connetti(url) as c:
        for n, i in enumerate(range(0, len(dati), PEZZO), n0):
            c.execute('insert into file_esecuzione (esecuzione_id, nome, n, dati) values (%s, %s, %s, %s)',
                      (eid, nome, n, dati[i:i + PEZZO]))
    return (len(dati) + PEZZO - 1) // PEZZO


def salva_file(url, eid, nome, percorso):
    try:
        with open(percorso, 'rb') as f:
            salva(url, eid, nome, f.read())
    except FileNotFoundError:
        pass


class Scrittore:
    """Scrive un file a pezzi mentre arriva (le marce indietro di un'esecuzione lunga): in memoria resta al massimo
    un pezzo."""

    def __init__(self, url, eid, nome):
        self.url, self.eid, self.nome = url, eid, nome
        self.buf = bytearray()
        self.n = 0
        self.totale = 0

    def scrivi(self, dati):
        self.buf += dati
        while len(self.buf) >= PEZZO:
            self._scarica(PEZZO)

    def chiudi(self):
        if self.buf:
            self._scarica(len(self.buf))

    def _scarica(self, quanti):
        pezzo = bytes(self.buf[:quanti])
        with db.connetti(self.url) as c:
            c.execute('insert into file_esecuzione (esecuzione_id, nome, n, dati) values (%s, %s, %s, %s)',
                      (self.eid, self.nome, self.n, pezzo))
        del self.buf[:quanti]
        self.n += 1
        self.totale += len(pezzo)


def dimensioni(c, eid):
    """{nome: byte} dei file di un'esecuzione."""
    return {r['nome']: r['byte'] for r in c.execute(
        'select nome, sum(octet_length(dati))::bigint as byte from file_esecuzione where esecuzione_id = %s '
        'group by nome', (eid,)).fetchall()}


def pezzi(url, eid, nome):
    """Il contenuto di un file, un pezzo alla volta (per la risposta in streaming: non si tiene tutto in memoria)."""
    with db.connetti(url) as c:
        n = 0
        while True:
            r = c.execute('select dati from file_esecuzione where esecuzione_id = %s and nome = %s and n = %s',
                          (eid, nome, n)).fetchone()
            if not r:
                return
            yield bytes(r['dati'])
            n += 1
