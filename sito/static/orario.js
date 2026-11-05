// Le pagine del sito: la griglia dell'orario e la pagina di un test con i suoi backtrack.
//
// I dati di un test (il record "dati" della traccia del solver): giorni, ore, classi [{nome, ore}],
// docenti [{nome, ore, ingresso}], cattedre, parametri. Un orario e' un elenco per posizione nella ricerca,
// pos = (giorno * ore + ora) * classi + classe, ogni elemento [docente, codocente] (-1 = nessuno).

function el(tag, attributi, figli) {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attributi || {})) {
    if (v === undefined || v === null || v === false) continue;
    if (k === 'testo') e.textContent = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else e.setAttribute(k, v === true ? '' : v);
  }
  for (const f of [].concat(figli || [])) if (f !== null && f !== undefined && f !== false)
    e.append(f instanceof Node ? f : document.createTextNode(String(f)));
  return e;
}

function numeroIt(v) { return v == null ? '-' : Number(v).toLocaleString('it-IT'); }
const nomeDoc = (d, p) => p >= 0 && d.docenti[p] ? d.docenti[p].nome : '?';
const nomeClasse = (d, c) => c >= 0 && d.classi[c] ? d.classi[c].nome : '?';
const nomeOra = (d, o) => d.ore[o] !== undefined ? d.ore[o] : '?';
const coppia = (d, a) => !a || a[0] < 0 ? 'nessuno' : nomeDoc(d, a[0]) + (a[1] >= 0 ? ' + ' + nomeDoc(d, a[1]) : '');

function slotDi(d, pos) {
  const C = d.classi.length, O = d.ore.length;
  return {giorno: Math.floor(pos / C / O), ora: Math.floor(pos / C) % O, classe: pos % C};
}

// ---- la griglia dell'orario ----

// opz.fino: le posizioni da fino in poi non sono ancora decise; opz.blocco: lo slot del backtrack
function disegnaOrario(contenitore, d, orario, opz) {
  opz = opz || {};
  const C = d.classi.length, O = d.ore.length, G = d.giorni.length;
  const fino = opz.fino === undefined ? orario.length : opz.fino;
  const testa1 = el('tr', {}, [el('th', {class: 'classe', rowspan: 2, testo: 'Classe'})]);
  const testa2 = el('tr');
  for (let g = 0; g < G; g++) {
    testa1.append(el('th', {colspan: O, class: 'inizio-giorno', testo: d.giorni[g]}));
    for (let o = 0; o < O; o++) testa2.append(el('th', {class: o === 0 ? 'inizio-giorno' : null, testo: d.ore[o]}));
  }
  const corpo = el('tbody');
  const bloccoSlot = opz.blocco !== undefined ? slotDi(d, opz.blocco) : null;
  for (let c = 0; c < C; c++) {
    const tr = el('tr', {class: bloccoSlot && bloccoSlot.classe === c ? 'classe-blocco' : null},
      [el('td', {class: 'classe', testo: d.classi[c].nome})]);
    for (let g = 0; g < G; g++) for (let o = 0; o < O; o++) {
      const pos = (g * O + o) * C + c;
      const classi = [o === 0 ? 'inizio-giorno' : ''];
      let figli = [];
      if (pos === opz.blocco) {
        classi.push('blocco');
        figli = ['nessuno'];
      } else if (pos >= fino) {
        classi.push('dopo');
      } else {
        const [p1, p2] = orario[pos] || [-1, -1];
        if (p1 < 0 && p2 < 0) { classi.push('buca'); figli = ['–']; }
        else figli = [nomeDoc(d, p1), p2 >= 0 ? el('span', {class: 'cod', testo: nomeDoc(d, p2)}) : null];
      }
      tr.append(el('td', {class: classi.join(' ').trim() || null,
        title: `${d.giorni[g]} ${d.ore[o]} ${d.classi[c].nome}`}, figli));
    }
    corpo.append(tr);
  }
  contenitore.replaceChildren(el('div', {class: 'scorre'},
    el('table', {class: 'orario'}, [el('thead', {}, [testa1, testa2]), corpo])));
}

// ---- i controlli di main, in parole ----

