"""Configurazione del sito, dalle variabili di ambiente (sul Pi: il file indicato in EnvironmentFile del servizio).

    DATABASE_URL       indirizzo di Postgres (default: postgresql://localhost/orario, va bene sul Mac con Postgres.app)
    SITO_DATI          cartella dei file del sito: eseguibili, risultati delle prove, password (default: dati_sito/)
    SITO_PARALLELI     casi in parallelo nei test notturni (default: processori - 1)
    SITO_SPAZIO_MB     spazio massimo per i file delle marce indietro di una prova notturna (default: 20480 = 20 GB)
"""
import os

from .solver import RADICE


class Config:
    def __init__(self):
        self.database_url = os.environ.get('DATABASE_URL', 'postgresql://localhost/orario')
        self.dati = os.path.abspath(os.environ.get('SITO_DATI', os.path.join(RADICE, 'dati_sito')))
        self.paralleli = int(os.environ.get('SITO_PARALLELI', max(1, (os.cpu_count() or 2) - 1)))
        self.spazio_mb = int(os.environ.get('SITO_SPAZIO_MB', 20 * 1024))

    @property
    def cartella_bin(self):
        return os.path.join(self.dati, 'bin')

    @property
    def cartella_prove(self):
        return os.path.join(self.dati, 'prove')

    @property
    def file_password(self):
        return os.path.join(self.dati, 'password')

    @property
    def file_chiave(self):
        return os.path.join(self.dati, 'chiave_segreta')

    def chiave_segreta(self):
        """Chiave per firmare i cookie di sessione: creata al primo avvio e conservata in SITO_DATI."""
        os.makedirs(self.dati, exist_ok=True)
        if not os.path.exists(self.file_chiave):
            with open(os.open(self.file_chiave, os.O_WRONLY | os.O_CREAT, 0o600), 'w') as f:
                f.write(os.urandom(32).hex())
        return open(self.file_chiave).read().strip()

    def hash_password(self):
        if not os.path.exists(self.file_password):
            return None
        return open(self.file_password).read().strip() or None

    def salva_hash_password(self, h):
        os.makedirs(self.dati, exist_ok=True)
        with open(os.open(self.file_password, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600), 'w') as f:
            f.write(h + '\n')
