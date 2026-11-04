// Componenti della pagina del caso e della prova in corso: griglia dell'orario (con la coda dei docenti al passaggio del
// mouse o al tocco), orario per docente, mappa dei blocchi in tempo reale e la ricerca passo passo (slider, animazione,
// blocchi). I dati arrivano dal JSON del solver (--json), dalle righe di progresso (--progresso), dal file della ricerca
// passo passo (/casi/<id>/passi) e dall'analisi di uno slot (--analizza-slot, /casi/<id>/analisi).

function el(tag, attributi, figli) {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attributi || {})) {
    if (k === 'testo') e.textContent = v; else if (typeof v === 'function') e[k] = v; else e.setAttribute(k, v);
  }
  for (const f of figli || []) e.appendChild(f);
  return e;
}

// Intestazione comune: classe | Lun 08 | Lun 09 | ... (una colonna per giorno e ora).
function intestazione(dati, titolo) {
  const r1 = el('tr', {}, [el('th', {class: 'classe', rowspan: 2, testo: titolo || 'Classe'})]);
  const r2 = el('tr');
  dati.giorni.forEach((g, gi) => {
    r1.appendChild(el('th', {colspan: dati.ore.length, class: 'inizio-giorno', testo: g}));
    dati.ore.forEach((o, oi) => r2.appendChild(el('th', {class: oi === 0 ? 'inizio-giorno' : '', testo: o})));
  });
  return el('thead', {}, [r1, r2]);
}

// Griglia dell'orario: orario[g][o][c] = [docente, codocente] (-1 = nessuno).
// passaggi (facoltativo): [g][o][c] = quante volte la ricerca e' passata da quella posizione; colora lo sfondo
// delle celle con la stessa scala di calore della mappa (logaritmica, piu' scuro = piu' volte).
function disegnaOrario(contenitore, dati, orario, blocco, alPassaggio, passaggi) {
  let massimo = 0;  // il primo passaggio e' normale: si colora solo cio' che e' stato attraversato piu' di una volta
  if (passaggi) dati.giorni.forEach((_, g) => dati.ore.forEach((__, o) => dati.classi.forEach((___, c) => {
    massimo = Math.max(massimo, passaggi[g][o][c] - 1);
  })));
  const corpo = el('tbody');
  dati.classi.forEach((cl, c) => {
    const tr = el('tr', {}, [el('td', {class: 'classe', testo: cl.nome})]);
    dati.giorni.forEach((_, g) => dati.ore.forEach((__, o) => {
      const [p, q] = orario[g][o][c];
      const qui = blocco && blocco[0] === g && blocco[1] === o && blocco[2] === c;
      const td = el('td', {class: (o === 0 ? 'inizio-giorno ' : '') + (p < 0 ? 'vuoto ' : '') + (qui ? 'blocco' : '')});
      if (p < 0) td.textContent = qui ? '✕' : '—';
      else {
        td.appendChild(el('span', {testo: dati.docenti[p].nome}));
        if (q >= 0) td.appendChild(el('span', {class: 'cod', testo: '+ ' + dati.docenti[q].nome}));
        td.title = dati.docenti[p].nome + (q >= 0 ? ' con ' + dati.docenti[q].nome : '');
      }
      if (passaggi) {
        const v = passaggi[g][o][c];
        if (v > 1 && massimo > 0) {
          const a = 0.08 + 0.82 * Math.log1p(v - 1) / Math.log1p(massimo);
          td.style.background = `rgba(var(--calore), ${a.toFixed(3)})`;
          if (a > 0.5) td.style.color = '#fff';
        }
        td.title = (td.title ? td.title + ' · ' : '') + `attraversata ${v.toLocaleString('it-IT')} ` + (v === 1 ? 'volta' : 'volte');
      }
      if (alPassaggio) collegaCella(td, (ev) => alPassaggio(g, o, c, td, ev), alPassaggio);
      tr.appendChild(td);
    }));
    corpo.appendChild(tr);
  });
  contenitore.replaceChildren(el('table', {class: 'orario'}, [intestazione(dati), corpo]));
}

// Collega una cella al pannello della coda: con il mouse si apre passandoci sopra, con il dito (iPad, telefono: niente
// passaggio del mouse) toccandola; il tocco funziona anche con il mouse (clic). apri(ev) apre, chiudi(null) chiude.
const SENZA_MOUSE = window.matchMedia && matchMedia('(hover: none)').matches;
function collegaCella(td, apri, chiudi) {
  if (!SENZA_MOUSE) {
    td.addEventListener('mouseenter', ev => apri(ev));
    td.addEventListener('mouseleave', () => chiudi(null));
  }
  td.addEventListener('click', ev => { ev.stopPropagation(); apri(ev); });
  td.classList.add('con-coda');
}

// Orario di ogni docente: una riga per docente, nella cella la classe in cui insegna (con il codocente, se c'e').
// Un docente in codocenza compare in entrambe le righe: nella cella c'e' la classe e, piccolo, l'altro docente.
function disegnaOrarioDocenti(contenitore, dati, orario, alPassaggio) {
  const corpo = el('tbody');
  dati.docenti.forEach((d, p) => {
    const tr = el('tr', {}, [el('td', {class: 'classe', testo: d.nome})]);
    dati.giorni.forEach((_, g) => dati.ore.forEach((__, o) => {
      let classe = -1, altro = -1;
      orario[g][o].forEach(([a, b], c) => {
        if (a === p) { classe = c; altro = b; } else if (b === p) { classe = c; altro = a; }
      });
      const td = el('td', {class: (o === 0 ? 'inizio-giorno ' : '') + (classe < 0 ? 'vuoto' : '')});
      if (classe < 0) td.textContent = '—';
      else {
        td.appendChild(el('span', {testo: dati.classi[classe].nome}));
        if (altro >= 0) td.appendChild(el('span', {class: 'cod', testo: '+ ' + dati.docenti[altro].nome}));
        td.title = dati.classi[classe].nome + (altro >= 0 ? ' con ' + dati.docenti[altro].nome : '');
        if (alPassaggio) {
          const cl = classe;
          collegaCella(td, (ev) => alPassaggio(g, o, cl, td, ev), alPassaggio);
        }
      }
      tr.appendChild(td);
    }));
    corpo.appendChild(tr);
  });
  contenitore.replaceChildren(el('table', {class: 'orario'}, [intestazione(dati, 'Docente'), corpo]));
}

