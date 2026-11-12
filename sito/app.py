"""Le pagine: il modulo con lo storico dei test (/) e la pagina di un test (/test/<id>), che segue la ricerca in
tempo reale leggendo /test/<id>/stato.json ogni secondo."""
import datetime
import os

from flask import Flask, abort, flash, jsonify, redirect, render_template, request, url_for

from . import dati, notturni, test_realtime

LAVORO = os.environ.get('SITO_LAVORO') or os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lavoro')
MAX_MILIONI = 20  # i passi occupano 6 byte per nodo: 20 milioni di nodi sono 120 MB
MAX_MILIONI_NOTTURNI = 200  # nei notturni i passi non si tengono: il limite e' solo il tempo di un giro


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
        ultimo = tests[0]['info'] if tests else {'seme': 0, 'classi': 8, 'ritardi': False, 'ordine': 0,
                                                 'limite': 1000000}
        return render_template('indice.html', tests=tests, ultimo=ultimo, in_corso=test_realtime.in_corso(),
                               max_milioni=MAX_MILIONI)

    @app.route('/avvia', methods=['POST'])
    def avvia():
        f = request.form
        try:
            seme = int(f.get('seme') or 0)
            classi = int(f.get('classi') or 8)
            ordine = int(f.get('ordine') or 0)
            milioni = float((f.get('milioni') or '1').replace(',', '.'))
            if seme < 0 or not 1 <= classi <= 120 or ordine < 0 or not 0 < milioni <= MAX_MILIONI:
                raise ValueError
        except ValueError:
            flash('Seme e ordine delle classi da 0 in su, classi da 1 a 120, limite di nodi da 0,1 a %d milioni.'
                  % MAX_MILIONI)
            return redirect(url_for('indice'))
        if notturni.in_corso() is not None:
            flash('Sta girando un giro di test notturni: aspetta che finisca, o fermalo.')
            return redirect(url_for('indice'))
        try:
            tid = test_realtime.avvia(LAVORO, seme, classi, f.get('ritardi') == 'con', ordine, milioni)
        except (ValueError, RuntimeError) as e:
            flash(str(e))
            return redirect(url_for('indice'))
        return redirect(url_for('pagina_test', tid=tid))

    @app.route('/test/<int:tid>')
    def pagina_test(tid):
        t = test_realtime.test(LAVORO, tid)
        if not t:
            abort(404)
        d = test_realtime.dati(LAVORO, t['info'])
        if not d:
            abort(500)
        classi, docenti, coppie, riepilogo = dati.riassunto_dati(d)
        return render_template('test.html', t=t, classi=classi, docenti=docenti, coppie=coppie, riepilogo=riepilogo,
                               grafo=dati.grafo(coppie), ore=d['ore'])

    @app.route('/test/<int:tid>/passo/<int:k>')
    def passo(tid, k):
        """Il nodo k: l'orario fin li', la cella che la ricerca sta per decidere e tutti i suoi controlli."""
        p = test_realtime.passo(LAVORO, tid, k)
        if not p:
            abort(404)
        return jsonify(p)

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

    # ---- i test notturni ----

    @app.route('/notturni')
    def pagina_notturni():
        return render_template('notturni.html', n=notturni.situazione(LAVORO), max_milioni=MAX_MILIONI_NOTTURNI)

    @app.route('/notturni/stato.json')
    def stato_notturni():
        return jsonify(notturni.situazione(LAVORO))

    @app.route('/notturni/limite', methods=['POST'])
    def limite_notturni():
        try:
            milioni = float((request.form.get('milioni') or '').replace(',', '.'))
            if not 0 < milioni <= MAX_MILIONI_NOTTURNI:
                raise ValueError
        except ValueError:
            flash('Il limite di nodi va da 0,1 a %d milioni.' % MAX_MILIONI_NOTTURNI)
            return redirect(url_for('pagina_notturni'))
        if notturni.in_corso() is not None:
            flash('C\'è un giro in corso: il limite si cambia quando è finito.')
        else:
            notturni.cambia_limite(LAVORO, int(round(milioni * 1000000)))
        return redirect(url_for('pagina_notturni'))

    @app.route('/notturni/aggiungi', methods=['POST'])
    def aggiungi_notturno():
        """Il caso di un test in tempo reale diventa un test notturno."""
        t = test_realtime.test(LAVORO, int(request.form.get('test') or 0))
        if not t:
            abort(404)
        i = t['info']
        nuovo, aggiunto = notturni.aggiungi(LAVORO, i['seme'], i['classi'], bool(i['ritardi']), i.get('ordine', 0))
        flash('Aggiunto ai test notturni (test %d).' % nuovo['id'] if aggiunto
              else 'Questo caso è già nei test notturni (test %d).' % nuovo['id'])
        return redirect(request.form.get('poi') or url_for('pagina_notturni'))

    @app.route('/notturni/togli/<int:tid>', methods=['POST'])
    def togli_notturno(tid):
        if notturni.in_corso() is not None:
            flash('C\'è un giro in corso: i test si tolgono quando è finito.')
        else:
            notturni.togli(LAVORO, tid)
        return redirect(url_for('pagina_notturni'))

    @app.route('/notturni/avvia', methods=['POST'])
    def avvia_giro():
        try:
            notturni.avvia_giro(LAVORO)
        except (ValueError, RuntimeError) as e:
            flash(str(e))
        return redirect(url_for('pagina_notturni'))

    @app.route('/notturni/ferma', methods=['POST'])
    def ferma_giro():
        notturni.ferma_giro()
        return redirect(url_for('pagina_notturni'))

    notturni.chiudi_giri_interrotti(LAVORO)
    return app