function descrizioneControllo(nome, d) {
  const p = d.parametri || {};
  return ({
    activities_for_this_class_and_hour: "la classe può fare lezione in quest'ora",
    'prof_è_disponibile': "è libero in quest'ora",
    deve_ancora_svolgere_ore_in_questa_classe: 'ha ancora ore in questa classe',
    prof_fa_al_max_n_ore: `oggi ha meno di ${p.max_ore_prof_in_una_classe || 3} ore in questa classe`,
    prof_fa_ore_contigue: 'le sue ore di oggi in questa classe restano contigue',
    ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno:
      `oggi resta entro ${p.max_classi_diverse_al_giorno || 3} classi diverse`,
    verifica_ora_ingresso: "è già entrato a quest'ora",
    docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe: 'hanno ancora ore insieme in questa classe',
    sabato_esaurisci_priorita_subito_cod: "regola del sabato: chi c'era nell'ora prima e ha ancora ore deve restare (in coppia)",
    sabato_esaurisci_priorita_subito: "regola del sabato: chi c'era nell'ora prima e ha ancora ore deve restare",
    deve_svolgere_ancora_ora_in_questa_classe_senza_codocente: 'ha ancora ore da solo in questa classe',
  })[nome] || nome;
}

// etichetta breve per l'elenco dei backtrack
function motivoBreve(nome) {
  const solo = nome.startsWith('da solo: ');
  const n = solo ? nome.slice(9) : nome;
  const t = ({
    activities_for_this_class_and_hour: 'la classe non può fare lezione',
    'prof_è_disponibile': 'occupato',
    deve_ancora_svolgere_ore_in_questa_classe: 'ore in classe finite',
    prof_fa_al_max_n_ore: 'già al massimo di ore oggi',
    prof_fa_ore_contigue: 'ore non contigue',
    ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno: 'troppe classi oggi',
    verifica_ora_ingresso: 'non ancora entrato',
    deve_svolgere_ancora_ora_in_questa_classe_senza_codocente: 'ore da solo finite',
    sabato_esaurisci_priorita_subito: 'regola del sabato',
  })[n] || n;
  return solo ? 'coppie e da solo: ' + t : t;
}

const siNo = v => v ? 'sì' : 'no';

function descriviIngressi(nome, i, d) {
  switch (nome) {
    case 'activities_for_this_class_and_hour':
      return `ore che restano alla classe: ${i.ore_rimanenti}, slot obbligatori ancora da riempire: ` +
        `${i.slot_obbligatori_mancanti}, ore buche oggi: ${i.ore_buche_oggi}`;
    case 'prof_è_disponibile':
      return `occupato in quest'ora: ${siNo(i.impegnato_in_quest_ora)}` +
        (i.in_classe >= 0 ? ` (in ${nomeClasse(d, i.in_classe)})` : '');
    case 'deve_ancora_svolgere_ore_in_questa_classe':
      return `ore fatte in questa classe: ${i.ore_svolte} su ${i.ore_da_fare}`;
    case 'prof_fa_al_max_n_ore':
      return `ore oggi in questa classe: ${i.ore_oggi_in_classe}, massimo ${i.massimo}`;
    case 'prof_fa_ore_contigue':
      return `ore oggi in questa classe: ${i.ore_oggi_in_classe}; nell'ora prima: ` +
        (i.ora_precedente ? coppia(d, i.ora_precedente) : '(è la prima ora)');
    case 'ogni_prof_fa_al_max_n_classi_diverse_ogni_giorno':
      return `classi di oggi, con questa: ${i.classi_oggi_con_questa.map(c => nomeClasse(d, c)).join(', ')}` +
        ` (${i.classi_oggi_con_questa.length}), massimo ${i.massimo}`;
    case 'verifica_ora_ingresso':
      return `ora ${nomeOra(d, i.ora)}, entra alle ${nomeOra(d, i.ora_di_ingresso)}`;
    case 'docente_e_codocente_hanno_ore_da_fare_insieme_in_questa_classe':
      return `ore insieme fatte: ${i.ore_insieme_svolte} su ${i.ore_insieme_da_fare}`;
    case 'sabato_esaurisci_priorita_subito_cod':
    case 'sabato_esaurisci_priorita_subito':
      if (!i.ultimo_giorno) return "non è l'ultimo giorno: non vale";
      if (!i.ora_precedente) return "è la prima ora: non vale";
      return `nell'ora prima: ${coppia(d, i.ora_precedente)}; ore che restano loro in questa classe: ` +
        i.ore_rimaste.filter(x => x !== null).join(' e ');
    case 'deve_svolgere_ancora_ora_in_questa_classe_senza_codocente':
      return `ore da solo fatte: ${i.ore_da_solo_svolte} su ${i.ore_da_solo}`;
  }
  return Object.entries(i).map(([k, v]) => `${k}: ${JSON.stringify(v)}`).join(', ');
}

