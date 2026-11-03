"""Utenti, scuole e controllo degli accessi.

Ogni utente entra con nome e password e lavora su una scuola alla volta (quella scelta in /scuole). Un amministratore
vede tutte le scuole e gestisce scuole e utenti in /amministrazione. I dati (classi, docenti, prove...) sono sempre
quelli della scuola scelta: il controllo centrale `controlla_accesso` verifica anche che ogni identificativo che compare
nell'indirizzo (classe, docente, lezione, esecuzione, prova...) appartenga alla scuola scelta, altrimenti risponde 404.
"""
import hmac
import os
import time

from flask import (Blueprint, abort, flash, g, redirect, render_template, request, session, url_for)
from werkzeug.security import check_password_hash, generate_password_hash

from .db import conn

bp = Blueprint('accesso', __name__)

CALENDARIO_PREDEFINITO = ('{Lun,Mar,Mer,Gio,Ven,Sab}', '{08,09,10,11,12,13}')
LUNGHEZZA_MINIMA_PASSWORD = 8

# a chi il controllo centrale non chiede ne' login ne' scuola
PUBBLICHE = ('login', 'static')
# pagine per chi e' entrato ma non ha (ancora) scelto la scuola
SENZA_SCUOLA = ('logout', 'accesso.scuole', 'accesso.scegli_scuola')

# tentativi di accesso falliti: (indirizzo, nome) -> [numero, istante dell'ultimo]
_falliti = {}
MAX_TENTATIVI, ATTESA_SECONDI = 5, 60


def hash_password(password):
    return generate_password_hash(password, method='pbkdf2:sha256')  # scrypt manca in alcuni Python del Mac


def crea_scuola(c, nome):
    """Una scuola nuova, con il calendario predefinito. Ritorna l'id."""
    sid = c.execute('insert into scuole (nome) values (%s) returning id', (nome,)).fetchone()['id']
    c.execute('insert into calendario (scuola_id, giorni, ore) values (%s, %s, %s)', (sid,) + CALENDARIO_PREDEFINITO)
    return sid


def crea_o_aggiorna_utente(c, nome, password, amministratore=None):
    """Crea l'utente (o cambia la password). Ritorna l'id."""
    h = hash_password(password)
    riga = c.execute('select id from utenti where nome = %s', (nome,)).fetchone()
    if riga:
        c.execute('update utenti set password_hash = %s, attivo = true where id = %s', (h, riga['id']))
        if amministratore is not None:
            c.execute('update utenti set amministratore = %s where id = %s', (amministratore, riga['id']))
        return riga['id']
    return c.execute('insert into utenti (nome, password_hash, amministratore) values (%s, %s, %s) returning id',
                     (nome, h, bool(amministratore))).fetchone()['id']


def importa_password_vecchia(c, config):
    """Il sito aveva una password sola (file `password`): al primo avvio con utenti vuoti diventa l'utente "admin"."""
    if c.execute('select 1 from utenti limit 1').fetchone():
        return False
    h = config.hash_password()
    if not h:
        return False
    uid = c.execute("insert into utenti (nome, password_hash, amministratore) values ('admin', %s, true) returning id",
                    (h,)).fetchone()['id']
    c.execute('insert into utenti_scuole (utente_id, scuola_id) select %s, id from scuole', (uid,))
    return True


def scuole_dell_utente(c, utente):
    if utente['amministratore']:
        return c.execute('select id, nome from scuole order by nome').fetchall()
    return c.execute('select s.id, s.nome from scuole s join utenti_scuole us on us.scuola_id = s.id '
                     'where us.utente_id = %s order by s.nome', (utente['id'],)).fetchall()


# ---- controllo centrale ----

# (blueprint, parametro dell'indirizzo) -> query che dice se l'oggetto e' della scuola %s  (primo %s: id, secondo: scuola)
_PROPRIETA = {
    ('dati', 'cid'): 'select 1 from classi where id = %s and scuola_id = %s',
    ('dati', 'did'): 'select 1 from docenti where id = %s and scuola_id = %s',
    ('dati', 'lid'): 'select 1 from lezioni l join classi c on c.id = l.classe_id where l.id = %s and c.scuola_id = %s',
    ('casi', 'eid'): 'select 1 from esecuzioni e join prove p on p.id = e.prova_id where e.id = %s and p.scuola_id = %s',
    ('live', 'pid'): 'select 1 from prove where id = %s and scuola_id = %s',
    ('notturni', 'pid'): 'select 1 from prove where id = %s and scuola_id = %s',
}
_PIANIFICAZIONI = ('notturni.attiva_pianificazione', 'notturni.elimina_pianificazione')


