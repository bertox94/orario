"""Sezione "Dati della scuola": calendario, classi, docenti e lezioni, al posto dei dati scritti nel programma."""
import datetime
import json
import os
import shutil
import tempfile

import psycopg
from flask import Blueprint, Response, current_app, flash, g, redirect, render_template, request, url_for

from . import modello, solver
from .db import conn

bp = Blueprint('dati', __name__, url_prefix='/dati')


def _config():
    return current_app.config['SITO']


def _sid():
    return g.scuola['id']


def _intero(nome, predefinito=None):
    try:
        return int(request.form.get(nome, ''))
    except ValueError:
        return predefinito


def _salva(azione, messaggio_ok):
    """Esegue una modifica al database; se viola un vincolo (nome ripetuto, ore non valide...) lo dice in chiaro."""
    c = conn()
    try:
        with c.transaction():
            azione(c)
        if messaggio_ok:
            flash(messaggio_ok, 'ok')
        return True
    except psycopg.errors.UniqueViolation:
        flash('Nome gia\' usato: scegline un altro.', 'errore')
    except psycopg.errors.CheckViolation:
        flash('Valori non validi (controlla ore e codocente).', 'errore')
    return False


def _verifica(dati_solver=None, caso=None):
    """Esegue il solver con --solo-verifica sui dati (formato solver) o su un caso. Ritorna il risultato."""
    cfg = _config()
    exe = solver.compila(cartella_bin=cfg.cartella_bin)
    os.makedirs(cfg.dati, exist_ok=True)
    cartella = tempfile.mkdtemp(prefix='verifica-', dir=cfg.dati)
    try:
        if caso is None:
            file = os.path.join(cartella, 'dati.json')
            with open(file, 'w') as f:
                json.dump(dati_solver, f, ensure_ascii=False)
            caso = {'tipo': 'input', 'file': file}
        return solver.esegui(exe, caso, {'solo_verifica': True, 'motivi': False, 'tempo': 60}, cartella)
    finally:
        shutil.rmtree(cartella, ignore_errors=True)


# ---- riepilogo ----

@bp.route('/')
def indice():
    dati = modello.leggi(conn(), _sid())
    versioni = conn().execute('select id, nome, motivo, creata, utente, classi, docenti, ore from versioni_dati '
                              'where scuola_id = %s order by creata desc, id desc limit 30', (_sid(),)).fetchall()
    return render_template('dati/indice.html', dati=dati, r=modello.riepilogo(dati), versioni=versioni)


@bp.route('/calendario', methods=['POST'])
def calendario():
    giorni = request.form.get('giorni', '').split()
    ore = request.form.get('ore', '').split()
    if not 1 <= len(giorni) <= 6 or not modello.ORE_OBBLIGATORIE <= len(ore) <= 6:
        flash('Servono da 1 a 6 giorni e da %d a 6 ore.' % modello.ORE_OBBLIGATORIE, 'errore')
    elif len(set(giorni)) != len(giorni) or len(set(ore)) != len(ore):
        flash('Giorni e ore devono avere nomi diversi.', 'errore')
    else:
        _salva(lambda c: c.execute('update calendario set giorni = %s, ore = %s where scuola_id = %s',
                                   (giorni, ore, _sid())), 'Calendario salvato.')
    return redirect(url_for('.indice'))


# ---- classi ----

@bp.route('/classi', methods=['POST'])
def nuova_classe():
    nome = request.form.get('nome', '').strip()
    if nome:
        _salva(lambda c: c.execute('insert into classi (scuola_id, nome) values (%s, %s)', (_sid(), nome)),
               'Classe %s aggiunta.' % nome)
    return redirect(url_for('.indice'))


@bp.route('/classi/<int:cid>')
def classe(cid):
    c = conn()
    cl = c.execute('select * from classi where id = %s and scuola_id = %s', (cid, _sid())).fetchone()
    if not cl:
        return redirect(url_for('.indice'))
    dati = modello.leggi(c, _sid())
    lezioni = [l for l in dati['lezioni'] if l['classe_id'] == cid]
    riep = next(x for x, k in zip(modello.riepilogo(dati)['classi'], dati['classi']) if k['id'] == cid)
    return render_template('dati/classe.html', classe=cl, lezioni=lezioni, docenti=dati['docenti'], riep=riep,
                           giorni=len(dati['giorni']), ore=len(dati['ore']), ore_obbligatorie=modello.ORE_OBBLIGATORIE)


@bp.route('/classi/<int:cid>/rinomina', methods=['POST'])
def rinomina_classe(cid):
    nome = request.form.get('nome', '').strip()
    if nome:
        _salva(lambda c: c.execute('update classi set nome = %s where id = %s and scuola_id = %s', (nome, cid, _sid())), 'Classe rinominata.')
    return redirect(url_for('.classe', cid=cid))


@bp.route('/classi/<int:cid>/elimina', methods=['POST'])
def elimina_classe(cid):
    _salva(lambda c: c.execute('delete from classi where id = %s and scuola_id = %s', (cid, _sid())), 'Classe eliminata, con le sue lezioni.')
    return redirect(url_for('.indice'))


