"""Il sito dell'orario: le pagine e i loro indirizzi. Mostra soltanto: i casi e gli orari li fa il programma
(orario/), che il sito lancia.

Ogni indirizzo qui fa poco: legge quello che arriva dal modulo della pagina, chiama prove.py o notturni.py, e torna a
una pagina. Quando qualcosa non si puo' fare, prove.py e notturni.py lo dicono con un ValueError: il suo testo diventa
il messaggio mostrato in cima alla pagina (flash)."""
import datetime
import os

from flask import Flask, abort, flash, jsonify, redirect, render_template, request, url_for

from orario.scuola import MAX_CLASSI

from . import notturni, prove

MAX_SECONDI_NOTTURNI = 3600   # il tempo massimo che un caso dei test notturni puo' avere
MAX_CASI_PER_VOLTA = 2000     # quanti casi si possono aggiungere ai test notturni in una volta


def numero(n):
    """Un numero con i punti delle migliaia."""
    return '{:,}'.format(n).replace(',', '.')


def _interi(campo):
    """I numeri arrivati in un campo del modulo, separati da virgole."""
    return [int(x) for x in (request.form.get(campo) or '').split(',') if x.strip().isdigit()]


def crea_app():
    app = Flask(__name__)
    # la chiave serve solo ai messaggi che passano da una pagina all'altra (flash)
    app.secret_key = os.environ.get('SITO_SEGRETO') or os.urandom(16)
    # due filtri per le pagine: i numeri con i punti, e una data da un tempo in secondi
    app.jinja_env.filters['numero'] = numero
    app.jinja_env.filters['data'] = lambda t: datetime.datetime.fromtimestamp(t).strftime('%d/%m/%Y %H:%M')

    # ---- le prove ----

    @app.route('/')
    def indice():
        tutte = prove.tutte()
        ultima = tutte[0]['info'] if tutte else {'seme': 0, 'classi': 8, 'ritardi': False}
        return render_template('indice.html', prove=tutte, ultima=ultima, in_corso=prove.in_corso(),
                               tempo=prove.TEMPO, max_classi=MAX_CLASSI)

    @app.route('/avvia', methods=['POST'])
    def avvia():
        f = request.form
        try:
            if notturni.in_corso() is not None:
                raise ValueError('Sta girando un giro di test notturni: aspetta che finisca, o fermalo.')
            # (se seme o classi non vanno, o il caso non si genera, lo dice il generatore: ValueError)
            n = prove.avvia(int(f.get('seme') or 0), int(f.get('classi') or 0), f.get('ritardi') == 'con')
        except (ValueError, RuntimeError) as e:
            flash(str(e))
            return redirect(url_for('indice'))
        return redirect(url_for('prova', n=n))

    @app.route('/prova/<int:n>')
    def prova(n):
        p = prove.prova(n)
        if not p:
            abort(404)
        return render_template('prova.html', p=p, tempo=prove.TEMPO)

    @app.route('/prova/<int:n>/stato.json')
    def stato(n):
        p = prove.prova(n)
        if not p:
            abort(404)
        return jsonify(p['stato'])

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
        return render_template('notturni.html', n=notturni.situazione(), max_secondi=MAX_SECONDI_NOTTURNI,
                               max_classi=MAX_CLASSI)

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
            # senza, con, o tutti e due (ogni caso due volte)
            ritardi = {'senza': (False,), 'con': (True,)}.get(f.get('ritardi'), (False, True))
            if not (0 <= semi_da <= semi_a and 1 <= classi_da <= classi_a <= MAX_CLASSI):
                raise ValueError('Semi da 0 in su e classi da 1 a %d, il primo numero non più grande del secondo.'
                                 % MAX_CLASSI)
            # tutte le combinazioni: per ogni numero di classi, ogni seme, con e senza
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
        """Il caso di una prova entra tra i test notturni."""
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
        """Il tempo di ogni caso, e il giro della notte: se parte da solo e a che ora."""
        def imposta():
            f = request.form
            tempo = int(f.get('secondi') or 0)
            # l'ora deve essere scritta HH:MM (se no ValueError, e il messaggio)
            ora = datetime.datetime.strptime(f.get('ora') or '', '%H:%M').strftime('%H:%M')
            if not 1 <= tempo <= MAX_SECONDI_NOTTURNI:
                raise ValueError('Il tempo di un caso va da 1 a %d secondi.' % MAX_SECONDI_NOTTURNI)
            notturni.imposta(tempo=tempo, notte=f.get('notte') == 'si', ora=ora)
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

    # all'avvio del sito: i giri rimasti a meta' risultano interrotti, e parte l'orologio del giro della notte
    notturni.all_avvio()
    return app
