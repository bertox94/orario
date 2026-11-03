"""Comandi del sito:

    python3 -m sito avvia [--host 0.0.0.0] [--porta 8000]   avvia il sito (sul Pi: dal servizio systemd)
    python3 -m sito imposta-password                        imposta (o cambia) la password di accesso
    python3 -m sito migra                                   aggiorna il database (lo fa anche "avvia")
"""
import argparse
import getpass
import sys

from werkzeug.security import generate_password_hash

from .config import Config


def _con_database(config, azione):
    """Esegue azione(); se il database non e' raggiungibile esce con un messaggio chiaro (systemd riprova da solo)."""
    import psycopg
    from .db import descrivi_errore
    try:
        return azione()
    except psycopg.OperationalError as e:
        sys.exit(descrivi_errore(config.database_url, e))


def _prova_database(config):
    from .db import connetti
    def prova():
        with connetti(config.database_url) as c:
            versione = c.execute('show server_version').fetchone()['server_version']
            migrazioni = c.execute("select count(*) as n from information_schema.tables "
                                   "where table_name = 'migrazioni_applicate'").fetchone()['n']
        print('Collegato a Postgres %s (%s).' % (versione, 'database gia\' usato dal sito' if migrazioni else
                                                  'database vuoto: le tabelle si creano al primo avvio'))
        if int(versione.split('.')[0]) < 14:
            print('Attenzione: il sito e\' pensato per Postgres 14 o piu\' recente (sul Mac: la 18).')
    _con_database(config, prova)


def main():
    ap = argparse.ArgumentParser(prog='python3 -m sito', description=__doc__.split('\n')[0])
    sotto = ap.add_subparsers(dest='comando', required=True)
    a = sotto.add_parser('avvia', help='avvia il sito')
    a.add_argument('--host', default='127.0.0.1', help='indirizzo su cui ascoltare (0.0.0.0 = tutta la rete locale)')
    a.add_argument('--porta', type=int, default=8000)
    sotto.add_parser('imposta-password', help='imposta la password di accesso')
    sotto.add_parser('migra', help='aggiorna il database')
    sotto.add_parser('prova-database', help='controlla il collegamento a Postgres (anche sul Mac) e mostra la versione')
    args = ap.parse_args()
    config = Config()

    if args.comando == 'imposta-password':
        p1 = getpass.getpass('Nuova password: ')
        if len(p1) < 8:
            sys.exit('La password deve avere almeno 8 caratteri.')
        if getpass.getpass('Ripeti: ') != p1:
            sys.exit('Le due password non coincidono.')
        config.salva_hash_password(generate_password_hash(p1, method='pbkdf2:sha256'))  # scrypt manca nel Python del Mac
        print('Password salvata in', config.file_password)
    elif args.comando == 'prova-database':
        _prova_database(config)
    elif args.comando == 'migra':
        from .db import migra
        _con_database(config, lambda: print('Applicate:', migra(config.database_url) or 'nessuna (database gia\' aggiornato)'))
    elif args.comando == 'avvia':
        from .app import crea_app
        app = _con_database(config, lambda: crea_app(config))
        print('Sito su http://%s:%d (dati in %s)' % (args.host, args.porta, config.dati), flush=True)
        try:
            from waitress import serve
        except ImportError:
            # senza waitress: il server di Flask, sufficiente per un solo utente
            app.run(host=args.host, port=args.porta, threaded=True)
        else:
            serve(app, host=args.host, port=args.porta, threads=16)


if __name__ == '__main__':
    main()
