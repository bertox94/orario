"""Le pagine: il modulo con lo storico dei test (/) e la pagina di un test (/test/<id>), che segue la ricerca in
tempo reale leggendo /test/<id>/stato.json ogni secondo."""
import datetime
import os

from flask import Flask, abort, flash, jsonify, redirect, render_template, request, url_for

from . import test_realtime

LAVORO = os.environ.get('SITO_LAVORO') or os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lavoro')


def numero(v):
    """1234567 -> 1.234.567"""
    return '{:,}'.format(v).replace(',', '.') if isinstance(v, int) else v


def crea_app():
    app = Flask(__name__)
    app.secret_key = os.environ.get('SITO_SEGRETO') or os.urandom(16)
    app.jinja_env.filters['numero'] = numero
    app.jinja_env.filters['data'] = lambda t: datetime.datetime.fromtimestamp(t).strftime('%d/%m/%Y %H:%M')

    @app.route('/')
    def indice():
        tests = test_realtime.storico(LAVORO)
        ultimo = tests[0]['info'] if tests else {'seme': 0, 'classi': 8, 'ritardi': False, 'limite': 1000000}
        return render_template('indice.html', tests=tests, ultimo=ultimo, in_corso=test_realtime.in_corso())

    @app.route('/avvia', methods=['POST'])
    def avvia():
        f = request.form
        try:
            seme = int(f.get('seme') or 0)
            classi = int(f.get('classi') or 8)
            milioni = float((f.get('milioni') or '0').replace(',', '.'))
            if seme < 0 or not 1 <= classi <= 120 or milioni < 0:
                raise ValueError
        except ValueError:
            flash('Seme da 0 in su, classi da 1 a 120, limite in milioni di nodi (0: senza limite).')
            return redirect(url_for('indice'))
        try:
            tid = test_realtime.avvia(LAVORO, seme, classi, f.get('ritardi') == 'con', milioni)
        except (ValueError, RuntimeError) as e:
            flash(str(e))
            return redirect(url_for('indice'))
        return redirect(url_for('pagina_test', tid=tid))

    @app.route('/test/<int:tid>')
    def pagina_test(tid):
        t = test_realtime.test(LAVORO, tid)
        if not t:
            abort(404)
        return render_template('test.html', t=t)

    @app.route('/test/<int:tid>/stato.json')
    def stato(tid):
        t = test_realtime.test(LAVORO, tid)
        if not t:
            abort(404)
        return jsonify(t['stato'])

    @app.route('/ferma', methods=['POST'])
    def ferma():
        test_realtime.ferma(LAVORO)
        return redirect(request.form.get('poi') or url_for('indice'))

    @app.route('/test/<int:tid>/elimina', methods=['POST'])
    def elimina(tid):
        try:
            test_realtime.elimina(LAVORO, tid)
        except ValueError as e:
            flash(str(e))
        return redirect(url_for('indice'))

    return app
