"""Il sito dell'orario: le prove in tempo reale e i test notturni. Mostra soltanto: i casi e la ricerca sono del
solver (solver/), che il sito compila e lancia."""
import datetime
import os

from flask import Flask, abort, flash, jsonify, redirect, render_template, request, url_for

from . import notturni, passi, prove

MAX_MILIONI = 1000        # il limite di nodi di una prova (richiesta di Alb). Ogni nodo si registra: 19 MB per
                          # milione, quindi 19 GB per mille milioni; prove.avvia controlla che il disco basti
MAX_MILIONI_NOTTURNI = 100
MAX_CASI_PER_VOLTA = 2000


def numero(n):
    """Un numero con i punti delle migliaia."""
    return '{:,}'.format(n).replace(',', '.')


def _interi(campo):
    """I numeri arrivati in un campo del modulo, separati da virgole."""
    return [int(x) for x in (request.form.get(campo) or '').split(',') if x.strip().isdigit()]


def crea_app():
    app = Flask(__name__)
    app.secret_key = os.environ.get('SITO_SEGRETO') or os.urandom(16)
    app.jinja_env.filters['numero'] = numero
    app.jinja_env.filters['data'] = lambda t: datetime.datetime.fromtimestamp(t).strftime('%d/%m/%Y %H:%M')

    # ---- le prove in tempo reale ----

    @app.route('/')
    def indice():
        tutte = prove.tutte()
        ultima = tutte[0]['info'] if tutte else {'seme': 0, 'classi': 8, 'ritardi': False, 'limite': 1000000}
        return render_template('indice.html', prove=tutte, ultima=ultima, in_corso=prove.in_corso(),
                               max_milioni=MAX_MILIONI)

    @app.route('/avvia', methods=['POST'])
    def avvia():
        f = request.form
        try:
            seme, classi = int(f.get('seme') or 0), int(f.get('classi') or 0)
            milioni = float((f.get('milioni') or '1').replace(',', '.'))
            if seme < 0 or not 1 <= classi <= 120 or not 0 < milioni <= MAX_MILIONI:
                raise ValueError('Seme da 0 in su, classi da 1 a 120, limite di nodi fino a %d milioni.' % MAX_MILIONI)
            if notturni.in_corso() is not None:
                raise ValueError('Sta girando un giro di test notturni: aspetta che finisca, o fermalo.')
            n = prove.avvia(seme, classi, f.get('ritardi') == 'con', int(round(milioni * 1000000)))
        except (ValueError, RuntimeError) as e:
            flash(str(e))
            return redirect(url_for('indice'))
        return redirect(url_for('prova', n=n))

    @app.route('/prova/<int:n>')
    def prova(n):
        p = prove.prova(n)
        if not p:
            abort(404)
        return render_template('prova.html', p=p)

    @app.route('/prova/<int:n>/stato.json')
    def stato(n):
        p = prove.prova(n)
        if not p:
            abort(404)
        return jsonify(p['stato'])

    @app.route('/prova/<int:n>/passo/<int:k>')
    def passo(n, k):
        """Il nodo k della ricerca: l'ultima scelta, l'orario fin li' e, se da li' non si va avanti, il motivo."""
        p = prove.prova(n)
        x = p and passi.passo(prove.file_dei_passi(n), k, p['stato'].get('celle', 0))
        if not x:
            abort(404)
        return jsonify(x)

    @app.route('/prova/<int:n>/nodo/<int:k>')
    def nodo(n, k):
        """Quello che il solver dice del nodo k: i conti di ogni docente e, se da li' non si va avanti, il conto
        che non torna in dettaglio e le celle di cui parla (il tasto «Mostra nell'orario»)."""
        x = prove.nodo(n, k)
        if not x:
            abort(404)
        return jsonify(x)

    @app.route('/prova/<int:n>/ritorno/<int:k>/<verso>')
    def ritorno(n, k, verso):
        """Il primo ritorno indietro dopo il nodo k (verso «dopo») o prima: {"k": ...}, null se non c'e'. Con
        ?cella=N solo i ritorni di quella cella."""
        if verso not in ('prima', 'dopo') or not prove.prova(n):
            abort(404)
        cella = request.args.get('cella', type=int)
        return jsonify({'k': passi.ritorno(prove.file_dei_passi(n), k, verso == 'dopo', cella)})

    @app.route('/prova/<int:n>/profilo.json')
    def profilo(n):
        if not prove.prova(n):
            abort(404)
        return jsonify(passi.profilo(prove.file_dei_passi(n)))

    @app.route('/ferma', methods=['POST'])
    def ferma():
        prove.ferma()
        return redirect(request.form.get('poi') or url_for('indice'))

    @app.route('/elimina', methods=['POST'])
    def elimina():
        """Elimina le prove scelte: i loro numeri arrivano in `numeri`, separati da virgole."""
        for n in _interi('numeri'):
            try:
                prove.elimina(n)
            except ValueError as e:
                flash(str(e))
        return redirect(url_for('indice'))

    # ---- i test notturni ----

    @app.route('/notturni')
    def pagina_notturni():
        return render_template('notturni.html', n=notturni.situazione(), max_milioni=MAX_MILIONI_NOTTURNI)

    @app.route('/notturni/stato.json')
    def stato_notturni():
        return jsonify(notturni.situazione())

    def _con_giro_fermo(azione):
        """Fa l'azione se non c'e' un giro in corso, e torna alla pagina."""
        if notturni.in_corso() is not None:
            flash('C\'è un giro in corso: si può fare quando è finito.')
        else:
            try:
                azione()
            except (ValueError, RuntimeError) as e:
                flash(str(e))
        return redirect(url_for('pagina_notturni'))

    @app.route('/notturni/aggiungi', methods=['POST'])
    def aggiungi_notturni():
        """Aggiunge i casi con i semi e le classi negli intervalli scelti, senza ritardi, con, o tutti e due."""
        def aggiungi():
            f = request.form
            semi_da, semi_a, classi_da, classi_a = (int(f.get(k) or 0)
                                                    for k in ('semi_da', 'semi_a', 'classi_da', 'classi_a'))
            ritardi = {'senza': (False,), 'con': (True,)}.get(f.get('ritardi'), (False, True))
            if not (0 <= semi_da <= semi_a and 1 <= classi_da <= classi_a <= 120):
                raise ValueError('Semi da 0 in su e classi da 1 a 120, il primo numero non più grande del secondo.')
            nuovi = [(s, c, r) for c in range(classi_da, classi_a + 1) for s in range(semi_da, semi_a + 1)
                     for r in ritardi]
            if len(nuovi) > MAX_CASI_PER_VOLTA:
                raise ValueError('Sono %s casi: al massimo %s per volta.'
                                 % (numero(len(nuovi)), numero(MAX_CASI_PER_VOLTA)))
            aggiunti = notturni.aggiungi(nuovi)
            flash('Aggiunti %s casi (%s c\'erano già).' % (numero(aggiunti), numero(len(nuovi) - aggiunti)))
        return _con_giro_fermo(aggiungi)

    @app.route('/notturni/da-prova', methods=['POST'])
    def notturno_da_prova():
        """Il caso di una prova in tempo reale entra tra i test notturni."""
        p = prove.prova(int(request.form.get('prova') or 0))
        if not p:
            abort(404)
        i = p['info']
        flash('Aggiunto ai test notturni.' if notturni.aggiungi([(i['seme'], i['classi'], bool(i['ritardi']))])
              else 'Questo caso è già tra i test notturni.')
        return redirect(url_for('prova', n=i['numero']))

    @app.route('/notturni/togli', methods=['POST'])
    def togli_notturni():
        return _con_giro_fermo(lambda: notturni.togli(_interi('numeri')))

    @app.route('/notturni/impostazioni', methods=['POST'])
    def impostazioni_notturni():
        """Il limite di nodi, e il giro della notte: se parte da solo e a che ora."""
        def imposta():
            f = request.form
            milioni = float((f.get('milioni') or '').replace(',', '.'))
            ora = datetime.datetime.strptime(f.get('ora') or '', '%H:%M').strftime('%H:%M')
            if not 0 < milioni <= MAX_MILIONI_NOTTURNI:
                raise ValueError('Il limite di nodi va fino a %d milioni.' % MAX_MILIONI_NOTTURNI)
            notturni.imposta(limite=int(round(milioni * 1000000)), notte=f.get('notte') == 'si', ora=ora)
        return _con_giro_fermo(imposta)

    @app.route('/notturni/avvia', methods=['POST'])
    def avvia_giro():
        return _con_giro_fermo(notturni.avvia_giro)

    @app.route('/notturni/ferma', methods=['POST'])
    def ferma_giro():
        notturni.ferma_giro()
        return redirect(url_for('pagina_notturni'))

    @app.route('/notturni/visto', methods=['POST'])
    def visto_notturni():
        notturni.visto(_interi('numeri'))
        return redirect(url_for('pagina_notturni'))

    notturni.all_avvio()
    return app