// Mappa dei blocchi (prova in corso): valore(g, o, c) per ogni slot; colore proporzionale (in scala logaritmica) al massimo.
function disegnaMappa(contenitore, dati, valore, descrizione) {
  let massimo = 0;
  dati.giorni.forEach((_, g) => dati.ore.forEach((__, o) => dati.classi.forEach((___, c) => {
    massimo = Math.max(massimo, valore(g, o, c));
  })));
  let totale = 0;
  dati.giorni.forEach((_, g) => dati.ore.forEach((__, o) => dati.classi.forEach((___, c) => { totale += valore(g, o, c); })));
  const percento = v => totale ? (100 * v / totale).toLocaleString('it-IT', {maximumFractionDigits: 1}) + '%' : '-';
  const scala = v => v <= 0 ? 0 : Math.log1p(v) / Math.log1p(massimo);
  const corpo = el('tbody');
  dati.classi.forEach((cl, c) => {
    const tr = el('tr', {}, [el('td', {class: 'classe', testo: cl.nome})]);
    dati.giorni.forEach((g_nome, g) => dati.ore.forEach((o_nome, o) => {
      const v = valore(g, o, c);
      const td = el('td', {class: o === 0 ? 'inizio-giorno' : ''});
      if (v > 0) {
        const a = 0.08 + 0.92 * scala(v);
        td.style.background = `rgba(var(--calore), ${a.toFixed(3)})`;
        if (a > 0.55) td.style.color = '#fff';
        td.textContent = compatto(v);
      }
      td.title = `${g_nome} ${o_nome} ${cl.nome}: ${v.toLocaleString('it-IT')} ${descrizione} (${percento(v)} del totale)`;
      tr.appendChild(td);
    }));
    corpo.appendChild(tr);
  });
  const legenda = el('div', {class: 'legenda'}, [
    el('span', {testo: '0'}), el('span', {class: 'scala'}),
    el('span', {testo: massimo.toLocaleString('it-IT') + ' ' + descrizione + ' (scala logaritmica)'})]);
  // la mappa in tempo reale si ridisegna ogni secondo: si conserva lo scorrimento orizzontale
  const prima = contenitore.querySelector('.scorre');
  const sinistra = prima ? prima.scrollLeft : 0;
  const box = el('div', {class: 'scorre'}, [el('table', {class: 'orario mappa'}, [intestazione(dati), corpo])]);
  contenitore.replaceChildren(legenda, box);
  box.scrollLeft = sinistra;
}

function compatto(v) {
  if (v >= 1e6) return (v / 1e6).toFixed(v >= 1e7 ? 0 : 1) + 'M';
  if (v >= 1e3) return (v / 1e3).toFixed(v >= 1e4 ? 0 : 1) + 'k';
  return String(v);
}

// I blocchi di uno slot per la mappa: quante volte la ricerca e' arrivata a quello slot mentre era la posizione massima
// raggiunta, senza superarla (diagnosi.blocchi del JSON del solver). Le prove fatte prima di questo conteggio hanno
// solo marce indietro e rami scartati: si sommano quelli.
function blocchiDaRisultato(diagnosi) {
  if (diagnosi.blocchi) return (g, o, c) => diagnosi.blocchi[g][o][c];
  const scartati = Object.values(diagnosi.scartati || {});
  return (g, o, c) => diagnosi.marce[g][o][c] + scartati.reduce((s, t) => s + t[g][o][c], 0);
}

// Lo stesso dalla riga di progresso (prova in corso): tabella piatta con indice (g * ore + o) * classi + c.
function blocchiDaProgresso(dati, p) {
  const O = dati.ore.length, C = dati.classi.length;
  const t = p.blocchi_slot || p.marce_slot || [];
  return (g, o, c) => t[(g * O + o) * C + c] || 0;
}

function numeroIt(v) { return v == null ? '-' : Number(v).toLocaleString('it-IT'); }

// Il dettaglio di un ramo scartato dalla potatura (nella finestra della coda, da --analizza-slot): la scelta scartata e i
// numeri della verifica, lezione per lezione (lezioni della classe) o per il docente (ore dei docenti).
const volte = n => n === 1 ? '1 volta' : `${numeroIt(n)} volte`;
const oreDi = (fatte, tot) => `${fatte} ore su ${tot} (${tot - fatte === 1 ? 'ne manca 1' : `ne mancano ${tot - fatte}`})`;
const nomeDoc = (dati, p) => p >= 0 && dati.docenti[p] ? dati.docenti[p].nome : '?';
const coppia = (dati, p, q) => nomeDoc(dati, p) + (q >= 0 ? ' + ' + nomeDoc(dati, q) : '');

function dettaglioScartoLezioni(dati, det, s, slotNome) {
  const classe = dati.classi[det.classe].nome;
  const fineSett = s.fino_ora === dati.ore.length - 1;
  const entro = fineSett ? 'entro la fine della settimana' : `entro le ${dati.ore[s.fino_ora]}`;
  const parti = [el('h4', {testo: `Ramo scartato${s.volte != null ? ` (${volte(s.volte)})` : ''}: ${coppia(dati, s.scelta[0], s.scelta[1])} in ${slotNome}`}),
    el('p', {testo: `Con questa scelta la classe ${classe} arrivava a ${oreDi(s.ore_classe_fatte, det.ore_classe)}. ` +
      (fineSett ? `Entro la fine della settimana doveva fare ancora ${s.da_riempire} ore, ma le sue lezioni ne potevano fare al massimo ${s.possibili}.`
                : `Fino alle ${dati.ore[s.fino_ora]} di ogni giorno che resta ogni ora è obbligatoria: servivano ancora ${s.da_riempire} lezioni ` +
                  `entro quell'ora, ma ne potevano cadere al massimo ${s.possibili}.`) +
      ' La verifica se n\'è accorta subito e la ricerca ha provato un\'altra scelta.'})];
  const somma = (s.lezioni || []).reduce((x, l) => x + l[3], 0);
  if (s.lezioni && s.lezioni.length) {
    const righe = s.lezioni.slice().sort((x, y) => (y[2] - y[3]) - (x[2] - x[3])).map(([p, q, rimaste, possibili]) => {
      const note = [];
      [p, q].forEach(x => {
        const d = x >= 0 && dati.docenti[x];
        if (d && d.ingresso && (fineSett || d.ingresso > s.fino_ora)) note.push(`${d.nome} entra alle ${dati.ore[d.ingresso]}`);
      });
      if (possibili < rimaste && !note.length) note.push('non ci stanno: al massimo per un numero di ore al giorno nella classe, e ore attaccate');
      const tr = el('tr', {}, [el('td', {testo: coppia(dati, p, q), style: 'white-space: nowrap'}), el('td', {class: 'num', testo: String(rimaste)}),
        el('td', {class: 'num', testo: String(possibili)}), el('td', {class: 'piccolo', testo: note.join('; ')})]);
      if (possibili < rimaste) tr.className = 'non-ci-sta';
      return tr;
    });
    parti.push(el('table', {class: 'dati'}, [
      el('thead', {}, [el('tr', {}, [el('th', {testo: 'Lezione'}), el('th', {class: 'num', testo: 'Ore rimaste'}),
        el('th', {class: 'num', testo: `Ore possibili ${entro}`}), el('th', {testo: 'Note'})])]),
      el('tbody', {}, righe),
      el('tfoot', {}, [el('tr', {}, [el('td', {testo: 'Totale'}), el('td', {class: 'num', testo: String(s.lezioni.reduce((x, l) => x + l[2], 0))}),
        el('td', {class: 'num', testo: String(somma)}), el('td', {class: 'piccolo', testo: `ne servivano ${s.da_riempire}`})])])]));
  }
  // se il limite e' il numero di slot al giorno e non le singole lezioni, lo si dice
  const perGiorni = Math.min(s.oggi, s.slot_oggi) + s.giorni_dopo * Math.min(s.in_un_giorno, s.slot_al_giorno);
  if (s.lezioni && perGiorni < somma) {
    parti.push(el('p', {class: 'piccolo', testo: `Il limite qui è giornaliero: in un giorno, ${entro}, le lezioni possono coprire al massimo ` +
      `${Math.min(s.in_un_giorno, s.slot_al_giorno)} slot su ${s.slot_al_giorno}, e restano ${s.giorni_dopo} giorni (oggi al massimo ` +
      `${Math.min(s.oggi, s.slot_oggi)}): in tutto ${perGiorni}.`}));
  }
  return parti;
}

