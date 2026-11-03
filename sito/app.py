"""L'applicazione Flask: login, protezione dei moduli, sezioni.

Sezioni (blueprint): dati della scuola (pagine_dati), test in tempo reale (pagine_live), test notturni
(pagine_notturni), dettaglio di un caso (pagine_casi). Le prove girano in thread dello stesso processo
(esecutore.py): il sito va avviato con un solo processo (python3 -m sito avvia).
"""
import datetime
import hmac
import os
import re

from flask import Flask, abort, flash, redirect, render_template, request, session, url_for
from werkzeug.security import check_password_hash

from . import db
from .config import Config


def crea_app(config=None, avvia_esecutore=True):
    app = Flask(__name__)
    config = config or Config()
    app.config['SITO'] = config
    app.config['SECRET_KEY'] = config.chiave_segreta()
    app.config['SESSION_COOKIE_SAMESITE'] = 'Lax'
    app.config['PERMANENT_SESSION_LIFETIME'] = datetime.timedelta(days=30)
    app.teardown_appcontext(db.chiudi)

    db.migra(config.database_url)

    from .pagine_casi import bp as casi
    from .pagine_dati import bp as dati
    from .pagine_live import bp as live
    from .pagine_notturni import bp as notturni
    for bp in (dati, live, notturni, casi):
        app.register_blueprint(bp)

    @app.before_request
    def controlla_accesso():
        # pagine pubbliche: login e file statici
        if request.endpoint in ('login', 'static'):
            return None
        if not session.get('dentro'):
            return redirect(url_for('login', poi=request.full_path))
        # protezione dei moduli: ogni POST deve portare il gettone della sessione
        if request.method == 'POST':
            gettone = request.form.get('_gettone') or request.headers.get('X-Gettone', '')
            if not hmac.compare_digest(gettone, session.get('gettone', '')):
                abort(400, 'Modulo scaduto: ricarica la pagina e riprova.')
        return None

    @app.context_processor
    def variabili_comuni():
        return {'gettone': session.get('gettone', '')}

    @app.route('/login', methods=['GET', 'POST'])
    def login():
        h = config.hash_password()
        if h is None:
            return render_template('senza_password.html'), 503
        if request.method == 'POST':
            if check_password_hash(h, request.form.get('password', '')):
                session.clear()
                session.permanent = True
                session['dentro'] = True
                session['gettone'] = os.urandom(16).hex()
                poi = request.args.get('poi') or url_for('home')
                return redirect(poi if poi.startswith('/') and not poi.startswith('//') else url_for('home'))
            flash('Password sbagliata.', 'errore')
        return render_template('login.html')

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
        """Classe CSS di un esito o di uno stato: "interrotto (file troppo grande)" -> esito-interrotto."""
        if not esito:
            return 'esito-in-corso'
        return 'esito-' + re.sub(r'\s*\(.*', '', esito).strip().replace(' ', '-')

    @app.template_filter('opzioni')
    def opzioni(o):
        """Le opzioni di un'esecuzione in parole: "tempo 60 s · -b fino a 150 MB · motivi"."""
        o = o or {}
        parti = ['tempo %s s' % o['tempo'] if o.get('tempo') else 'senza limite di tempo']
        if o.get('orari', 1) != 1:
            parti.append('%s orari' % o['orari'])
        if o.get('backtrack'):
            parti.append('-b fino a %s MB' % o.get('max_mb', '?'))
        if o.get('motivi'):
            parti.append('motivi degli scarti')
        if o.get('ordine_classi') is not None:
            parti.append('ordine delle classi %s' % o['ordine_classi'])
        if o.get('ore_obbligatorie') not in (None, 5):
            parti.append('%s ore obbligatorie al giorno' % o['ore_obbligatorie'])
        if o.get('ore_prof_classe') not in (None, 3):
            parti.append('docente al massimo %s ore al giorno in una classe' % o['ore_prof_classe'])
        if o.get('classi_giorno') not in (None, 3):
            parti.append('docente al massimo %s classi al giorno' % o['classi_giorno'])
        if o.get('solo_verifica'):
            parti.append('solo verifica')
        return ' · '.join(parti)

    @app.template_filter('numero')
    def numero(v):
        if v is None:
            return '-'
        return '{:,}'.format(v).replace(',', '.')

