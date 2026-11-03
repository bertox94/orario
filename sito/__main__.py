"""Comandi del sito:

    python3 -m sito avvia [--host 0.0.0.0] [--porta 8000]   avvia il sito (sul Pi: dal servizio systemd)
    python3 -m sito crea-utente NOME [--admin]              crea un utente (o ne cambia la password)
    python3 -m sito prova-database                          controlla il collegamento a Postgres
    python3 -m sito migra                                   aggiorna il database (lo fa anche "avvia")
"""
import argparse
import getpass
import sys


from .config import Config


def _con_database(config, azione):
    """Esegue azione(); se il database non e' raggiungibile esce con un messaggio chiaro (systemd riprova da solo)."""
    import psycopg
    from .db import descrivi_errore
    try:
        return azione()
    except psycopg.OperationalError as e:
        sys.exit(descrivi_errore(config.database_url, e))


def _crea_utente(config, nome, password, admin):
    from .accesso import crea_o_aggiorna_utente
    from .db import connetti, migra
    def azione():
        migra(config.database_url)  # le tabelle degli utenti nascono con la migrazione
        with connetti(config.database_url) as c:
            crea_o_aggiorna_utente(c, nome, password, admin or None)  # un amministratore vede tutte le scuole
            c.commit()
        print('Utente «%s» pronto%s.' % (nome, ' (amministratore)' if admin else ': abbinagli le scuole in Amministrazione'))
    _con_database(config, azione)


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
    u = sotto.add_parser('crea-utente', help='crea un utente o cambia la sua password')
    u.add_argument('nome')
    u.add_argument('--admin', action='store_true', help='amministratore: vede tutte le scuole e gestisce utenti e scuole')
    sotto.add_parser('migra', help='aggiorna il database')
    sotto.add_parser('prova-database', help='controlla il collegamento a Postgres (anche sul Mac) e mostra la versione')
    args = ap.parse_args()
    config = Config()

    if args.comando == 'crea-utente':
        nome, admin = args.nome, args.admin
        p1 = getpass.getpass('Password di %s: ' % nome)
        if len(p1) < 8:
            sys.exit('La password deve avere almeno 8 caratteri.')
        if getpass.getpass('Ripeti: ') != p1:
            sys.exit('Le due password non coincidono.')
        _crea_utente(config, nome, p1, admin)
    elif args.comando == 'prova-database':
        _prova_database(config)
    elif args.comando == 'migra':
        from .db import migra
        _con_database(config, lambda: print('Applicate:', migra(config.database_url) or 'nessuna (database gia\' aggiornato)'))
    elif args.comando == 'avvia':
        from .app import crea_app
        app = _con_database(config, lambda: crea_app(config))
        print('Sito su http://%s:%d (lavoro in %s, risultati nel database)' % (args.host, args.porta, config.lavoro), flush=True)
        try:
            from waitress import serve
        except ImportError:
            # senza waitress: il server di Flask, sufficiente per un solo utente
            app.run(host=args.host, port=args.porta, threaded=True)
        else:
            serve(app, host=args.host, port=args.porta, threads=16)


if __name__ == '__main__':
    main()