function dettaglioScartoDocenti(dati, det, s, slotNome) {
  const d = dati.docenti[s.docente];
  const dove = s.classe_docente >= 0 ? `nella classe ${dati.classi[s.classe_docente].nome}` : 'in tutto (in tutte le sue classi)';
  return [el('h4', {testo: `Ramo scartato${s.volte != null ? ` (${volte(s.volte)})` : ''}: dopo l'ultima scelta dell'ora, ${coppia(dati, s.scelta[0], s.scelta[1])} in ${slotNome}`}),
    el('p', {testo: `Finita quest'ora, ${d ? d.nome : 'un docente'} aveva ancora ${s.ore_rimaste} ore da fare ${dove}, ma nel tempo rimasto ` +
      `ne poteva fare al massimo ${s.ore_possibili}` + (d && d.ingresso ? ` (entra alle ${dati.ore[d.ingresso]})` : '') +
      (s.classe_docente >= 0 ? ', perché in una classe può fare solo un massimo di ore al giorno, attaccate.' : '.') +
      ` La classe dello slot aveva fatto ${oreDi(s.ore_classe_fatte, det.ore_classe)}. La ricerca ha provato un'altra scelta.`})];
}

// La coda dei docenti al passaggio del mouse su una cella di un orario: per lo slot (giorno, ora, classe), dato
// l'orario parziale fino a quello slot, il solver (--analizza-slot, POST su urlAnalisi) dice la coda in ordine di
// priorita' e perche' ognuno e' o non e' utilizzabile. Ritorna passaggio(fonte) -> (g, o, c, td): fonte e' l'orario
// mostrato {chiave, orario, ordine (ordine delle classi nella ricerca), limite (ultimo slot raggiunto), blocco}.
function codaAlPassaggio(dati, urlAnalisi) {
  const contenuto = el('div');
  const chiudi = el('button', {type: 'button', class: 'chiudi-pannello', title: 'Chiudi', 'aria-label': 'Chiudi', testo: '✕'});
  const pannello = el('div', {class: 'pannello-slot'}, [chiudi, contenuto]);
  pannello.style.display = 'none';
  document.body.appendChild(pannello);
  const cache = {};
  let timer = null, cellaAperta = null;
  let rettangolo = null, timerChiusura = null;
  const nascondi = () => { clearTimeout(timer); clearTimeout(timerChiusura); pannello.style.display = 'none'; cellaAperta = null; };
  // Posizione e altezza in base al contenuto: mai oltre lo schermo, e se il contenuto e' piu' alto il pannello si scorre
  // (con il dito o la rotella). Si chiama dopo ogni cambio di contenuto.
  const posiziona = () => {
    if (pannello.style.display === 'none') return;
    if (window.innerWidth >= 760 && rettangolo) {
      pannello.style.maxHeight = (window.innerHeight - 16) + 'px';
      pannello.style.left = Math.max(8, Math.min(rettangolo.right + 8, window.innerWidth - pannello.offsetWidth - 8)) + 'px';
      pannello.style.top = Math.max(8, Math.min(rettangolo.top, window.innerHeight - pannello.offsetHeight - 8)) + 'px';
    } else {
      pannello.style.maxHeight = pannello.style.left = pannello.style.top = '';
    }
  };
  chiudi.addEventListener('click', ev => { ev.stopPropagation(); nascondi(); });
  pannello.addEventListener('click', ev => ev.stopPropagation());
  // con il mouse: spostandosi dalla cella al pannello (per scorrerlo con la rotella) il pannello resta aperto
  pannello.addEventListener('mouseenter', () => clearTimeout(timerChiusura));
  pannello.addEventListener('mouseleave', () => { if (!SENZA_MOUSE) nascondi(); });
  document.addEventListener('click', () => { if (cellaAperta) nascondi(); });  // tocco fuori: chiude
  const C = dati.classi.length, O = dati.ore.length;
  return fonte => (g, o, c, td, ev) => {
    clearTimeout(timer);
    if (g == null) {  // il mouse e' uscito dalla cella: breve attesa, il tempo di arrivare sul pannello
      clearTimeout(timerChiusura);
      timerChiusura = setTimeout(nascondi, 250);
      return;
    }
    clearTimeout(timerChiusura);
    const tocco = ev && ev.type === 'click';
    if (tocco && SENZA_MOUSE && cellaAperta === td) { nascondi(); return; }  // secondo tocco sulla stessa cella: chiude
    const f = typeof fonte === 'function' ? fonte() : fonte;
    if (!f) return;
    const pos = (g * O + o) * C + f.ordine.indexOf(c);
    if (f.limite != null && pos > f.limite) return;  // oltre il blocco la ricerca non e' arrivata
    timer = setTimeout(async () => {
      const chiave = `${f.chiave}:${pos}`;
      cellaAperta = td;
      pannello.style.display = 'block';
      // schermo stretto (telefono, iPad in verticale): il pannello in basso, largo come lo schermo
      pannello.classList.toggle('in-basso', window.innerWidth < 760);
      rettangolo = td.getBoundingClientRect();
      contenuto.textContent = 'Calcolo della coda...';
      posiziona();
      if (!cache[chiave]) {
        const slot = [];
        for (let i = 0; i < pos; i++) {
          const gg = Math.floor(i / (O * C)), oo = Math.floor(i / C) % O, cc = f.ordine[i % C];
          const [p, q] = f.orario[gg][oo][cc];
          slot.push(p < 0 ? '-' : q >= 0 ? `${p}+${q}` : String(p));
        }
        try {
          const risp = await fetch(urlAnalisi, {method: 'POST', body: JSON.stringify({pos, slot}),
            headers: {'Content-Type': 'application/json', 'X-Gettone': window.GETTONE}});
          cache[chiave] = await risp.json();
          if (cache[chiave].potatura && pos > 0) {  // la causa e' la scelta dello slot prima
            const k = pos - 1, cc = f.ordine[k % C];
            cache[chiave].potatura.slotNome = `${dati.giorni[Math.floor(k / (O * C))]} ${dati.ore[Math.floor(k / C) % O]} ${dati.classi[cc].nome}`;
          }
        } catch (e) { cache[chiave] = {errore: 'Analisi non disponibile.'}; }
      }
      if (pannello.style.display === 'none' || cellaAperta !== td) return;
      mostraAnalisi(contenuto, dati, cache[chiave], f.orario[g][o][c], `${dati.giorni[g]} ${dati.ore[o]} ${dati.classi[c].nome}`,
        f.limite != null && pos === f.limite ? (f.testoLimite || true) : false, c, o, pannello);
      posiziona();
      pannello.scrollTop = 0;
    }, tocco ? 0 : 250);
  };
}

