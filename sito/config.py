"""Configurazione del sito, dalle variabili di ambiente (sul Pi: il file indicato in EnvironmentFile del servizio).

    DATABASE_URL       indirizzo di Postgres (default: postgresql://localhost/orario, va bene sul Mac con Postgres.app)
    SITO_LAVORO        cartella di lavoro, temporanea: eseguibile compilato e file delle prove in corso. Meglio in RAM
                       (default: /dev/shm/orario-sito) cosi' sulla SD del Raspberry non si scrive. I risultati
                       restano nel database.
    SITO_PARALLELI     casi in parallelo nei test notturni (default: processori - 1)
    SITO_HTTPS         1 se il sito e' raggiunto solo via HTTPS (Tailscale, proxy): il cookie di sessione diventa Secure
    SITO_PROXY         1 se il sito sta dietro un proxy fidato (tailscale serve, Caddy, nginx): legge indirizzo e protocollo
                       dalle intestazioni X-Forwarded-*, cosi' il limite ai tentativi di login conta l'indirizzo vero
    SITO_DB_TIMEOUT    secondi di attesa per collegarsi al database prima di dire che non e' raggiungibile (default: 5)

Il database puo' stare su un altro computer della rete locale (per esempio Postgres 18 sul Mac, con il sito sul Raspberry):
DATABASE_URL=postgresql://orario:PASSWORD@nome-del-mac.local:5432/orario  (vedi sito/INSTALLAZIONE.md)
"""
import os
import tempfile


class Config:
    def __init__(self):
        self.database_url = os.environ.get('DATABASE_URL', 'postgresql://localhost/orario')
        self.db_timeout = int(os.environ.get('SITO_DB_TIMEOUT', 5))
        base = '/dev/shm' if os.access('/dev/shm', os.W_OK) else tempfile.gettempdir()
        self.lavoro = os.path.abspath(os.environ.get('SITO_LAVORO') or os.path.join(base, 'orario-sito'))
        self.https = os.environ.get('SITO_HTTPS') == '1'
        self.proxy = os.environ.get('SITO_PROXY') == '1'
        self.paralleli = int(os.environ.get('SITO_PARALLELI', max(1, (os.cpu_count() or 2) - 1)))

    @property
    def cartella_bin(self):
        return os.path.join(self.lavoro, 'bin')

    @property
    def cartella_esecuzioni(self):
        return os.path.join(self.lavoro, 'esecuzioni')