# ---- docenti ----

@bp.route('/docenti', methods=['POST'])
def nuovo_docente():
    nome = request.form.get('nome', '').strip()
    ingresso = _intero('ingresso', 0)
    if nome:
        _salva(lambda c: c.execute('insert into docenti (scuola_id, nome, ingresso) values (%s, %s, %s)',
                                   (_sid(), nome, ingresso)),
               'Docente %s aggiunto.' % nome)
    return redirect(url_for('.indice') + '#docenti')


@bp.route('/docenti/<int:did>', methods=['POST'])
def modifica_docente(did):
    nome = request.form.get('nome', '').strip()
    ingresso = _intero('ingresso', 0)
    if nome:
        _salva(lambda c: c.execute('update docenti set nome = %s, ingresso = %s where id = %s and scuola_id = %s',
                                   (nome, ingresso, did, _sid())),
               'Docente aggiornato.')
    return redirect(url_for('.indice') + '#docenti')


@bp.route('/docenti/<int:did>/elimina', methods=['POST'])
def elimina_docente(did):
    _salva(lambda c: c.execute('delete from docenti where id = %s and scuola_id = %s', (did, _sid())),
           'Docente eliminato, con le sue lezioni (anche quelle in cui era codocente).')
    return redirect(url_for('.indice') + '#docenti')


# ---- lezioni ----

def _lezione_dal_modulo():
    """(docente, ore, codocente, ore_codocente) dal modulo, o None con il motivo in un messaggio."""
    docente, ore = _intero('docente'), _intero('ore')
    codocente = _intero('codocente')
    ore_cod = _intero('ore_codocente', 0) if codocente else 0
    if not docente or not ore or ore < 1:
        flash('Indica docente e ore (almeno 1).', 'errore')
        return None
    if codocente == docente:
        flash('Docente e codocente devono essere persone diverse.', 'errore')
        return None
    if codocente and not 1 <= ore_cod <= ore:
        flash('Le ore con il codocente devono essere da 1 alle ore della lezione.', 'errore')
        return None
    scelti = [d for d in (docente, codocente) if d]
    trovati = conn().execute('select count(*) as n from docenti where scuola_id = %s and id in (%s)' %
                             ('%s', ', '.join(['%s'] * len(scelti))), (_sid(),) + tuple(scelti)).fetchone()['n']
    if trovati != len(scelti):
        flash('Docente non trovato in questa scuola.', 'errore')
        return None
    return docente, ore, codocente or None, ore_cod


@bp.route('/classi/<int:cid>/lezioni', methods=['POST'])
def nuova_lezione(cid):
    l = _lezione_dal_modulo()
    if l:
        _salva(lambda c: c.execute('insert into lezioni (classe_id, docente_id, ore, codocente_id, ore_codocente) '
                                   'values (%s, %s, %s, %s, %s)', (cid,) + l), 'Lezione aggiunta.')
    return redirect(url_for('.classe', cid=cid))


@bp.route('/lezioni/<int:lid>', methods=['POST'])
def modifica_lezione(lid):
    cid = conn().execute('select classe_id from lezioni where id = %s', (lid,)).fetchone()['classe_id']
    l = _lezione_dal_modulo()
    if l:
        _salva(lambda c: c.execute('update lezioni set docente_id = %s, ore = %s, codocente_id = %s, ore_codocente = %s '
                                   'where id = %s', l + (lid,)), 'Lezione aggiornata.')
    return redirect(url_for('.classe', cid=cid))


@bp.route('/lezioni/<int:lid>/elimina', methods=['POST'])
def elimina_lezione(lid):
    cid = conn().execute('select classe_id from lezioni where id = %s', (lid,)).fetchone()['classe_id']
    _salva(lambda c: c.execute('delete from lezioni where id = %s', (lid,)), 'Lezione eliminata.')
    return redirect(url_for('.classe', cid=cid))


# ---- versioni salvate ----

def _sostituisci_con_copia(dati_solver, messaggio_ok, origine):
    """Sostituisce i dati della scuola; se la casella «salva prima» e' spuntata (di default si') salva prima una
    versione dei dati attuali, che si puo' ripristinare dall'elenco delle versioni."""
    salva = bool(request.form.get('salva_prima'))
    nome = 'Prima di importare: %s' % origine
    def azione(c):
        if salva:
            modello.salva_versione(c, _sid(), nome, 'prima di importare', g.utente['nome'])
        modello.sostituisci(c, _sid(), dati_solver)
    return _salva(azione, messaggio_ok + (' I dati precedenti sono nelle versioni salvate.' if salva else ''))


@bp.route('/versioni', methods=['POST'])
def salva_versione():
    nome = request.form.get('nome', '').strip() or datetime.datetime.now().strftime('Versione del %d/%m/%Y %H:%M')
    def azione(c):
        if modello.salva_versione(c, _sid(), nome[:120], 'manuale', g.utente['nome']) is None:
            raise _NienteDaSalvare()
    try:
        _salva(azione, 'Versione «%s» salvata.' % nome[:120])
    except _NienteDaSalvare:
        flash('Non ci sono ancora dati da salvare.', 'errore')
    return redirect(url_for('.indice') + '#versioni')


