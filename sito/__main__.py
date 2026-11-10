"""Avvio del sito:

    python3 -m sito avvia [--host 0.0.0.0] [--porta 8000]

Con --host 0.0.0.0 il sito si vede da tutta la rete locale (sul Raspberry). Non c'e' login: chiunque sia nella rete
puo' usarlo.
"""
import argparse

from .app import crea_app


def main():
    ap = argparse.ArgumentParser(prog='python3 -m sito', description='Il sito dell\'orario: test in tempo reale.')
    sotto = ap.add_subparsers(dest='comando', required=True)
    a = sotto.add_parser('avvia', help='avvia il sito')
    a.add_argument('--host', default='127.0.0.1', help='indirizzo su cui ascoltare (0.0.0.0 = tutta la rete locale)')
    a.add_argument('--porta', type=int, default=8000)
    args = ap.parse_args()
    app = crea_app()
    try:
        from waitress import serve
    except ImportError:  # senza waitress il server di Flask, che per un uso da soli va bene
        app.run(host=args.host, port=args.porta, threaded=True)
    else:
        print('Sito su http://%s:%d' % (args.host, args.porta))
        serve(app, host=args.host, port=args.porta, threads=4)


if __name__ == '__main__':
    main()