def appartiene(tipo_blueprint, parametro, valore, scuola_id, endpoint=''):
    if endpoint in _PIANIFICAZIONI:
        q = 'select 1 from pianificazioni where id = %s and scuola_id = %s'
    else:
        q = _PROPRIETA.get((tipo_blueprint, parametro))
        if q is None:
            return True
    return conn().execute(q, (valore, scuola_id)).fetchone() is not None



def controlla_accesso():
    if request.endpoint in PUBBLICHE:
        return None
    uid = session.get('utente_id')
    utente = conn().execute('select id, nome, amministratore from utenti where id = %s and attivo', (uid,)).fetchone() \
        if uid else None
    if not utente:
        session.clear()
        return redirect(url_for('login', poi=request.full_path))
    g.utente = utente
    # protezione dei moduli: ogni POST deve portare il gettone della sessione
    if request.method == 'POST':
        gettone = request.form.get('_gettone') or request.headers.get('X-Gettone', '')
        if not hmac.compare_digest(gettone, session.get('gettone', '')):
            abort(400, 'Modulo scaduto: ricarica la pagina e riprova.')
    if request.endpoint and request.endpoint.startswith('accesso.amministrazione') and not utente['amministratore']:
        abort(403)
    if request.endpoint in SENZA_SCUOLA or (request.endpoint or '').startswith('accesso.amministrazione'):
        return None
    sid = session.get('scuola_id')
    scuola = conn().execute('select id, nome from scuole where id = %s', (sid,)).fetchone() if sid else None
    consentita = scuola and (utente['amministratore'] or conn().execute(
        'select 1 from utenti_scuole where utente_id = %s and scuola_id = %s', (utente['id'], sid)).fetchone())
    if not consentita:
        session.pop('scuola_id', None)
        return redirect(url_for('accesso.scuole'))
    g.scuola = scuola
    for parametro, valore in (request.view_args or {}).items():
        if parametro in ('cid', 'did', 'lid', 'eid', 'pid') and not appartiene(
                request.blueprint, parametro, valore, sid, request.endpoint):
            abort(404)
    return None


def variabili_comuni():
    return {'gettone': session.get('gettone', ''), 'utente': g.get('utente'), 'scuola': g.get('scuola')}


# ---- login e scelta della scuola ----

def login():
    from flask import current_app
    if request.method == 'POST':
        nome, password = request.form.get('nome', '').strip(), request.form.get('password', '')
        chiave = (request.remote_addr, nome.lower())
        n, quando = _falliti.get(chiave, (0, 0))
        if n >= MAX_TENTATIVI and time.time() - quando < ATTESA_SECONDI:
            flash('Troppi tentativi: riprova tra un minuto.', 'errore')
            return render_template('login.html'), 429
        u = conn().execute('select id, password_hash from utenti where nome = %s and attivo', (nome,)).fetchone()
        # controllo anche per nomi inesistenti, per non far capire se l'utente c'e'
        valida = check_password_hash(u['password_hash'] if u else generate_password_hash('x'), password)
        if u and valida:
            _falliti.pop(chiave, None)
            session.clear()
            session.permanent = True
            session['utente_id'] = u['id']
            session['gettone'] = os.urandom(16).hex()
            poi = request.args.get('poi') or url_for('home')
            return redirect(poi if poi.startswith('/') and not poi.startswith('//') else url_for('home'))
        _falliti[chiave] = (n + 1 if time.time() - quando < ATTESA_SECONDI else 1, time.time())
        flash('Nome o password sbagliati.', 'errore')
    if not conn().execute('select 1 from utenti limit 1').fetchone():
        return render_template('senza_password.html'), 503
    return render_template('login.html')


@bp.route('/scuole')
def scuole():
    elenco = scuole_dell_utente(conn(), g.utente)
    if len(elenco) == 1 and not request.args.get('cambia'):
        session['scuola_id'] = elenco[0]['id']
        return redirect(url_for('home'))
    return render_template('scuole.html', elenco=elenco)


@bp.route('/scuole/scegli', methods=['POST'])
def scegli_scuola():
    try:
        sid = int(request.form.get('scuola', ''))
    except ValueError:
        sid = None
    if sid not in [s['id'] for s in scuole_dell_utente(conn(), g.utente)]:
        flash('Scuola non disponibile per questo utente.', 'errore')
        return redirect(url_for('.scuole', cambia=1))
    session['scuola_id'] = sid
    return redirect(url_for('home'))


# ---- amministrazione ----

def _int(nome):
    try:
        return int(request.form.get(nome, ''))
    except ValueError:
        return None