// La ricerca passo passo (registrata con --passi): l'orario parziale a ogni passo, dal passo 0 (orario vuoto), con lo
// slider (aggiorna in tempo reale), i pulsanti, l'animazione a velocita' regolabile e i pulsanti dei blocchi (vicoli
// ciechi e rami scartati). La sezione resta nascosta finche' non si sa che c'e' almeno un passo registrato.
function sfogliaPassi(sezione, contenitore, dati, url, nomiMotivi) {
  const barra = el('input', {type: 'range', min: 0, max: 0, value: 0, style: 'width: 100%'});
  const numero = el('input', {type: 'number', min: 0, value: 0, class: 'stretto'});
  const stato = el('div', {class: 'tenue piccolo'});
  const griglia = el('div', {class: 'scorre'});
  const motivi = el('div', {});
  // spazio riservato (la riga si riempie dopo il caricamento: senza, la tabella si sposta e un tocco puo' finire su un'altra cella)
  const infoVicoli = el('div', {class: 'tenue piccolo', style: 'min-height: 2.6em; flex: 1 1 100%'});
  let totale = null, n = 0, richiesta = 0, vicoli = [], vicoliTroncati = false, ultimo = null;
  // passando il mouse (o toccando) una cella: la coda dei docenti in quel punto e i vincoli, come nelle marce indietro
  const passaggio = codaAlPassaggio(dati, url.replace(/passi$/, 'analisi'))(
    () => ultimo && {chiave: 'passo' + n, orario: ultimo.orario, ordine: ultimo.ordine, limite: ultimo.pos,
      testoLimite: ultimo.esito === 'scarto' ? ' · ramo scartato: la ricerca è tornata indietro subito dopo la scelta evidenziata'
      : ultimo.bloccato ? (ultimo.esito === 'vuoto' ? ' · qui nessun docente era utilizzabile: lo slot resta vuoto' : ' · qui nessun docente era utilizzabile (vicolo cieco)')
      : ultimo.pos >= ultimo.slot ? ' · orario completo' : ' · qui la ricerca sta decidendo'});
  const veri = () => vicoli.filter(v => v[1] === 'indietro' || v[1] === 'scarto');  // i blocchi: veri ritorni indietro
  const di = () => totale ? ` di ${numeroIt(totale)}` : '';
  function scriviStato(r) {
    const scarto = r.esito === 'scarto';
    const dove = scarto ? `ultima scelta: ${dati.giorni[r.blocco[0]]} ${dati.ore[r.blocco[1]]} ${dati.classi[r.blocco[2]].nome} (✕)`
      : r.pos >= r.slot ? 'orario completo'
      : `la ricerca sta decidendo ${dati.giorni[r.blocco[0]]} ${dati.ore[r.blocco[1]]} ${dati.classi[r.blocco[2]].nome} (✕)`;
    stato.textContent = `Passo ${numeroIt(n)}${di()} · ${numeroIt(r.pos)} slot su ${numeroIt(r.slot)} assegnati; ${dove}`
      + (scarto ? ` · ✂ RAMO SCARTATO: con questa scelta ${r.motivoScarto === 1 ? 'i docenti non avrebbero più ore sufficienti per finire' : r.motivoScarto === 0 ? 'la classe non potrebbe più completare le sue lezioni' : 'la ricerca non può più finire l\'orario (il motivo non è registrato in questa prova)'}, la ricerca torna indietro subito`
        : !r.bloccato ? '' : r.esito === 'vuoto' ? ' · ⚠ NESSUN DOCENTE UTILIZZABILE: lo slot resta vuoto (ora buca) e la ricerca prosegue'
        : r.esito === 'indietro' ? ' · ⚠ VICOLO CIECO: nessun docente utilizzabile e lo slot non può restare vuoto, la ricerca torna indietro'
        : ' · ⚠ NESSUN DOCENTE UTILIZZABILE: com\'è andata dopo non è registrato');
    stato.style.fontWeight = r.bloccato || scarto ? '600' : '';
  }

  // ---- animazione: i passi si scaricano a blocchi (sole differenze) e si ridisegnano a velocita' regolabile ----
  const VELOCITA = [1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500];  // passi al secondo
  const anim = {attiva: false, passi: [], da: 0, slot: [], conta: {}, ordine: [], finito: false, carica: false, acc: 0, t: 0, raf: 0, ultimoCodice: 0};
  const bottonePlay = el('button', {type: 'button', testo: '▶ Avvia l\'animazione', onclick: () => anim.attiva ? ferma(true) : avvia()});
  const sliderVelocita = el('input', {type: 'range', min: 0, max: VELOCITA.length - 1, value: 4, 'aria-label': 'Velocità dell\'animazione', style: 'width: 160px'});
  const etichettaVelocita = el('span', {class: 'tenue piccolo', style: 'min-width: 9em'});
  const aggiornaVelocita = () => { etichettaVelocita.textContent = `${numeroIt(VELOCITA[+sliderVelocita.value])} passi al secondo`; };
  sliderVelocita.addEventListener('input', aggiornaVelocita);
  aggiornaVelocita();
  async function caricaBlocco(da, dimensione) {
    anim.carica = true;
    try {
      const r = await (await fetch(`${url}?da=${da}&k=${dimensione}`)).json();
      return r;
    } finally { anim.carica = false; }
  }
  const dimensioneBlocco = () => Math.max(300, Math.min(2000, VELOCITA[+sliderVelocita.value] * 3));
  async function avvia() {
    if (!totale || anim.avviando) return;
    anim.avviando = true;
    if (n >= totale) { n = 0; barra.value = numero.value = 0; }
    motivi.replaceChildren();
    bottonePlay.textContent = '⏳ Carico...';
    const mio = ++richiesta;
    let r;
    try { r = await caricaBlocco(n + 1, dimensioneBlocco()); } catch (e) { anim.avviando = false; bottonePlay.textContent = '▶ Avvia l\'animazione'; return; }
    anim.avviando = false;
    if (mio !== richiesta || anim.attiva) { if (!anim.attiva) bottonePlay.textContent = '▶ Avvia l\'animazione'; return; }
    if (r.errore || !r.passi.length) { bottonePlay.textContent = '▶ Avvia l\'animazione'; if (r.errore) stato.textContent = r.errore; return; }
    anim.slot = r.iniziale.slot.slice();
    anim.conta = {};
    r.iniziale.conta.forEach(([i, v]) => { anim.conta[i] = v; });
    anim.ordine = r.ordine; anim.passi = r.passi; anim.da = n + 1 + r.passi.length; anim.finito = r.finito;
    anim.attiva = true; anim.acc = 0; anim.t = 0;
    bottonePlay.textContent = '⏸ Pausa';
    anim.raf = requestAnimationFrame(passo);
  }
  function ferma(ricarica) {
    cancelAnimationFrame(anim.raf);
    anim.attiva = false;
    bottonePlay.textContent = '▶ Avvia l\'animazione';
    if (ricarica) vai(n);  // da fermo: lo stato esatto del passo, con i motivi e la finestra al passaggio del mouse
  }
  const prossimoBlocco = () => {
    if (anim.carica || anim.finito) return;
    caricaBlocco(anim.da, dimensioneBlocco()).then(r => {
      if (!anim.attiva) return;
      anim.passi = anim.passi.concat(r.passi); anim.da += r.passi.length; anim.finito = r.finito || !r.passi.length;
    }).catch(() => { anim.finito = true; });
  };
  // l'orario [g][o][c] e i passaggi [g][o][c] dallo stato locale (slot e conteggi per posizione)
  function disegnaLocale(codice) {
    const G = dati.giorni.length, O = dati.ore.length, C = dati.classi.length;
    const orario = dati.giorni.map(() => dati.ore.map(() => dati.classi.map(() => [-1, -1])));
    anim.slot.forEach((t, i) => {
      if (t === '-') return;
      const [a, q] = t.split('+');
      orario[Math.floor(i / (O * C))][Math.floor(i / C) % O][anim.ordine[i % C]] = [+a, q === undefined ? -1 : +q];
    });
    const passaggi = dati.giorni.map(() => dati.ore.map(() => dati.classi.map(() => 0)));
    for (const i in anim.conta) passaggi[Math.floor(i / (O * C))][Math.floor(i / C) % O][anim.ordine[i % C]] = anim.conta[i];
    const pos = anim.slot.length, completo = pos >= G * O * C;
    const scarto = codice >= 4;
    const k = scarto ? pos - 1 : Math.min(pos, G * O * C - 1);
    const blocco = [Math.floor(k / (O * C)), Math.floor(k / C) % O, anim.ordine[k % C]];
    const r = {n, pos, slot: G * O * C, blocco, bloccato: codice > 0 && !scarto, motivoScarto: scarto ? (codice === 6 ? -1 : codice - 4) : null,
      esito: scarto ? 'scarto' : codice === 1 ? 'vuoto' : codice === 2 ? 'indietro' : null};
    scriviStato(r);
    disegnaOrario(griglia, dati, orario, completo && !scarto ? null : blocco, null, passaggi);
    teniInVista();
  }
  function passo(t) {
    if (!anim.attiva) return;
    const dt = anim.t ? Math.min(100, t - anim.t) : 0;
    anim.t = t;
    anim.acc += dt / 1000 * VELOCITA[+sliderVelocita.value];
    let quanti = Math.floor(anim.acc);
    anim.acc -= quanti;
    let fatti = 0;
    while (quanti > 0 && anim.passi.length) {
      const [pos, da, nuovi, codice] = anim.passi.shift();
      if (anim.slot.length > da) anim.slot.length = da;
      while (anim.slot.length < da) anim.slot.push('-');
      nuovi.forEach(x => anim.slot.push(x));
      anim.conta[pos - 1] = (anim.conta[pos - 1] || 0) + 1;
      n++; quanti--; fatti++; anim.ultimoCodice = codice;
    }
    if (fatti) { barra.value = numero.value = n; disegnaLocale(anim.ultimoCodice); }
    if (anim.passi.length < dimensioneBlocco() / 2) prossimoBlocco();
    if (!anim.passi.length && anim.finito && !anim.carica) { ferma(true); return; }  // arrivato in fondo
    anim.raf = requestAnimationFrame(passo);
  }
  const bottone = (testo, delta) => el('button', {class: 'secondario', type: 'button', testo, onclick: () => vai(n + delta)});
  async function vai(nuovo) {
    if (anim.attiva) ferma(false);  // un comando a mano ferma l'animazione
    n = Math.max(0, totale !== null ? Math.min(totale, nuovo) : nuovo);
    barra.value = numero.value = n;
    motivi.replaceChildren();
    if (n === 0) {  // il passo 0 non e' nel file: e' l'orario vuoto, prima che la ricerca cominci
      richiesta++;
      stato.textContent = `Passo 0${di()}: orario vuoto, la ricerca non è ancora cominciata.`;
      ultimo = null;
      disegnaOrario(griglia, dati, dati.giorni.map(() => dati.ore.map(() => dati.classi.map(() => [-1, -1]))), null, null,
        dati.giorni.map(() => dati.ore.map(() => dati.classi.map(() => 0))));
      return;
    }
    const mia = ++richiesta;
    stato.textContent = 'Carico il passo ' + n + '...';
    const r = await (await fetch(`${url}?n=${n}`)).json();
    if (mia !== richiesta) return;
    if (r.errore) { stato.textContent = r.errore; griglia.replaceChildren(); return; }
    n = r.n; barra.value = numero.value = n;
    if (r.scarto != null) {  // ramo scartato: si evidenzia la scelta che l'ha causato (lo slot prima di quello in esame)
      const k = r.pos - 1, C = dati.classi.length, O = dati.ore.length;
      r.esito = 'scarto'; r.motivoScarto = r.scarto;
      r.blocco = [Math.floor(k / (O * C)), Math.floor(k / C) % O, r.ordine[k % C]];
    }
    scriviStato(r);
    if (r.bloccato) {
      const per = {};
      (r.motivi || []).forEach(([p, m]) => (per[m] = per[m] || []).push(dati.docenti[p].nome));
      const voci = Object.entries(per).map(([m, nomi]) => el('li', {}, [el('strong', {testo: (nomiMotivi[m] || m) + ': '}), el('span', {testo: nomi.join(', ')})]));
      motivi.replaceChildren(el('h3', {testo: 'Perché non si può andare avanti'}),
        voci.length ? el('ul', {}, voci) : el('p', {class: 'tenue piccolo', testo: r.motivi ? 'Nessun docente aveva ancora ore da fare in questa classe.' : 'Motivi non registrati: attiva «Motivi degli scarti» nelle opzioni.'}));
    }
    ultimo = r;
    disegnaOrario(griglia, dati, r.orario, r.pos >= r.slot && r.esito !== 'scarto' ? null : r.blocco, passaggio, r.passaggi);
    teniInVista();
  }
  // la cella evidenziata deve restare visibile: se e' fuori dallo schermo la pagina (e la tabella) scorrono fino a lei
  function teniInVista() {
    const td = griglia.querySelector('td.blocco');
    if (td && td.scrollIntoView) td.scrollIntoView({block: 'nearest', inline: 'nearest'});
  }
  // Mentre si trascina lo slider l'orario segue in tempo reale (evento input, non solo al rilascio). Una richiesta alla
  // volta: se nel frattempo la barra si e' mossa, dopo la risposta si carica subito l'ultima posizione raggiunta.
  let occupato = false, atteso = null;
  async function segui(valore) {
    atteso = valore;
    if (occupato) return;
    occupato = true;
    try {
      while (atteso !== null) { const v = atteso; atteso = null; await vai(v); }
    } finally { occupato = false; }
  }
  barra.addEventListener('input', () => { numero.value = barra.value; segui(+barra.value); });
  numero.addEventListener('change', () => { if (+numero.value !== n) vai(+numero.value); });  // lo stesso numero: niente ridisegno (un tocco su una cella non va perso)
  contenitore.replaceChildren(el('div', {style: 'display:flex;gap:8px;flex-wrap:wrap;align-items:center'}, [
    bottone('⏮', -1e12), bottone('−1000', -1000), bottone('−100', -100), bottone('−1', -1), numero,
    bottone('+1', 1), bottone('+100', 100), bottone('+1000', 1000), bottone('⏭', 1e12)]),
    el('div', {style: 'display:flex;gap:10px;flex-wrap:wrap;align-items:center;margin:6px 0'}, [bottonePlay, el('label', {class: 'piccolo tenue', style: 'display:flex;gap:6px;align-items:center'}, [el('span', {testo: 'Velocità'}), sliderVelocita]), etichettaVelocita]),
    el('div', {style: 'display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-top:6px'}, [
      el('button', {class: 'secondario', type: 'button', testo: '◀ blocco precedente', onclick: () => {
        const prima = veri().filter(v => v[0] < n); if (prima.length) vai(prima[prima.length - 1][0]); }}),
      el('button', {class: 'secondario', type: 'button', testo: 'blocco successivo ▶', onclick: () => {
        const dopo = veri().find(v => v[0] > n); if (dopo) vai(dopo[0]); }}), infoVicoli]),
    barra, stato, motivi, griglia);
  function aggiornaVicoli() {
    return fetch(url + '?vicoli=1').then(r => r.json()).then(v => {
      vicoli = v.vicoli || []; vicoliTroncati = !!v.troncato;
      const vuoti = vicoli.filter(v => v[1] === 'vuoto').length, ciechi = vicoli.filter(v => v[1] === 'indietro').length,
        scarti = vicoli.filter(v => v[1] === 'scarto').length, blocchi = ciechi + scarti;
      const più = vicoliTroncati ? '+' : '';
      infoVicoli.textContent = blocchi
        ? `${numeroIt(blocchi)}${più} ${blocchi === 1 && !più ? 'marcia indietro' : 'marce indietro'}: ${numeroIt(ciechi)} vicoli ciechi (nessun docente utilizzabile), ${numeroIt(scarti)} rami scartati` +
          (vuoti ? ` · ${numeroIt(vuoti)} ore buche non contate (nessun docente, lo slot resta vuoto e la ricerca prosegue)` : '') +
          (blocchi < 30 ? ` · passi ${veri().map(v => v[0]).join(', ')}` : '')
        : 'Nessuna marcia indietro tra i passi registrati: la ricerca è sempre andata avanti' + (vuoti ? ` (${numeroIt(vuoti)} ore buche non contate).` : '.');
    }).catch(() => {});
  }
  fetch(url + '?totale=1').then(r => r.json()).then(t => {
    totale = t.totale || 0;
    if (!totale) return;  // nessuna riga P nel file: la sezione resta nascosta
    if (sezione) sezione.style.display = '';
    barra.max = totale;
    vai(0);
    aggiornaVicoli();
  }).catch(() => {});
}