// La tabella dei controlli di un docente (o di una coppia, o del ramo da solo), nell'ordine in cui la ricerca li
// valuta: la ricerca si ferma al primo falso (fermato_a); i successivi li calcola solo la traccia, in grigio.
function tabellaControlli(controlli, fermato, d) {
  return el('table', {class: 'controlli'}, el('tbody', {}, controlli.map((c, k) => {
    const valutato = fermato < 0 || k <= fermato;
    const cl = !valutato ? 'non-valutato' : c.uscita ? 'si' : 'no';
    return el('tr', {class: cl, title: valutato ? null :
        'La ricerca si ferma al primo controllo falso e questo non lo valuta: è calcolato solo per mostrarlo.'}, [
      el('td', {class: 'segno', testo: c.uscita ? '✓' : '✗'}),
      el('td', {class: 'nome'}, [descrizioneControllo(c.nome, d), el('br'), el('code', {testo: c.nome})]),
      el('td', {class: 'ingressi', testo: descriviIngressi(c.nome, c.ingressi, d)}),
    ]);
  })));
}

function esitoTesto(fermato, controlli, d) {
  return fermato >= 0 ? `si ferma a «${descrizioneControllo(controlli[fermato].nome, d)}»` : 'passa tutti i controlli';
}

// Ha esaurito le ore in questa classe? Lo dice il controllo deve_ancora_svolgere_ore_in_questa_classe, che la traccia
// calcola per ogni docente (anche quando la ricerca si ferma prima).
function haEsauritoOre(x) {
  const c = x.controlli.find(c => c.nome === 'deve_ancora_svolgere_ore_in_questa_classe');
  return c ? !c.uscita : false;
}

// filtro: 'tutti', 'con-ore' (nasconde chi ha esaurito le ore in questa classe), 'esauriti' (nasconde chi ne ha ancora)
// La griglia "l'orario fino a qui" va in sopra (in cima alla pagina), il resto in contenitore (sotto l'elenco).
function disegnaBacktrack(contenitore, d, b, filtro, sopra) {
  const s = slotDi(d, b.pos);
  const parti = [];
  parti.push(el('h3', {}, `Backtrack n. ${numeroIt(b.n)}: ${d.giorni[s.giorno]} ${d.ore[s.ora]} ${nomeClasse(d, s.classe)}`));
  parti.push(el('p', {class: 'tenue piccolo'}, [
    `Al nodo ${numeroIt(b.nodo)} della ricerca la classe ${nomeClasse(d, s.classe)} non ha potuto avere nessun docente in quest'ora. `,
    b.buca_dopo ? "Siamo oltre le ore obbligatorie: dopo la ricerca prova l'ora buca, quindi non è un vicolo cieco."
                : 'Siamo nelle ore obbligatorie: la ricerca torna indietro e cambia una scelta precedente.']));
  if (b.incoerente) parti.push(el('div', {class: 'avviso errore'},
    'Attenzione: secondo la traccia una combinazione passerebbe tutti i controlli, ma la ricerca non l\'ha messa. ' +
    'La traccia non rispecchia la ricerca: va segnalato.'));
  parti.push(el('h4', {testo: 'Lo slot'}));
  parti.push(tabellaControlli([b.slot], b.slot.uscita ? -1 : 0, d));
  const griglia = el('div');
  disegnaOrario(griglia, d, b.orario, {fino: b.pos, blocco: b.pos});
  sopra.replaceChildren(el('p', {class: 'tenue piccolo'},
    `Al backtrack n. ${numeroIt(b.n)}: ${d.giorni[s.giorno]} ${d.ore[s.ora]} ${nomeClasse(d, s.classe)} (in rosso).`),
    griglia);
  // la griglia scorre in orizzontale fino allo slot del backtrack (che puo' stare a fine settimana)
  requestAnimationFrame(() => {
    const box = griglia.querySelector('.scorre'), td = griglia.querySelector('td.blocco');
    if (box && td) box.scrollLeft = Math.max(0, td.offsetLeft - box.clientWidth / 2);
  });
  const mostrati = b.docenti.filter(x => filtro === 'con-ore' ? !haEsauritoOre(x)
                                       : filtro === 'esauriti' ? haEsauritoOre(x) : true);
  const nascosti = b.docenti.length - mostrati.length;
  if (b.docenti.length) {
    parti.push(el('h4', {testo: 'I docenti della classe, nell\'ordine in cui la ricerca li prova'}));
    parti.push(el('p', {class: 'tenue piccolo'}, 'L\'ordine è quello della coda: prima chi ha più ore da fare (la ' +
      'priorità). ✓ = vero, ✗ = falso; la ricerca si ferma al primo ✗, i controlli in grigio non li valuta.'));
    if (nascosti) parti.push(el('p', {class: 'tenue piccolo'}, `${nascosti} docent${nascosti === 1 ? 'e nascosto' :
      'i nascosti'} (${filtro === 'con-ore' ? 'hanno già fatto tutte le ore' : 'hanno ancora ore'} in questa classe): ` +
      'si cambia con «Docenti da mostrare» qui sopra.'));
  }
  for (const x of mostrati) {
    const blocco = el('div', {class: 'docente'});
    blocco.append(el('h4', {}, `${nomeDoc(d, x.docente)} · priorità ${x.priorita} · ${esitoTesto(x.fermato_a, x.controlli, d)}`));
    blocco.append(tabellaControlli(x.controlli, x.fermato_a, d));
    if (x.fermato_a < 0) {
      if (x.coppie.length) {
        blocco.append(el('h5', {testo: 'In coppia (con i docenti dopo di lui nella coda che hanno ore insieme a lui qui)'}));
        for (const c of x.coppie) {
          blocco.append(el('p', {class: 'piccolo', style: 'margin: 6px 0 2px'},
            `con ${nomeDoc(d, c.codocente)}: ${esitoTesto(c.fermato_a, c.controlli, d)}`));
          blocco.append(tabellaControlli(c.controlli, c.fermato_a, d));
        }
      } else {
        blocco.append(el('h5', {testo: 'In coppia: nessun docente dopo di lui nella coda ha ore insieme a lui in questa classe'}));
      }
      blocco.append(el('h5', {}, `Da solo: ${esitoTesto(x.da_solo.fermato_a, x.da_solo.controlli, d)}`));
      blocco.append(tabellaControlli(x.da_solo.controlli, x.da_solo.fermato_a, d));
    }
    parti.push(blocco);
  }
  if (b.altri_docenti) parti.push(el('p', {class: 'tenue piccolo'},
    `Più ${b.altri_docenti} docenti senza ore in questa classe: la ricerca li scarta al primo o al secondo controllo.`));
  contenitore.replaceChildren(...parti);
}