@bp.route('/amministrazione')
def amministrazione():
    c = conn()
    scuole_tutte = c.execute('select s.*, (select count(*) from classi where scuola_id = s.id) as classi, '
                             '(select count(*) from docenti where scuola_id = s.id) as docenti from scuole s '
                             'order by nome').fetchall()
    utenti = c.execute('select * from utenti order by nome').fetchall()
    abbinate = {}
    for r in c.execute('select utente_id, scuola_id from utenti_scuole').fetchall():
        abbinate.setdefault(r['utente_id'], set()).add(r['scuola_id'])
    return render_template('amministrazione.html', scuole=scuole_tutte, utenti=utenti, abbinate=abbinate,
                           minimo=LUNGHEZZA_MINIMA_PASSWORD)


@bp.route('/amministrazione/scuola', methods=['POST'])
def amministrazione_scuola():
    import psycopg
    c = conn()
    azione, nome, sid = request.form.get('azione'), request.form.get('nome', '').strip(), _int('scuola')
    try:
        with c.transaction():
            if azione == 'nuova' and nome:
                crea_scuola(c, nome)
                flash('Scuola «%s» creata.' % nome, 'ok')
            elif azione == 'rinomina' and nome and sid:
                c.execute('update scuole set nome = %s where id = %s', (nome, sid))
                flash('Scuola rinominata.', 'ok')
            elif azione == 'elimina' and sid:
                if request.form.get('conferma') != request.form.get('nome_scuola'):
                    flash('Per eliminare la scuola scrivi il suo nome nella conferma.', 'errore')
                else:
                    c.execute('delete from scuole where id = %s', (sid,))
                    flash('Scuola eliminata, con tutti i suoi dati e le sue prove.', 'ok')
    except psycopg.errors.UniqueViolation:
        flash('Esiste già una scuola con questo nome.', 'errore')
    return redirect(url_for('.amministrazione'))


@bp.route('/amministrazione/utente', methods=['POST'])
def amministrazione_utente():
    import psycopg
    c = conn()
    azione, uid = request.form.get('azione'), _int('utente')
    nome, password = request.form.get('nome', '').strip(), request.form.get('password', '')
    try:
        with c.transaction():
            if azione == 'nuovo':
                if not nome or len(password) < LUNGHEZZA_MINIMA_PASSWORD:
                    flash('Servono un nome e una password di almeno %d caratteri.' % LUNGHEZZA_MINIMA_PASSWORD, 'errore')
                else:
                    uid = c.execute('insert into utenti (nome, password_hash, amministratore) values (%s, %s, %s) '
                                    'returning id', (nome, hash_password(password),
                                                     bool(request.form.get('amministratore')))).fetchone()['id']
                    flash('Utente «%s» creato: ora scegli le sue scuole.' % nome, 'ok')
            elif azione == 'password' and uid:
                if len(password) < LUNGHEZZA_MINIMA_PASSWORD:
                    flash('La password deve avere almeno %d caratteri.' % LUNGHEZZA_MINIMA_PASSWORD, 'errore')
                else:
                    c.execute('update utenti set password_hash = %s where id = %s', (hash_password(password), uid))
                    flash('Password cambiata.', 'ok')
            elif azione == 'scuole' and uid:
                scelte = [int(x) for x in request.form.getlist('scuole') if x.isdigit()]
                c.execute('delete from utenti_scuole where utente_id = %s', (uid,))
                for sid in scelte:
                    c.execute('insert into utenti_scuole (utente_id, scuola_id) select %s, id from scuole '
                              'where id = %s', (uid, sid))
                c.execute('update utenti set amministratore = %s where id = %s',
                          (bool(request.form.get('amministratore')), uid))
                _controlla_ultimo_amministratore(c)
                flash('Utente aggiornato.', 'ok')
            elif azione == 'attiva' and uid:
                c.execute('update utenti set attivo = not attivo where id = %s', (uid,))
                _controlla_ultimo_amministratore(c)
            elif azione == 'elimina' and uid:
                if uid == g.utente['id']:
                    flash('Non puoi eliminare te stesso.', 'errore')
                else:
                    c.execute('delete from utenti where id = %s', (uid,))
                    _controlla_ultimo_amministratore(c)
                    flash('Utente eliminato.', 'ok')
    except psycopg.errors.UniqueViolation:
        flash('Esiste già un utente con questo nome.', 'errore')
    except _UltimoAmministratore:
        flash('Deve restare almeno un amministratore attivo.', 'errore')
    return redirect(url_for('.amministrazione'))


class _UltimoAmministratore(Exception):
    pass


def _controlla_ultimo_amministratore(c):
    """Se la modifica toglie l'ultimo amministratore attivo, annulla tutto (l'eccezione fa tornare indietro la
    transazione di chi chiama)."""
    if not c.execute('select 1 from utenti where amministratore and attivo limit 1').fetchone():
        raise _UltimoAmministratore()