// Spiegazione a parole del perche' un docente non va bene nello slot, con i dettagli (ore, classi, orari).
function spiegaMotivo(dati, a, c, classe, ora) {
  const d = c.dettaglio || {};
  const nomeOra = i => dati.ore[i], nomeClasse = i => dati.classi[i].nome, nomeDoc = i => dati.docenti[i].nome;
  const oggi = d.oggi || [], qui = oggi.filter(x => x[1] === classe).map(x => x[0]);
  const lista = v => v.map(nomeOra).join(', ');
  switch (c.motivo) {
    case 0: return d.occupato_in >= 0 ? `occupato in ${nomeClasse(d.occupato_in)} alle ${nomeOra(ora)}` : 'occupato in un\'altra classe a quest\'ora';
    case 1: return `entra alle ${nomeOra(d.ingresso)}, ora sono le ${nomeOra(ora)}`;
    case 2: return `ha già fatto oggi ${qui.length} ore in questa classe (alle ${lista(qui)}), il massimo è ${a.limiti.ore_in_classe}`;
    case 3: return `oggi ha insegnato in questa classe alle ${lista(qui)} ma non alle ${nomeOra(ora - 1)}: il suo blocco è finito e non può tornare`;
    case 4: { const cl = [...new Set(oggi.map(x => x[1]))];
      return `oggi è già in ${cl.length} classi (${oggi.map(x => nomeClasse(x[1]) + ' alle ' + nomeOra(x[0])).join(', ')}), il massimo è ${a.limiti.classi_al_giorno}`; }
    case 5: { const p = (d.partner || []).map(([q, st, cl]) => nomeDoc(q) + ' (' + (st === 0 ? 'occupato' + (cl >= 0 ? ' in ' + nomeClasse(cl) : '')
        : st === 1 ? 'non ancora entrato' : st === 2 ? 'al massimo di ore in classe oggi' : st === 3 ? 'ore non contigue' : st === 4 ? 'al massimo di classi oggi' : 'libero') + ')');
      return 'ha solo ore in codocenza' + (p.length ? ': con ' + p.join(', ') : '') + ': nessun codocente utilizzabile'; }
    case 6: return `regola del sabato: nell'ora precedente ${(d.ora_precedente || []).map(nomeDoc).join(' e ')} deve continuare`;
    case 7: return 'non ha ore da fare in questa classe';
    default: return a.motivi[c.motivo];
  }
}

