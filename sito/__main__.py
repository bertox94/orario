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


def main():
    ap = argparse.ArgumentParser(prog='python3 -m sito', description=__doc__.split('\n')[0])
    sotto = ap.add_subparsers(dest='comando', required=True)
    a = sotto.add_parser('avvia', help='avvia il sito')
    a.add_argument('--host', default='127.0.0.1', help='indirizzo su cui ascoltare (0.0.0.0 = tutta la rete locale)')
    a.add_argument('--porta', type=int, default=8000)
    sotto.add_parser('imposta-password', help='imposta la password di accesso')
    sotto.add_parser('migra', help='aggiorna il database')
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
    elif args.comando == 'migra':
        from .db import migra
        print('Applicate:', migra(config.database_url) or 'nessuna (database gia\' aggiornato)')
    elif args.comando == 'avvia':
        from .app import crea_app
        app = crea_app(config)
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