class _NienteDaSalvare(Exception):
    pass


def _versione(vid):
    return conn().execute('select * from versioni_dati where id = %s and scuola_id = %s', (vid, _sid())).fetchone()


@bp.route('/versioni/<int:vid>/ripristina', methods=['POST'])
def ripristina_versione(vid):
    v = _versione(vid)
    if not v:
        flash('Versione non trovata.', 'errore')
        return redirect(url_for('.indice') + '#versioni')
    def azione(c):
        modello.salva_versione(c, _sid(), 'Prima di ripristinare «%s»' % v['nome'][:80], 'prima di ripristinare',
                               g.utente['nome'])
        modello.sostituisci(c, _sid(), v['dati'])
    _salva(azione, 'Versione «%s» ripristinata (i dati di prima sono salvati come nuova versione).' % v['nome'])
    return redirect(url_for('.indice') + '#versioni')


@bp.route('/versioni/<int:vid>/elimina', methods=['POST'])
def elimina_versione(vid):
    _salva(lambda c: c.execute('delete from versioni_dati where id = %s and scuola_id = %s', (vid, _sid())),
           'Versione eliminata.')
    return redirect(url_for('.indice') + '#versioni')


@bp.route('/versioni/<int:vid>.json')
def scarica_versione(vid):
    v = _versione(vid)
    if not v:
        return redirect(url_for('.indice'))
    return Response(json.dumps(v['dati'], ensure_ascii=False, indent=1), mimetype='application/json',
                    headers={'Content-Disposition': 'attachment; filename=versione_%d.json' % vid})


# ---- verifica, importazione, esportazione ----

@bp.route('/verifica', methods=['POST'])
def verifica():
    dati = modello.leggi(conn(), _sid())
    if not dati['classi'] or not dati['docenti']:
        flash('Prima inserisci almeno una classe e un docente.', 'errore')
        return redirect(url_for('.indice'))
    r = _verifica(modello.per_solver(dati))
    if r['esito'] == 'verifica superata':
        flash('Verifica preliminare superata: nessuna condizione necessaria violata (non garantisce che un orario '
              'esista: lo dice solo la ricerca).', 'ok')
    elif r['esito'] == 'impossibile':
        flash('Dati impossibili: ' + r['impossibile'], 'errore')
    elif r['esito'] == 'errore nei dati':
        flash('Errore nei dati: ' + r['errore_dati'], 'errore')
    else:
        flash('Verifica non riuscita (%s).' % r['esito'], 'errore')
    return redirect(url_for('.indice'))


@bp.route('/importa-programma', methods=['POST'])
def importa_programma():
    """Copia nel database i dati scritti nel programma (inizializza_dati), sostituendo quelli presenti."""
    if not request.form.get('conferma'):
        flash('Spunta la conferma: l\'importazione sostituisce tutti i dati presenti.', 'errore')
        return redirect(url_for('.indice'))
    r = _verifica(caso={'tipo': 'programma'})
    if not r.get('json'):
        flash('Il solver non ha scritto i dati (%s).' % r['esito'], 'errore')
        return redirect(url_for('.indice'))
    _sostituisci_con_copia(r['json']['dati'], 'Importati i dati del programma.', 'dati del programma')
    return redirect(url_for('.indice'))


@bp.route('/importa-file', methods=['POST'])
def importa_file():
    """Sostituisce i dati con quelli di un file JSON (formato --input, o un risultato.json di una prova)."""
    f = request.files.get('file')
    if not request.form.get('conferma') or not f:
        flash('Scegli un file e spunta la conferma: l\'importazione sostituisce tutti i dati presenti.', 'errore')
        return redirect(url_for('.indice'))
    try:
        contenuto = json.load(f)
    except ValueError:
        flash('Il file non e\' JSON valido.', 'errore')
        return redirect(url_for('.indice'))
    dati = contenuto.get('dati', contenuto) if isinstance(contenuto, dict) else None
    r = _verifica(dati) if isinstance(dati, dict) else {'esito': 'errore nei dati', 'errore_dati': 'formato sconosciuto'}
    if r['esito'] == 'errore nei dati' or not r.get('json'):
        flash('File non valido: %s' % (r.get('errore_dati') or r['esito']), 'errore')
        return redirect(url_for('.indice'))
    _sostituisci_con_copia(r['json']['dati'], 'Dati importati dal file.', 'file %s' % (f.filename or '')[:60])
    return redirect(url_for('.indice'))


@bp.route('/esporta.json')
def esporta():
    testo = json.dumps(modello.per_solver(modello.leggi(conn(), _sid())), ensure_ascii=False, indent=1)
    return Response(testo, mimetype='application/json',
                    headers={'Content-Disposition': 'attachment; filename=dati_scuola.json'})