// Una frase per ognuno dei sette vincoli che il solver controlla per un docente in uno slot (a.coda[].vincoli, nello
// stesso ordine di diagnosi.c, vincoli_docente): rispettato (ok = 1), non rispettato (0) o non applicabile (-1).
function vincoliDocente(dati, a, c, classe, ora) {
  const d = c.dettaglio || {}, v = c.vincoli || [];
  const nomeOra = i => dati.ore[i], nomeClasse = i => dati.classi[i].nome, nomeDoc = i => dati.docenti[i].nome;
  const oggi = d.oggi || [], qui = oggi.filter(x => x[1] === classe).map(x => x[0]);
  const lista = x => x.map(nomeOra).join(', ');
  const ore = n => n === 1 ? '1 ora' : `${n} ore`;
  const cl = [...new Set(oggi.map(x => x[1]))];
  const L = a.limiti || {};
  const frasi = [];
  frasi.push([v[0],
    v[0] ? `Libero: non è in un'altra classe alle ${nomeOra(ora)}.`
         : `Occupato: alle ${nomeOra(ora)} è già ${d.occupato_in >= 0 ? 'in ' + nomeClasse(d.occupato_in) : 'in un\'altra classe'}.`]);
  frasi.push([v[1],
    v[1] ? (d.ingresso ? `Già entrato: entra alle ${nomeOra(d.ingresso)}, ora sono le ${nomeOra(ora)}.` : 'Già entrato: entra dalla prima ora.')
         : `Non ancora entrato: entra alle ${nomeOra(d.ingresso)}, ora sono le ${nomeOra(ora)}.`]);
  frasi.push([v[2],
    v[2] ? (qui.length ? `Massimo di ore rispettato: oggi ha fatto ${ore(qui.length)} in questa classe (alle ${lista(qui)}), il massimo è ${L.ore_in_classe}.`
                       : `Massimo di ore rispettato: oggi non ha ancora fatto lezione in questa classe (il massimo è ${L.ore_in_classe} ore).`)
         : `Massimo di ore superato: oggi ha già fatto ${ore(qui.length)} in questa classe (alle ${lista(qui)}), il massimo è ${L.ore_in_classe}.`]);
  frasi.push([v[3],
    v[3] ? (qui.length ? `Ore attaccate: oggi ha insegnato qui alle ${lista(qui)}, quindi alle ${nomeOra(ora)} continua il suo blocco.`
                       : 'Ore attaccate: oggi non ha ancora fatto lezione in questa classe, può cominciare ora.')
         : `Ore non attaccate: oggi ha insegnato qui alle ${lista(qui)} ma non alle ${nomeOra(ora - 1)}; il blocco è finito e non può tornare.`]);
  frasi.push([v[4],
    v[4] ? `Classi al giorno rispettate: oggi è in ${cl.length === 1 ? '1 classe' : cl.length + ' classi'}${cl.length ? ' (' + cl.map(nomeClasse).join(', ') + ')' : ''}, il massimo è ${L.classi_al_giorno}.`
         : `Troppe classi oggi: è già in ${cl.length} classi (${oggi.map(x => nomeClasse(x[1]) + ' alle ' + nomeOra(x[0])).join(', ')}), il massimo è ${L.classi_al_giorno}.`]);
  const partner = (d.partner || []).map(([q, st, k]) => nomeDoc(q) + ' (' + (st === 0 ? 'occupato' + (k >= 0 ? ' in ' + nomeClasse(k) : '')
    : st === 1 ? 'non ancora entrato' : st === 2 ? 'al massimo di ore in classe oggi' : st === 3 ? 'ore non attaccate' : st === 4 ? 'al massimo di classi oggi' : 'libero') + ')');
  frasi.push([v[5],
    v[5] ? (d.da_solo ? 'Da solo o in coppia: ha ore da fare anche da solo, quindi può fare lezione da solo.'
                      : `Codocente: ha un codocente utilizzabile (${c.codocente_libero >= 0 ? nomeDoc(c.codocente_libero) : 'sì'}).`)
         : `Nessun codocente: ha solo ore in coppia e nessun codocente è utilizzabile${partner.length ? ' — ' + partner.join(', ') : ''}.`]);
  frasi.push([v[6],
    v[6] === -1 ? 'Regola del sabato: non si applica, perché non è sabato.'
      : v[6] ? 'Regola del sabato: rispettata.'
             : `Regola del sabato: non rispettata, nell'ora precedente ${(d.ora_precedente || []).map(nomeDoc).join(' e ') || 'un altro docente'} deve finire le sue ore in questa classe.`]);
  return frasi.map(([ok, testo]) => ({stato: ok === -1 ? 'na' : ok ? 'si' : 'no', testo}));
}