// ---- la pagina di un test ----

function paginaTest(cfg) {
  const $ = id => document.getElementById(id);
  let d = cfg.dati;
  let ultimo = 0;           // ultimo backtrack nell'elenco
  let scelto = null;        // backtrack aperto
  const righe = new Map();  // n -> riga dell'elenco
  const elenco = $('elenco-backtrack');
  const dettaglio = $('dettaglio-backtrack');
  const raggiunto = $('orario-raggiunto');
  const sceltaFiltro = $('filtro-docenti');
  let aperto = null;        // il backtrack disegnato, per ridisegnarlo quando cambia il filtro
  try { const f = localStorage.getItem('orario.filtroDocenti'); if (f) sceltaFiltro.value = f; } catch (e) {}
  sceltaFiltro.addEventListener('change', () => {
    try { localStorage.setItem('orario.filtroDocenti', sceltaFiltro.value); } catch (e) {}
    if (aperto) disegnaBacktrack(dettaglio, d, aperto, sceltaFiltro.value, raggiunto);
  });

  async function caricaDati() {
    if (d) return d;
    const r = await fetch(cfg.urlDati);
    const x = await r.json();
    if (x && x.classi) d = x;
    return d;
  }

  function rigaBacktrack(b) {
    const s = slotDi(d, b.pos);
    const motivi = Object.entries(b.riassunto).sort((a, b) => b[1] - a[1])
      .map(([k, v]) => el('span', {class: 'motivo', title: k}, `${v} × ${motivoBreve(k)}`));
    const tr = el('tr', {class: 'backtrack', 'data-n': b.n, onclick: () => apri(b.n)}, [
      el('td', {class: 'num', testo: numeroIt(b.n)}),
      el('td', {class: 'num', testo: numeroIt(b.nodo)}),
      el('td', {testo: `${d.giorni[s.giorno]} ${d.ore[s.ora]} ${nomeClasse(d, s.classe)}`}),
      el('td', {testo: b.buca_dopo ? 'segue ora buca' : 'torna indietro'}),
      el('td', {}, motivi),
    ]);
    righe.set(b.n, tr);
    return tr;
  }

  async function aggiungiBacktrack() {
    if (!(await caricaDati())) return;
    const r = await fetch(cfg.urlElenco + '?da=' + ultimo);
    const nuovi = await r.json();
    if (!nuovi.length) return;
    const corpo = elenco.querySelector('tbody');
    const vuoto = corpo.querySelector('.vuoto');
    if (vuoto) vuoto.remove();
    for (const b of nuovi) corpo.append(rigaBacktrack(b));
    ultimo = nuovi[nuovi.length - 1].n;
    $('n-registrati').textContent = numeroIt(righe.size);
    if (scelto === null) apri(nuovi[0].n);
  }

  async function apri(n) {
    if (!righe.has(n)) return;
    if (scelto !== null && righe.has(scelto)) righe.get(scelto).classList.remove('scelto');
    scelto = n;
    righe.get(n).classList.add('scelto');
    righe.get(n).scrollIntoView({block: 'nearest'});
    $('posizione').textContent = `${numeroIt(n)} di ${numeroIt(righe.size)} registrati`;
    const r = await fetch(cfg.urlBacktrack.replace(/\/0$/, '/' + n));
    const b = await r.json();
    if (scelto === n) { aperto = b; disegnaBacktrack(dettaglio, d, b, sceltaFiltro.value, raggiunto); }
  }

  function sposta(passo) {
    const numeri = [...righe.keys()];
    const i = numeri.indexOf(scelto);
    if (i < 0) return;
    const j = Math.min(numeri.length - 1, Math.max(0, i + passo));
    if (j !== i) apri(numeri[j]);
  }
  $('precedente').addEventListener('click', () => sposta(-1));
  $('successivo').addEventListener('click', () => sposta(1));
  document.addEventListener('keydown', ev => {
    if (ev.target.closest('input, select, textarea')) return;
    if (ev.key === 'ArrowLeft') { sposta(-1); ev.preventDefault(); }
    if (ev.key === 'ArrowRight') { sposta(1); ev.preventDefault(); }
  });

  if (cfg.orario && d) disegnaOrario($('orario-trovato'), d, cfg.orario);

  async function segui() {
    try {
      const r = await fetch(cfg.urlStato);
      const s = await r.json();
      const p = s.progresso;
      if (p) $('progresso').textContent = `${numeroIt(p.nodi)} nodi · ${numeroIt(p.backtrack)} backtrack · ` +
        `${numeroIt(p.orari)} orari trovati · slot ${numeroIt(p.pos)} · ${numeroIt(Math.round(p.secondi))} s`;
      await aggiungiBacktrack();
      if (!s.in_corso && s.esito) { location.reload(); return; }
    } catch (e) { /* rete: si riprova */ }
    setTimeout(segui, 1000);
  }
  const ferma = $('ferma');
  if (ferma) ferma.addEventListener('click', async () => {
    ferma.disabled = true;
    await fetch(cfg.urlFerma, {method: 'POST', headers: {'X-Gettone': window.GETTONE}});
  });
  if (cfg.inCorso) segui();
  else aggiungiBacktrack();
}

// ---- tabelle larghe: un cursore in fondo allo schermo per scorrerle ----
// Su macOS la barra di scorrimento compare solo mentre si scorre: con tabelle lunghe sta in fondo, lontana.
function aggiornaCursori() {
  document.querySelectorAll('.scorre').forEach(box => {
    const eccesso = box.scrollWidth - box.clientWidth;
    let cursore = box._cursore;
    if (!cursore) {
      if (eccesso <= 1) return;
      cursore = el('input', {type: 'range', min: 0, value: 0, step: 1, class: 'cursore-tabella',
        'aria-label': 'Scorri la tabella in orizzontale', title: 'Scorri la tabella in orizzontale'});
      cursore.addEventListener('input', () => { box.scrollLeft = +cursore.value; });
      box.addEventListener('scroll', () => { cursore.value = box.scrollLeft; });
      const insieme = el('div', {class: 'con-cursore'});
      const sinistra = box.scrollLeft;
      box.parentNode.insertBefore(insieme, box);
      insieme.append(box, cursore);
      box.scrollLeft = sinistra;
      box._cursore = cursore;
    }
    cursore.max = Math.max(0, eccesso);
    cursore.value = box.scrollLeft;
    cursore.hidden = eccesso <= 1;
  });
}
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
