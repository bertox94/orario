"""L'applicazione Flask: login, protezione dei moduli, sezioni.

Sezioni (blueprint): dati della scuola (pagine_dati), test in tempo reale (pagine_live), dettaglio di un test con i
suoi backtrack (pagine_casi), test notturni (pagine_notturni). Le prove girano in thread dello stesso processo (esecutore.py): il sito va avviato con
un solo processo (python3 -m sito avvia).
"""
import datetime
import re

from flask import Flask, redirect, render_template, request, session, url_for
from flask.sessions import SecureCookieSessionInterface

import psycopg

from . import accesso, db
from .config import Config


class SessioneSecureSoloInHttps(SecureCookieSessionInterface):
    def get_cookie_secure(self, app):
        return request.is_secure  # con SITO_PROXY=1 e' il protocollo visto dal proxy (X-Forwarded-Proto)


def crea_app(config=None, avvia_esecutore=True):
    app = Flask(__name__)
    config = config or Config()
    app.config['SITO'] = config
    app.config['SESSION_COOKIE_SAMESITE'] = 'Lax'
    app.config['SESSION_COOKIE_HTTPONLY'] = True
    # Il cookie di sessione e' Secure solo quando la pagina arriva in HTTPS (Tailscale, proxy): cosi' il sito funziona
    # anche in chiaro sulla rete locale. Con un cookie sempre Secure, dal browser in http il login riusciva ma il cookie
    # non veniva salvato e si tornava alla pagina di accesso senza errori.
    app.session_interface = SessioneSecureSoloInHttps()
    if config.proxy:  # dietro un proxy fidato (SITO_PROXY=1): indirizzo del client e protocollo dalle intestazioni
        from werkzeug.middleware.proxy_fix import ProxyFix
        app.wsgi_app = ProxyFix(app.wsgi_app, x_for=1, x_proto=1, x_host=1)
    app.config['PERMANENT_SESSION_LIFETIME'] = datetime.timedelta(days=30)
    app.teardown_appcontext(db.chiudi)

    db.migra(config.database_url)
    app.config['SECRET_KEY'] = db.chiave_segreta(config.database_url)
    from .pagine_casi import bp as casi
    from .pagine_dati import bp as dati
    from .pagine_live import bp as live
    from .pagine_notturni import bp as notturni
    for bp in (dati, live, casi, notturni):
        app.register_blueprint(bp)

    app.register_blueprint(accesso.bp)
    app.before_request(accesso.controlla_accesso)

    @app.errorhandler(psycopg.OperationalError)
    def database_non_raggiungibile(errore):
        # per esempio il Mac con Postgres e' spento, in stop o fuori dalla rete
        return render_template('senza_database.html', messaggio=db.descrivi_errore(config.database_url, errore)), 503

    app.context_processor(accesso.variabili_comuni)

    app.add_url_rule('/login', 'login', accesso.login, methods=['GET', 'POST'])

    @app.route('/logout', methods=['POST'])
    def logout():
        session.clear()
        return redirect(url_for('login'))

    @app.route('/')
    def home():
        return redirect(url_for('dati.indice'))

    registra_filtri(app)

    if avvia_esecutore:
        from . import esecutore
        esecutore.avvia(app)
        from . import notturni as test_notturni
        test_notturni.avvia_sorveglianza(config)
    return app


def registra_filtri(app):
    app.jinja_env.globals['zip'] = zip
    @app.template_filter('data')
    def data(v):
        if not v:
            return '-'
        return v.astimezone().strftime('%d/%m/%Y %H:%M')

    @app.template_filter('durata')
    def durata(sec):
        if sec is None:
            return '-'
        sec = int(sec)
        if sec < 60:
            return '%d s' % sec
        if sec < 3600:
            return '%d min %02d s' % (sec // 60, sec % 60)
        return '%d h %02d min' % (sec // 3600, sec % 3600 // 60)

    @app.template_filter('classe_esito')
    def classe_esito(esito):
        """Classe CSS di un esito o di uno stato: "interrotto (errore di scrittura)" -> esito-interrotto."""
        if not esito:
            return 'esito-in-corso'
        return 'esito-' + re.sub(r'\s*\(.*', '', esito).strip().replace(' ', '-')

    @app.template_filter('opzioni')
    def opzioni(o):
        """Le opzioni di un test in parole: "tempo 60 s · 1 orario · 1.000 backtrack registrati"."""
        o = o or {}
        parti = (['tempo %s s' % o['tempo']] if o.get('tempo') else []) + \
            (['fermo dopo %s nodi' % numero(o['ferma_nodi'])] if o.get('ferma_nodi') else []) + \
            (['senza limite'] if not o.get('tempo') and not o.get('ferma_nodi') else []) + ['%s orar%s' % (o.get('orari', 1), 'io' if o.get('orari', 1) == 1 else 'i')]
        if o.get('max_backtrack') is not None:
            parti.append('%s backtrack registrati al massimo' % numero(o['max_backtrack']))
        if o.get('solutore'):
            parti.append('solver %s' % o['solutore'])
        return ' · '.join(parti)

    @app.template_filter('numero')
    def numero(v):
        if v is None:
            return '-'
        return '{:,}'.format(v).replace(',', '.')