// Il contenuto del pannello al passaggio del mouse: scelto nello slot, coda in ordine di priorita', esito di ognuno.
let mostraVincoli = true;  // le frasi per vincolo si vedono finche' non si nascondono (vale per tutte le aperture)
function mostraAnalisi(pannello, dati, a, scelto, titolo, bloccato, classe, ora, finestra) {
  if (!a || a.errore) { pannello.textContent = (a && a.errore) || 'Analisi non disponibile.'; return; }
  const [p, q] = scelto;
  const nome = i => dati.docenti[i].nome;
  const testa = el('div', {}, [el('strong', {testo: titolo}),
    el('span', {class: 'tenue', testo: bloccato ? (typeof bloccato === 'string' ? bloccato : ' · qui la ricerca si è bloccata')
      : p < 0 ? ' · slot lasciato vuoto' : ` · scelto: ${nome(p)}${q >= 0 ? ' + ' + nome(q) : ''}`})]);
  // in ordine di priorita' (la coda del solver; a parita' l'ordine e' quello della coda)
  const con = a.coda.filter(c => c.ore_classe > 0).map((c, i) => [c, i]).sort((x, y) => y[0].priorita - x[0].priorita || x[1] - y[1]).map(x => x[0]);
  const senza = a.coda.length - a.coda.filter(c => c.ore_classe > 0).length;
  const massima = Math.max(1, ...con.map(c => c.priorita));
  const corpo = el('tbody');
  const MAX = 14;
  con.slice(0, MAX).forEach(c => {
    const e = c.docente === p || c.docente === q;
    const esito = e ? 'scelto' : c.motivo < 0 ? 'utilizzabile (non scelto in questo ramo)' : spiegaMotivo(dati, a, c, classe, ora);
    const stato = e ? 'scelto' : c.motivo < 0 ? 'utilizzabile' : 'escluso';
    // la barra: lunghezza = priorita' (rispetto alla massima della coda), colore = scelto / utilizzabile / escluso
    const barra = el('div', {class: 'barra-priorita ' + stato, title: `priorità ${c.priorita}`},
      [el('div', {style: `width: ${(100 * c.priorita / massima).toFixed(1)}%`})]);
    const frasi = vincoliDocente(dati, a, c, classe, ora);
    const riga = el('tr', {class: stato === 'escluso' ? '' : stato}, [
      el('td', {testo: nome(c.docente), style: 'white-space: nowrap'}), el('td', {}, [barra]), el('td', {class: 'num', testo: c.priorita}),
      el('td', {class: 'num', title: 'ore ancora da fare / ore totali in questa classe', testo: `${c.ore_classe}/${c.ore_totali_classe}`}),
      el('td', {testo: esito})]);
    corpo.appendChild(riga);
    corpo.appendChild(el('tr', {class: 'riga-vincoli'}, [el('td', {colspan: 5}, [el('ul', {class: 'vincoli'},
      frasi.map(f => el('li', {class: f.stato}, [el('span', {class: 'segno', testo: f.stato === 'si' ? '✔' : f.stato === 'no' ? '✘' : '–'}), el('span', {testo: f.testo})])))])]));
  });
  // interruttore delle frasi per vincolo (resta com'e' alla prossima apertura)
  if (finestra) finestra.classList.toggle('senza-vincoli', !mostraVincoli);
  const interruttore = el('button', {type: 'button', class: 'secondario', testo: mostraVincoli ? 'Nascondi le frasi per vincolo' : 'Mostra le frasi per vincolo',
    onclick: ev => { ev.stopPropagation(); mostraVincoli = !mostraVincoli; if (finestra) finestra.classList.toggle('senza-vincoli', !mostraVincoli);
      interruttore.textContent = mostraVincoli ? 'Nascondi le frasi per vincolo' : 'Mostra le frasi per vincolo'; }});
  // verifiche di potatura, fatte appena la ricerca entra nel nodo: una riga per ognuna. Se una fallisce la ricerca scarta il
  // ramo senza provare i docenti di questo slot (la causa e' la scelta fatta nello slot prima).
  let causa = null, scartato = false;
  if (a.potatura) {
    const pot = a.potatura, C = pot.classe >= 0 ? dati.classi[pot.classe].nome : '';
    const det = {classe: pot.classe, ore_classe: pot.ore_classe};
    const nomeScelta = coppia(dati, pot.scelta[0], pot.scelta[1]);
    const righe = pot.verifiche.map(v => {
      const base = {...v, classe: pot.classe, ore_classe_fatte: pot.ore_classe_fatte, scelta: pot.scelta, slotNome: pot.slotNome};
      if (v.motivo === 0)
        return {v, base, ok: v.ok ? 1 : 0, testo: v.ok ? `Lezioni della classe ${C}: dopo la scelta in ${pot.slotNome} (${nomeScelta}) le lezioni che le restano bastano per gli slot che deve ancora riempire.`
          : `Lezioni della classe ${C}: dopo la scelta in ${pot.slotNome} (${nomeScelta}) le lezioni che le restano NON bastano per gli slot che deve ancora riempire.`};
      return {v, base, ok: !v.controllata ? -1 : v.ok ? 1 : 0, testo: !v.controllata ? 'Ore dei docenti: si controlla solo a inizio ora, e questo slot non è il primo dell\'ora.'
        : v.ok ? 'Ore dei docenti: le ore che mancano a ogni docente stanno ancora nel tempo rimasto della settimana.'
        : 'Ore dei docenti: le ore che mancano a un docente NON stanno più nel tempo rimasto della settimana.'};
    });
    scartato = righe.some(r => r.ok === 0);
    const dettagli = righe.filter(r => r.ok === 0).flatMap(r => r.v.motivo === 0 ? dettaglioScartoLezioni(dati, det, r.base, pot.slotNome)
      : dettaglioScartoDocenti(dati, det, r.base, pot.slotNome));
    causa = el('div', {class: 'causa-scarto' + (scartato ? ' scartato' : '')}, [
      el('p', {}, [el('strong', {testo: scartato ? '✂ Ramo scartato dalla potatura: ' : 'Verifiche di potatura (all\'ingresso in questo slot): '}),
        el('span', {testo: scartato ? 'la ricerca non ha provato nessun docente qui. Non dipende dai docenti di questo slot: la coda sotto è quella che ci sarebbe stata, ma la ricerca non l\'ha mai guardata, perché una verifica è fallita.'
          : 'tutte rispettate, quindi la ricerca prosegue a provare i docenti.'})]),
      el('ul', {class: 'vincoli'}, righe.map(r => el('li', {class: r.ok === -1 ? 'na' : r.ok ? 'si' : 'no'},
        [el('span', {class: 'segno', testo: r.ok === -1 ? '–' : r.ok ? '✔' : '✘'}), el('span', {testo: r.testo})]))),
      ...dettagli]);
  }
  pannello.replaceChildren(testa, ...(causa ? [causa] : []),
    el('ul', {class: 'vincoli'}, [el('li', {class: a.lezione_ammessa ? 'si' : 'no'}, [el('span', {class: 'segno', testo: a.lezione_ammessa ? '✔' : '✘'}),
      el('span', {testo: a.lezione_ammessa ? 'La classe può ancora fare lezione in quest\'ora.' : 'La classe non può più fare lezione in quest\'ora: lo slot può solo restare vuoto.'})])]),
    el('div', {class: 'barra-comandi'}, [interruttore]),
    el('p', {class: 'tenue piccolo', testo: (scartato ? 'Coda che ci sarebbe stata in questo slot, mai provata. ' : '') + 'Coda dei docenti per priorità (ore ancora da sistemare nella settimana); a parità di priorità l\'ordine dipende dalla ricerca. «Ore in classe» = ancora da fare / totali.'}),
    el('table', {class: 'dati'}, [el('thead', {}, [el('tr', {}, [el('th', {testo: 'Docente'}), el('th', {testo: 'Priorità'}), el('th', {class: 'num', testo: ''}),
      el('th', {class: 'num', testo: 'Ore in classe'}), el('th', {testo: 'Esito'})])]), corpo]),
    el('p', {class: 'tenue piccolo', testo: [con.length > MAX ? `altri ${con.length - MAX} docenti con ore da fare in coda` : '', senza ? `${senza} docenti senza ore da fare in questa classe` : ''].filter(Boolean).join(' · ')}));
}

