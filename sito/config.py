"""Configurazione del sito, dalle variabili di ambiente (sul Pi: il file indicato in EnvironmentFile del servizio).

    DATABASE_URL       indirizzo di Postgres (default: postgresql://localhost/orario, va bene sul Mac con Postgres.app)
    SITO_LAVORO        cartella di lavoro, temporanea: eseguibile compilato e file delle prove in corso. Meglio in RAM
                       (default: /dev/shm/orario-sito) cosi' sulla SD del Raspberry non si scrive. I risultati
                       restano nel database.
    SITO_DATI          solo per importare la vecchia password del sito, se c'e' (default: dati_sito/)
    SITO_PARALLELI     casi in parallelo nei test notturni (default: processori - 1)
    SITO_SPAZIO_MB     spazio massimo per i file delle marce indietro di una prova notturna (default: 20480 = 20 GB)
    SITO_DB_TIMEOUT    secondi di attesa per collegarsi al database prima di dire che non e' raggiungibile (default: 5)

Il database puo' stare su un altro computer della rete locale (per esempio Postgres 18 sul Mac, con il sito sul Raspberry):
DATABASE_URL=postgresql://orario:PASSWORD@nome-del-mac.local:5432/orario  (vedi sito/INSTALLAZIONE.md)
"""
import os
import tempfile

from .solver import RADICE


class Config:
    def __init__(self):
        self.database_url = os.environ.get('DATABASE_URL', 'postgresql://localhost/orario')
        self.db_timeout = int(os.environ.get('SITO_DB_TIMEOUT', 5))
        self.dati = os.path.abspath(os.environ.get('SITO_DATI', os.path.join(RADICE, 'dati_sito')))
        base = '/dev/shm' if os.access('/dev/shm', os.W_OK) else tempfile.gettempdir()
        self.lavoro = os.path.abspath(os.environ.get('SITO_LAVORO') or os.path.join(base, 'orario-sito'))
        self.paralleli = int(os.environ.get('SITO_PARALLELI', max(1, (os.cpu_count() or 2) - 1)))
        self.spazio_mb = int(os.environ.get('SITO_SPAZIO_MB', 20 * 1024))

    @property
    def cartella_bin(self):
        return os.path.join(self.lavoro, 'bin')

    @property
    def cartella_esecuzioni(self):
        return os.path.join(self.lavoro, 'esecuzioni')

    @property
    def file_password(self):
        return os.path.join(self.dati, 'password')

    def hash_password(self):
        if not os.path.exists(self.file_password):
            return None
        return open(self.file_password).read().strip() or None

