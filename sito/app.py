"""Le pagine: il modulo con lo storico dei test (/) e la pagina di un test (/test/<id>), che segue la ricerca in
tempo reale leggendo /test/<id>/stato.json ogni secondo."""
import datetime
import os

from flask import Flask, abort, flash, jsonify, redirect, render_template, request, url_for

from . import dati, passi, test_realtime

LAVORO = os.environ.get('SITO_LAVORO') or os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lavoro')
MAX_MILIONI = 20  # i passi occupano 6 byte per nodo: 20 milioni di nodi sono 120 MB


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
        return render_template('indice.html', tests=tests, ultimo=ultimo, in_corso=test_realtime.in_corso(),
                               max_milioni=MAX_MILIONI)

    @app.route('/avvia', methods=['POST'])
    def avvia():
        f = request.form
        try:
            seme = int(f.get('seme') or 0)
            classi = int(f.get('classi') or 8)
            milioni = float((f.get('milioni') or '1').replace(',', '.'))
            if seme < 0 or not 1 <= classi <= 120 or not 0 < milioni <= MAX_MILIONI:
                raise ValueError
        except ValueError:
            flash('Seme da 0 in su, classi da 1 a 120, limite di nodi da 0,1 a %d milioni.' % MAX_MILIONI)
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
        d = dati.dati_del_caso(t['info'])
        classi, docenti, coppie, riepilogo = dati.riassunto_dati(d)
        return render_template('test.html', t=t, classi=classi, docenti=docenti, coppie=coppie, riepilogo=riepilogo,
                               grafo=dati.grafo(coppie), ore=d['ore'])

    @app.route('/test/<int:tid>/passo/<int:k>')
    def passo(tid, k):
        """Il nodo k della ricerca: la cella che sta decidendo, quella del nodo prima e l'orario fin li'."""
        t = test_realtime.test(LAVORO, tid)
        if not t or not t['stato'].get('celle'):
            abort(404)
        x = passi.nodo(test_realtime.percorso_passi(LAVORO, tid), k, t['stato']['celle'])
        if x is None:
            abort(404)
        pos, prima, orario = x
        nomi = dati.docenti_in_ordine(dati.dati_del_caso(t['info']))
        nome = lambda i: nomi[i] if 0 <= i < len(nomi) else None  # noqa: E731
        return jsonify({'k': k, 'pos': pos, 'prima': prima, 'orario': [[nome(a), nome(b)] for a, b in orario]})

    @app.route('/test/<int:tid>/stato.json')
    def stato(tid):
        t = test_realtime.test(LAVORO, tid)
        if not t:
            abort(404)
        return jsonify(t['stato'])

    @app.route('/test/<int:tid>/controlli/<int:k>')
    def controlli(tid, k):
        """Al nodo k: il controllo della classe, la coda di priorita' e i controlli dei docenti della classe."""
        c = test_realtime.controlli(LAVORO, tid, k)
        if c is None:
            abort(404)
        return jsonify(c)

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