// Cursore sotto ogni tabella larga (contenitori .scorre): sempre visibile, fa scorrere la tabella in orizzontale e
// resta attaccato al fondo dello schermo finche' la tabella e' visibile (anche con tabelle lunghe, come l'orario per
// docente). Serve perche' su macOS la barra di scorrimento compare solo mentre si scorre, e con le tabelle lunghe sta
// in fondo, lontana. Tabella e cursore stanno in un contenitore comune, cosi' il cursore resta attaccato solo alla sua.
function aggiornaCursori() {
  document.querySelectorAll('.scorre').forEach(box => {
    if (box.closest('.pannello-slot')) return;
    const eccesso = box.scrollWidth - box.clientWidth;
    let cursore = box._cursore;
    if (!cursore) {
      if (eccesso <= 1) return;
      cursore = el('input', {type: 'range', min: 0, value: 0, step: 1, class: 'cursore-tabella', 'aria-label': 'Scorri la tabella in orizzontale',
        title: 'Scorri la tabella in orizzontale'});
      cursore.addEventListener('input', () => { box.scrollLeft = +cursore.value; });
      box.addEventListener('scroll', () => { cursore.value = box.scrollLeft; });
      const insieme = el('div', {class: 'con-cursore'});
      box.parentNode.insertBefore(insieme, box);
      insieme.append(box, cursore);
      box._cursore = cursore;
    }
    cursore.max = Math.max(0, eccesso);
    cursore.value = box.scrollLeft;
    cursore.hidden = eccesso <= 1;
  });
}

// Le tabelle si disegnano e ridisegnano dopo il caricamento (orario, mappa in tempo reale, marce indietro): si
// ricontrolla poco dopo ogni cambiamento della pagina (raggruppando quelli vicini) e a ogni ridimensionamento.
// setTimeout e non requestAnimationFrame: quest'ultimo e' sospeso finche' la scheda non e' in primo piano.
(() => {
  let atteso = false;
  const programma = () => {
    if (atteso) return;
    atteso = true;
    setTimeout(() => { atteso = false; aggiornaCursori(); }, 50);
  };
  new MutationObserver(programma).observe(document.documentElement, {childList: true, subtree: true});
  window.addEventListener('resize', programma);
  document.addEventListener('DOMContentLoaded', programma);
  programma();
})();
