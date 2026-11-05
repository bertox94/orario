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
      if (opz.cambiate && opz.cambiate.has(pos)) classi.push('cambiata');
      if (pos === opz.risolta) classi.push('risolta');
      if (pos === opz.tornata) classi.push('tornata');
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
      const chi = figli.length && pos !== opz.blocco && pos < fino ? ': ' + coppia(d, orario[pos]) : '';
      tr.append(el('td', {class: classi.join(' ').trim() || null,
        title: `${d.giorni[g]} ${d.ore[o]} ${d.classi[c].nome}${chi}`}, figli));
    }
    corpo.append(tr);
  }
  // larghezze fisse (colonna delle classi + una colonna per ora): passando da un backtrack all'altro la griglia
  // non cambia forma; i nomi troppo lunghi si accorciano con … (il nome intero nel title della cella)
  const LARGH_CLASSE = 64, LARGH_ORA = 86;
  const colonne = el('colgroup', {}, [el('col', {style: `width: ${LARGH_CLASSE}px`})]
    .concat(Array.from({length: G * O}, () => el('col', {style: `width: ${LARGH_ORA}px`}))));
  contenitore.replaceChildren(el('div', {class: 'scorre'},
    el('table', {class: 'orario', style: `width: ${LARGH_CLASSE + G * O * LARGH_ORA}px`},
      [colonne, el('thead', {}, [testa1, testa2]), corpo])));
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

function testoPos(d, pos) { const s = slotDi(d, pos); return `${d.giorni[s.giorno]} ${d.ore[s.ora]} ${nomeClasse(d, s.classe)}`; }

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

// I docenti in orizzontale: una colonna per docente (nell'ordine della coda), una riga per controllo. In ogni casella
// ✓ o ✗; dove la ricerca si ferma, anche gli ingressi del controllo. Gli ingressi di tutte le caselle sono nel title.
function tabellaDocenti(docenti, d) {
  // i controlli nell'ordine della ricerca (uguali per tutti i docenti; per sicurezza si uniscono i nomi)
  const nomi = [];
  for (const x of docenti) for (const c of x.controlli) if (!nomi.includes(c.nome)) nomi.push(c.nome);
  const testa = el('tr', {}, [el('th', {class: 'nome', testo: 'Controllo'}), ...docenti.map(x =>
    el('th', {class: 'doc' + (x.fermato_a < 0 ? ' passa' : '')}, [nomeDoc(d, x.docente), el('br'),
      el('span', {class: 'tenue piccolo', testo: `priorità ${x.priorita}`})]))]);
  const righe = nomi.map(nome => el('tr', {}, [
    el('td', {class: 'nome', title: nome}, descrizioneControllo(nome, d)),
    ...docenti.map(x => {
      const k = x.controlli.findIndex(c => c.nome === nome);
      if (k < 0) return el('td', {class: 'tenue', testo: '–'});
      const c = x.controlli[k];
      const valutato = x.fermato_a < 0 || k <= x.fermato_a;
      const ingressi = descriviIngressi(c.nome, c.ingressi, d);
      return el('td', {class: (!valutato ? 'non-valutato' : c.uscita ? 'si' : 'no') + (k === x.fermato_a ? ' fermo' : ''),
          title: ingressi + (valutato ? '' : ' (la ricerca non lo valuta: si è già fermata)')},
        k === x.fermato_a ? [el('strong', {testo: '✗ '}), el('span', {class: 'piccolo', testo: ingressi})]
                          : (c.uscita ? '✓' : '✗'));
    })]));
  return el('div', {class: 'scorre'}, el('table', {class: 'docenti-orizzontale'},
    [el('thead', {}, testa), el('tbody', {}, righe)]));
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
// Mette nel contenitore la didascalia e la griglia dell'orario senza spostare niente: lo scorrimento orizzontale della
// griglia resta quello di prima e la pagina resta dov'e'. Solo la prima volta la griglia scorre fino alla cella sel.
function mettiGriglia(contenitore, didascalia, griglia, sel) {
  // se c'e' gia' una griglia della stessa forma si cambiano solo le righe: il riquadro che scorre resta lo stesso,
  // cosi' si puo' scorrere (anche col dito) mentre il play va avanti, senza che lo scorrimento si interrompa
  const vecchiaTab = contenitore.querySelector('table.orario'), nuovaTab = griglia.querySelector('table.orario');
  const vecchiaP = contenitore.querySelector('p.didascalia');
  if (vecchiaTab && nuovaTab && vecchiaP && vecchiaTab.style.width === nuovaTab.style.width &&
      vecchiaTab.tBodies[0].rows.length === nuovaTab.tBodies[0].rows.length) {
    vecchiaTab.replaceChild(nuovaTab.tBodies[0], vecchiaTab.tBodies[0]);
    didascalia.title = didascalia.textContent;
    vecchiaP.replaceWith(didascalia);
    return;
  }
  const vecchia = contenitore.querySelector('.scorre');
  const sinistra = vecchia ? vecchia.scrollLeft : null, alto = window.scrollY;
  didascalia.title = didascalia.textContent;  // se non sta in due righe, intera passandoci sopra
  contenitore.replaceChildren(didascalia, griglia);
  aggiornaCursori();  // subito, non dopo: il cursore della tabella non fa saltare l'altezza
  const box = griglia.querySelector('.scorre');
  if (box && sinistra !== null) box.scrollLeft = sinistra;
  else if (box) {
    const td = griglia.querySelector(sel);
    if (td) box.scrollLeft = Math.max(0, td.offsetLeft - box.clientWidth / 2);
  }
  if (window.scrollY !== alto) window.scrollTo(window.scrollX, alto);
}

// prima: il backtrack precedente sul cursore (se c'e'): in verde le celle cambiate da allora, e bordata la prima
// (la cella fin dove la ricerca e' tornata indietro per cambiare una scelta).
function disegnaBacktrack(contenitore, d, b, filtro, sopra, prima) {
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
  const cambiate = new Set();
  if (prima) for (let p = 0; p < b.pos; p++)
    if (p >= prima.orario.length || p >= prima.pos || JSON.stringify(b.orario[p]) !== JSON.stringify(prima.orario[p]))
      cambiate.add(p);
  const tornata = cambiate.size ? Math.min(...cambiate) : undefined;
  disegnaOrario(griglia, d, b.orario, {fino: b.pos, blocco: b.pos, cambiate, tornata});
  const testoCambi = !prima ? '' : !cambiate.size ? ` Nessuna cella cambiata dal backtrack precedente (n. ${numeroIt(prima.n)}).`
    : ` In verde ${cambiate.size === 1 ? 'la cella cambiata' : `le ${numeroIt(cambiate.size)} celle cambiate`} dal ` +
      `backtrack precedente (n. ${numeroIt(prima.n)}); bordata la prima, ${testoPos(d, tornata)}: la ricerca è tornata ` +
      `fin lì e ha cambiato scelta.`;
  mettiGriglia(sopra, el('p', {class: 'tenue piccolo didascalia'},
    `Al backtrack n. ${numeroIt(b.n)}: ${d.giorni[s.giorno]} ${d.ore[s.ora]} ${nomeClasse(d, s.classe)} (in rosso).` +
    testoCambi), griglia, 'td.blocco');
  const mostrati = b.docenti.filter(x => filtro === 'con-ore' ? !haEsauritoOre(x)
                                       : filtro === 'esauriti' ? haEsauritoOre(x) : true);
  const nascosti = b.docenti.length - mostrati.length;
  if (b.docenti.length) {
    parti.push(el('h4', {testo: 'I docenti della classe, nell\'ordine in cui la ricerca li prova'}));
    parti.push(el('p', {class: 'tenue piccolo'}, 'L\'ordine è quello della coda: prima chi ha più ore da fare (la ' +
      'priorità), da sinistra a destra. ✓ = vero, ✗ = falso; la ricerca si ferma al primo ✗ (con gli ingressi), i ' +
      'controlli in grigio non li valuta. Passando sopra una casella si vedono i suoi ingressi.'));
    if (nascosti) parti.push(el('p', {class: 'tenue piccolo'}, `${nascosti} docent${nascosti === 1 ? 'e nascosto' :
      'i nascosti'} (${filtro === 'con-ore' ? 'hanno già fatto tutte le ore' : 'hanno ancora ore'} in questa classe): ` +
      'si cambia con «Docenti da mostrare» qui sopra.'));
  }
  if (mostrati.length) parti.push(tabellaDocenti(mostrati, d));
  // chi passa tutti i controlli: la ricerca prova le coppie e il docente da solo
  for (const x of mostrati.filter(x => x.fermato_a < 0)) {
    const blocco = el('div', {class: 'docente'});
    blocco.append(el('h4', {}, `${nomeDoc(d, x.docente)} · priorità ${x.priorita} · passa tutti i controlli`));
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
    parti.push(blocco);
  }
  if (b.altri_docenti) parti.push(el('p', {class: 'tenue piccolo'},
    `Più ${b.altri_docenti} docenti senza ore in questa classe: la ricerca li scarta al primo o al secondo controllo.`));
  const alto = window.scrollY;
  contenitore.replaceChildren(...parti);
  aggiornaCursori();  // subito: il cursore della tabella dei docenti non fa cambiare l'altezza dopo
  if (window.scrollY !== alto) window.scrollTo(window.scrollX, alto);
}

// ---- la pagina di un test ----

function paginaTest(cfg) {
  const $ = id => document.getElementById(id);
  let d = cfg.dati;
  let ultimo = 0;           // ultimo backtrack nell'elenco
  let scelto = null;        // backtrack aperto
  const elenco = $('elenco-backtrack');
  const dettaglio = $('dettaglio-backtrack');
  const raggiunto = $('orario-raggiunto');
  const sceltaFiltro = $('filtro-docenti');
  let aperto = null;        // il backtrack disegnato, per ridisegnarlo quando cambia il filtro
  let apertoPrima = null;   // il backtrack precedente sul cursore (per le celle cambiate)
  try { const f = localStorage.getItem('orario.filtroDocenti'); if (f) sceltaFiltro.value = f; } catch (e) {}
  sceltaFiltro.addEventListener('change', () => {
    try { localStorage.setItem('orario.filtroDocenti', sceltaFiltro.value); } catch (e) {}
    if (aperto) disegnaBacktrack(dettaglio, d, aperto, sceltaFiltro.value, raggiunto, apertoPrima);
  });

  async function caricaDati() {
    if (d) return d;
    const r = await fetch(cfg.urlDati);
    const x = await r.json();
    if (x && x.classi) d = x;
    return d;
  }

  const testoSlot = pos => { const s = slotDi(d, pos); return `${d.giorni[s.giorno]} ${d.ore[s.ora]} ${nomeClasse(d, s.classe)}`; };

  // le celle di un backtrack (nella riga sopra il cursore)
  function celleBacktrack(b) {
    const motivi = Object.entries(b.riassunto).sort((a, b) => b[1] - a[1])
      .map(([k, v]) => el('span', {class: 'motivo', title: k}, `${v} × ${motivoBreve(k)}`));
    return [
      el('td', {class: 'num', testo: numeroIt(b.n)}),
      el('td', {class: 'num', testo: numeroIt(b.nodo)}),
      el('td', {testo: testoSlot(b.pos)}),
      el('td', {testo: b.buca_dopo ? 'segue ora buca' : 'torna indietro'}),
      el('td', {}, motivi),
    ];
  }

  // ---- i gruppi di backtrack ----
  // Tre modi di raggrupparli, scelti dal menu sopra l'elenco:
  // - annidati: un gruppo prende i backtrack successivi nella stessa cella finche' la ricerca non fa un backtrack piu'
  //   avanti (ha superato la cella: il gruppo e' risolto). Un backtrack in una cella precedente (la ricerca torna
  //   indietro per sbloccare la cella e si blocca anche li') apre un gruppo a se', dentro quello di prima;
  // - per cella: tutti i backtrack nella stessa cella stanno insieme; il gruppo e' risolto se dopo il suo ultimo
  //   backtrack la ricerca fa un backtrack piu' avanti della cella;
  // - consecutivi: un gruppo parte dalla cella in cui la ricerca si blocca e prende tutti i backtrack successivi,
  //   anche nelle celle precedenti, finche' la ricerca non supera la cella di partenza.
  const riassunti = new Map();  // n -> il backtrack in breve
  const numeri = [];            // i numeri dei backtrack registrati, in ordine
  const gruppi = [];            // {k, inizio, membri, celle, primo, ultimo, nodoPrimo, nodoUltimo, livello, risoltoDa, tr}
  const gruppoDi = new Map();   // n -> gruppo
  let gruppoScelto = null;
  let inRisoluzione = false;    // il cursore e' sull'ultima posizione: come si risolve il gruppo

  function nuovoGruppo(b, livello) {
    const g = {k: gruppi.length, inizio: b.pos, membri: [], celle: new Map(), primo: b.n, nodoPrimo: b.nodo, livello,
               risoltoDa: null, tr: null, sotto: []};
    gruppi.push(g);
    return g;
  }
  function aggiungiMembro(g, b) {
    g.membri.push(b.n);
    g.ultimo = b.n;
    g.nodoUltimo = b.nodo;
    // per ogni cella del gruppo: quanti backtrack e il primo
    if (!g.celle.has(b.pos)) g.celle.set(b.pos, {quanti: 0, primo: b.n});
    g.celle.get(b.pos).quanti++;
    gruppoDi.set(b.n, g);
  }

  const MODI = {
    annidati: {
      spiega: "Gruppi annidati: un gruppo sono i backtrack in una stessa cella, finché la ricerca non la supera. Se, " +
        "tornando indietro per sbloccarla, la ricerca si blocca in una cella precedente, quello è un gruppo a sé, " +
        "rientrato sotto il primo.",
      stato: () => ({aperti: []}),  // i gruppi non ancora risolti, dal piu' esterno al piu' interno
      assegna(st, b, toccati) {
        // un backtrack piu' avanti della cella di un gruppo aperto: la ricerca l'ha superata, il gruppo e' risolto
        while (st.aperti.length && st.aperti[st.aperti.length - 1].inizio < b.pos) {
          const g = st.aperti.pop();
          g.risoltoDa = b.n;
          toccati.add(g);
        }
        let g = st.aperti[st.aperti.length - 1];
        if (!g || g.inizio !== b.pos) {
          g = nuovoGruppo(b, st.aperti.length);
          st.aperti.push(g);
        }
        aggiungiMembro(g, b);
        toccati.add(g);
      },
    },
    cella: {
      spiega: "Un gruppo per cella: tutti i backtrack in una stessa cella; il gruppo è risolto quando, dopo il suo " +
        "ultimo backtrack, la ricerca supera la cella.",
      stato: () => ({perCella: new Map(), aperti: new Set()}),
      assegna(st, b, toccati) {
        for (const g of st.aperti) if (g.inizio < b.pos) {
          g.risoltoDa = b.n;
          st.aperti.delete(g);
          toccati.add(g);
        }
        let g = st.perCella.get(b.pos);
        if (!g) { g = nuovoGruppo(b, 0); st.perCella.set(b.pos, g); }
        g.risoltoDa = null;  // un altro backtrack nella cella: si aspetta di nuovo che la ricerca la superi
        st.aperti.add(g);
        aggiungiMembro(g, b);
        toccati.add(g);
      },
    },
    passi: {
      spiega: "Passo passo: i gruppi sono quelli consecutivi; scegliendone uno, il cursore scorre ogni nodo della " +
        "ricerca, dal primo backtrack del gruppo fino a quando la ricerca supera la cella di partenza (o fin dove ci " +
        "sono nodi registrati).",
      passi: true,
      stato: () => ({}),
      assegna(st, b, toccati) { MODI.consecutivi.assegna(st, b, toccati); },
    },
    consecutivi: {
      spiega: "Gruppi consecutivi: un gruppo parte dalla cella in cui la ricerca si blocca e comprende i backtrack " +
        "successivi (anche in celle precedenti, mentre la ricerca torna indietro per sbloccarla) finché la ricerca " +
        "supera quella cella. Sotto ogni gruppo, le celle toccate.",
      conCelle: true,
      stato: () => ({}),
      assegna(st, b, toccati) {
        let g = gruppi[gruppi.length - 1];
        if (!g || b.pos > g.inizio) {
          if (g) { g.risoltoDa = b.n; toccati.add(g); }
          g = nuovoGruppo(b, 0);
        }
        aggiungiMembro(g, b);
        toccati.add(g);
      },
    },
  };
  const sceltaModo = $('modo-gruppi');
  try { const m = localStorage.getItem('modo-gruppi'); if (MODI[m]) sceltaModo.value = m; } catch (e) { /* niente */ }
  let modo = MODI[sceltaModo.value] || MODI.annidati;
  let statoModo = modo.stato();
  const assegna = (b, toccati) => modo.assegna(statoModo, b, toccati);
  const SPIEGA_CURSORE = $('spiega-cursore').textContent;
  function mostraModo() {
    $('spiega-gruppi').textContent = modo.spiega;
    $('spiega-cursore').textContent = modo.passi ? "Passo passo: il cursore va dal nodo «Da» al nodo «a» della " +
      "ricerca; a ogni passo, nell'orario raggiunto, è bordata la cella appena messa. Le tacche sotto il cursore sono " +
      "i backtrack (grigie nella cella di partenza, colorate nelle altre) e, in verde, il nodo in cui la ricerca " +
      "supera la cella di partenza." : SPIEGA_CURSORE;
    $('tabella-gruppi').classList.toggle('con-celle', !!modo.conCelle);
  }
  mostraModo();

  // risolto: la ricerca ha superato la cella (risoltoDa = il primo backtrack piu' avanti) o ha trovato l'orario
  const fineTrovato = () => !cfg.inCorso && cfg.esito === 'trovato' && !!cfg.orario;
  const risolto = g => g.risoltoDa !== null || fineTrovato();

  const record = new Map();  // n -> backtrack completo
  async function caricaRecord(n) {
    if (!record.has(n)) record.set(n, await (await fetch(cfg.urlBacktrack.replace(/\/0$/, '/' + n))).json());
    return record.get(n);
  }

  // L'ultima posizione del cursore: l'orario subito dopo che la ricerca ha superato la cella del gruppo. Viene dal
  // backtrack che ha risolto il gruppo (o dall'orario trovato), fino alla cella; in verde le celle cambiate rispetto
  // all'ultimo backtrack del gruppo.
  async function mostraRisoluzione(g) {
    inRisoluzione = true;
    scelto = g.ultimo;
    aggiornaCursore();
    const cella = testoSlot(g.inizio);
    $('posizione').textContent = `come si risolve il gruppo ${numeroIt(g.k + 1)}`;
    $('riga-attuale').replaceChildren(el('tr', {class: 'backtrack scelto risoluzione'}, [
      el('td', {class: 'num', testo: '✓'}), el('td', {class: 'num', testo: '–'}), el('td', {testo: cella}),
      el('td', {testo: 'risolta'}), el('td', {class: 'tenue', testo: 'caricamento…'})]));
    const ultimoBt = await caricaRecord(g.ultimo);
    const dopo = g.risoltoDa !== null ? (await caricaRecord(g.risoltoDa)).orario : cfg.orario;
    if (!inRisoluzione || gruppoScelto !== g) return;
    const fino = g.inizio + 1;
    const cambiate = new Set();
    for (let p = 0; p < fino; p++)
      if (p >= ultimoBt.orario.length || JSON.stringify(dopo[p]) !== JSON.stringify(ultimoBt.orario[p])) cambiate.add(p);
    const buca = !dopo[g.inizio] || (dopo[g.inizio][0] < 0 && dopo[g.inizio][1] < 0);
    const messo = buca ? "un'ora buca" : coppia(d, dopo[g.inizio]);
    $('riga-attuale').replaceChildren(el('tr', {class: 'backtrack scelto risoluzione'}, [
      el('td', {class: 'num', testo: '✓'}), el('td', {class: 'num', testo: '–'}), el('td', {testo: cella}),
      el('td', {testo: 'risolta'}),
      el('td', {testo: `${cella}: ${messo} · ${numeroIt(cambiate.size)} cell${cambiate.size === 1 ? 'a cambiata' :
        'e cambiate'} dall'ultimo backtrack del gruppo (n. ${numeroIt(g.ultimo)})`})]));
    const griglia = el('div');
    disegnaOrario(griglia, d, dopo, {fino, cambiate, risolta: g.inizio});
    mettiGriglia(raggiunto, el('p', {class: 'tenue piccolo didascalia'},
      `Come si risolve il gruppo ${numeroIt(g.k + 1)}: la ricerca supera ${cella} ` +
      (buca ? "con un'ora buca. " : `mettendo ${messo}. `) +
      `In verde le celle cambiate rispetto all'ultimo backtrack del gruppo (n. ${numeroIt(g.ultimo)}).`), griglia,
      'td.risolta');
    aperto = null;
    dettaglio.replaceChildren(el('p', {class: 'tenue piccolo'}, g.risoltoDa !== null
      ? `L'orario viene dal backtrack n. ${numeroIt(g.risoltoDa)}, il primo dopo che la ricerca ha superato la cella, ` +
        `fino alla cella del gruppo: i controlli dei docenti ci sono solo per i backtrack.`
      : "L'orario è quello trovato, fino alla cella del gruppo: i controlli dei docenti ci sono solo per i backtrack."));
  }

  function comeFinisce(g) {
    if (g.risoltoDa !== null) return ['risolto', 'ok'];
    if (cfg.inCorso) return ['in corso', ''];
    if (fineTrovato()) return ['risolto: orario trovato', 'ok'];
    if (cfg.totaleBacktrack > numeri.length && numeri.length >= cfg.maxBacktrack)
      return [`limite dei backtrack registrati (${numeroIt(cfg.maxBacktrack)})`, 'attenzione'];
    return [cfg.esito || 'non risolto', 'errore'];
  }

  function rigaGruppo(g) {
    const [fine, tipo] = comeFinisce(g);
    const tr = el('tr', {class: 'backtrack' + (g === gruppoScelto ? ' scelto' : ''), onclick: () => scegliGruppo(g)}, [
      el('td', {class: 'num', testo: numeroIt(g.k + 1)}),
      el('td', {style: g.livello ? `padding-left: ${10 + g.livello * 18}px` : null},
        (g.livello ? '↳ ' : '') + testoSlot(g.inizio)),
      el('td', {class: 'num', testo: g.primo === g.ultimo ? numeroIt(g.primo) : `${numeroIt(g.primo)}–${numeroIt(g.ultimo)}`}),
      el('td', {class: 'num', testo: numeroIt(g.membri.length)}),
      el('td', {class: 'num', testo: g.nodoPrimo === g.nodoUltimo ? numeroIt(g.nodoPrimo) :
        `${numeroIt(g.nodoPrimo)}–${numeroIt(g.nodoUltimo)}`}),
      el('td', {class: 'col-celle', testo: modo.conCelle ?
        celleOrdinate(g).map(([p, c]) => `${testoSlot(p)} (${numeroIt(c.quanti)})`).join(', ') : ''}),
      el('td', {}, el('span', {class: 'esito-gruppo ' + tipo, testo: fine})),
    ]);
    if (g.tr) g.tr.replaceWith(tr);
    g.tr = tr;
    // gruppi consecutivi: una sottoriga per ogni cella in cui la ricerca si e' bloccata (se sono piu' d'una); un clic
    // porta il cursore al primo backtrack in quella cella
    for (const x of g.sotto) x.remove();
    g.sotto = !modo.conCelle || g.celle.size < 2 ? [] : celleOrdinate(g).map(([p, c]) => el('tr', {class: 'sottoriga',
        onclick: ev => { ev.stopPropagation(); scegliGruppo(g, c.primo); }}, [
      el('td'),
      el('td', {}, [el('span', {class: 'pallino c' + coloreCella(g, p)}), testoSlot(p) +
        (p === g.inizio ? ' (partenza)' : '')]),
      el('td', {class: 'num', testo: `dal ${numeroIt(c.primo)}`}),
      el('td', {class: 'num', testo: numeroIt(c.quanti)}),
      el('td'), el('td', {class: 'col-celle'}), el('td'),
    ]));
    if (tr.parentNode) tr.after(...g.sotto);
    return tr;
  }

  // le celle di un gruppo, dalla cella di partenza all'indietro; ognuna con un colore (per le tacche sul cursore)
  const COLORI_CELLE = 4;
  const celleOrdinate = g => [...g.celle.entries()].sort((a, b) => b[0] - a[0]);
  const coloreCella = (g, pos) => pos === g.inizio ? 0 :
    1 + (celleOrdinate(g).findIndex(([p]) => p === pos) - 1) % COLORI_CELLE;

  function scegliGruppo(g, n) {
    const prima = gruppoScelto;
    gruppoScelto = g;
    if (prima && prima.tr) prima.tr.classList.remove('scelto');
    g.tr.classList.add('scelto');
    // scorre solo l'elenco, non la pagina
    const rr = g.tr.getBoundingClientRect(), re = elenco.getBoundingClientRect();
    const testa = elenco.querySelector('thead').offsetHeight;
    if (rr.top < re.top + testa) elenco.scrollTop -= re.top + testa - rr.top;
    else if (rr.bottom > re.top + elenco.clientHeight) elenco.scrollTop += rr.bottom - re.top - elenco.clientHeight;
    if (modo.passi) { apriGruppoPassi(g, n); return; }
    cursoreDa.value = g.primo;
    cursoreA.value = g.ultimo;
    apri(n === undefined ? g.primo : n);
  }

  // ---- il cursore: scorre i backtrack del gruppo scelto, da «Da» ad «a», e alla fine la risoluzione ----
  const cursore = $('cursore-backtrack'), cursoreDa = $('cursore-da'), cursoreA = $('cursore-a');
  let lista = [];      // i backtrack del gruppo tra «Da» e «a»
  let conRis = false;  // c'e' la posizione della risoluzione in fondo
  function aggiornaCursore() {
    if (modo.passi) return aggiornaCursorePassi();
    const g = gruppoScelto;
    if (!g) return;
    let da = parseInt(cursoreDa.value, 10), a = parseInt(cursoreA.value, 10);
    if (!(da >= g.primo)) da = g.primo;
    if (!(a <= g.ultimo)) a = g.ultimo;
    lista = g.membri.filter(n => n >= da && n <= a);
    if (!lista.length) lista = g.membri.slice();
    conRis = risolto(g) && lista[lista.length - 1] === g.ultimo;
    cursore.min = 0;
    cursore.max = lista.length - 1 + (conRis ? 1 : 0);
    cursore.value = inRisoluzione ? lista.length : Math.max(0, lista.indexOf(scelto));
    disegnaTacche();
  }

  // Sul cursore (gruppi consecutivi), una tacca per ogni backtrack del gruppo in una cella diversa da quella di
  // partenza (la ricerca e' tornata indietro fin li'), del colore della sua cella. Le tacche vicine si fondono.
  const tacche = $('tacche-cursore');
  let ultimeTacche = '';
  function disegnaTacche() {
    if (modo.passi) return;
    const g = gruppoScelto, max = +cursore.max;
    const chiave = g && modo.conCelle ? `${g.k}:${g.membri.length}:${lista[0]}:${lista.length}:${max}` : '';
    if (chiave === ultimeTacche) return;
    ultimeTacche = chiave;
    const parti = [];
    if (chiave && g.celle.size > 1 && max > 0) {
      const colori = new Map(celleOrdinate(g).map(([p]) => [p, coloreCella(g, p)]));
      const visti = new Set();
      lista.forEach((n, i) => {
        const p = riassunti.get(n).pos;
        if (p === g.inizio) return;
        const f = i / max, k = Math.round(f * 1000) + ':' + colori.get(p);
        if (visti.has(k)) return;
        visti.add(k);
        parti.push(el('span', {class: 'tacca c' + colori.get(p), style: `left: calc(8px + (100% - 16px) * ${f})`,
          title: `${numeroIt(n)}: ${testoSlot(p)}`}));
      });
    }
    tacche.replaceChildren(...parti);
  }
  function mostraRigaAttuale(n) {
    const b = riassunti.get(n);
    if (b) $('riga-attuale').replaceChildren(el('tr', {class: 'backtrack scelto'}, celleBacktrack(b)));
  }
  // vai alla posizione i del cursore (0 = primo backtrack della lista; lista.length = la risoluzione)
  function vaiA(i, subito) {
    if (modo.passi) return subito ? mostraPasso(i) : rigaPasso(i);
    if (conRis && i === lista.length) return subito ? mostraRisoluzione(gruppoScelto) : null;
    const n = lista[i];
    if (n === undefined) return;
    mostraRigaAttuale(n);
    if (subito) apri(n);
  }
  let attesaCursore = null;
  cursore.addEventListener('input', () => {
    const i = +cursore.value;
    vaiA(i, false);  // la riga cambia subito; il dettaglio quando il cursore si ferma un attimo
    clearTimeout(attesaCursore);
    attesaCursore = setTimeout(() => vaiA(i, true), 150);
  });
  for (const x of [cursoreDa, cursoreA]) x.addEventListener('change', () => {
    if (modo.passi) {
      aggiornaCursorePassi();
      if (P && !P.vuoto && (iPasso < P.lo || iPasso > P.hi)) mostraPasso(iPasso < P.lo ? P.lo : P.hi);
      return;
    }
    aggiornaCursore();
    if (!inRisoluzione && !lista.includes(scelto)) apri(lista[0]);
  });
  // −10, −1, +1, +10: si sposta di tante posizioni del cursore
  function passo(k) {
    if (modo.passi) {
      if (!P || P.vuoto) return;
      const i = Math.min(P.hi, Math.max(P.lo, iPasso + k));
      if (i !== iPasso) mostraPasso(i);
      return;
    }
    if (scelto === null) return;
    const da = inRisoluzione ? lista.length : lista.indexOf(scelto);
    const i = Math.min(+cursore.max, Math.max(0, da + k));
    if (i !== da) vaiA(i, true);
  }
  // ▶ Play: il cursore avanza da solo di una posizione alla volta, fino in fondo (o fino a ⏸)
  const play = $('cursore-play'), velocita = $('cursore-velocita');
  let giro = null;
  function fermaPlay() {
    clearTimeout(giro);
    giro = null;
    play.textContent = '▶ Play';
    dettaglio.style.minHeight = '';
  }
  function avanti() {
    if (+cursore.value >= +cursore.max) { fermaPlay(); return; }
    // durante il play la parte sotto l'orario non si accorcia (i controlli dei docenti ci sono solo ai backtrack):
    // cosi' la pagina non salta
    dettaglio.style.minHeight = Math.max(parseFloat(dettaglio.style.minHeight) || 0,
                                         dettaglio.getBoundingClientRect().height) + 'px';
    passo(1);
    giro = setTimeout(avanti, +velocita.value);
  }
  play.addEventListener('click', () => {
    if (giro) { fermaPlay(); return; }
    if (scelto === null && !(modo.passi && P && !P.vuoto)) return;
    play.textContent = '⏸ Pausa';
    if (+cursore.value >= +cursore.max) vaiA(+cursore.min, true);  // dal fondo: riparte dall'inizio
    giro = setTimeout(avanti, +velocita.value);
  });
  // un clic nell'elenco, sul cursore o sui pulsanti ferma il play
  elenco.addEventListener('click', fermaPlay);
  cursore.addEventListener('pointerdown', fermaPlay);
  document.querySelectorAll('.riga-cursore [data-passo]').forEach(b =>
    b.addEventListener('click', () => passo(+b.dataset.passo)));
  $('cursore-tutti').addEventListener('click', () => {
    if (!gruppoScelto) return;
    if (modo.passi) {
      if (!P || P.vuoto) return;
      cursoreDa.value = P.da;
      cursoreA.value = P.da + P.L - 1;
      aggiornaCursorePassi();
      return;
    }
    cursoreDa.value = gruppoScelto.primo;
    cursoreA.value = gruppoScelto.ultimo;
    aggiornaCursore();
  });

  // ---- passo passo: il cursore scorre i nodi della ricerca ----
  // Ogni nodo e' [pos, p1, p2]: la ricerca arriva alla posizione pos dopo aver messo la coppia p1, p2 in pos - 1 (le
  // celle da pos in poi non sono decise). Partendo dall'orario del primo backtrack del gruppo e applicando i nodi uno
  // dopo l'altro si ricostruisce l'orario a ogni passo; ogni OGNI nodi se ne tiene una copia, per saltare in fretta.
  // P: {g, da (il primo nodo), L (quanti nodi), pos, p1, p2, ris (indice del nodo che supera la cella, o -1),
  //     punti (le copie), bt (nodo -> backtrack), lo, hi (il tratto del cursore, indici)}
  const OGNI = 1000;
  let P = null, iPasso = 0, caricaId = 0;
  const lunghezzaOrario = () => d.classi.length * d.ore.length * d.giorni.length;

  async function apriGruppoPassi(g, nBt) {
    const id = ++caricaId;
    $('posizione').textContent = 'caricamento dei passi…';
    const base = await caricaRecord(g.primo);
    const da = g.nodoPrimo;
    const fineRis = g.risoltoDa !== null ? riassunti.get(g.risoltoDa).nodo : null;
    const r = await (await fetch(`${cfg.urlNodi}?da=${da}&a=${fineRis !== null ? fineRis : da + 2000000}`)).json();
    if (id !== caricaId) return;
    let a = fineRis !== null ? fineRis : Math.min(r.ultimo === null ? da - 1 : r.ultimo, da + 2000000);
    let L = Math.max(0, a - da + 1);
    const pos = new Int32Array(L).fill(-1), p1 = new Int32Array(L), p2 = new Int32Array(L);
    for (const [pda, passi] of r.pezzi) for (let j = 0; j < passi.length; j++) {
      const i = pda + j - da;
      if (i >= 0 && i < L) { pos[i] = passi[j][0]; p1[i] = passi[j][1]; p2[i] = passi[j][2]; }
    }
    let k = 0;
    while (k < L && pos[k] >= 0) k++;
    L = k;  // fin dove i nodi sono registrati senza buchi
    // la risoluzione: dopo l'ultimo backtrack del gruppo, il primo nodo oltre la cella di partenza
    let ris = -1;
    for (let i = g.nodoUltimo - da + 1; i < L; i++) if (pos[i] > g.inizio) { ris = i; break; }
    if (ris >= 0) L = ris + 1;
    gruppoScelto = g;
    if (!L) {
      P = {g, vuoto: true};
      aggiornaCursorePassi();
      $('posizione').textContent = '';
      $('riga-attuale').replaceChildren(el('tr', {}, el('td', {colspan: 5, class: 'tenue',
        testo: 'Nessun nodo registrato per questo gruppo.'})));
      raggiunto.replaceChildren(el('p', {class: 'tenue piccolo'}, 'Il passo passo non è disponibile per questo ' +
        'gruppo: il test non ha i nodi della ricerca (lanciato prima della traccia dei nodi, o oltre il limite dei ' +
        'nodi registrati). Si può rilanciare il test, oppure scegliere un altro raggruppamento.'));
      dettaglio.replaceChildren();
      return;
    }
    // le copie dell'orario ogni OGNI nodi
    const N = lunghezzaOrario();
    const st = new Int32Array(2 * N).fill(-1);
    (base.orario || []).forEach((c, q) => { if (c) { st[2 * q] = c[0]; st[2 * q + 1] = c[1]; } });
    const punti = [];
    for (let i = 0; i < L; i++) {
      if (pos[i] > 0) { st[2 * (pos[i] - 1)] = p1[i]; st[2 * (pos[i] - 1) + 1] = p2[i]; }
      if (i % OGNI === 0) punti.push(st.slice());
    }
    const bt = new Map();
    for (const n of g.membri) { const x = riassunti.get(n); if (x.nodo >= da && x.nodo < da + L) bt.set(x.nodo, n); }
    P = {g, da, L, pos, p1, p2, ris, punti, bt, lo: 0, hi: L - 1};
    cursoreDa.value = da;
    cursoreA.value = da + L - 1;
    iPasso = nBt !== undefined && riassunti.has(nBt) ? Math.min(L - 1, Math.max(0, riassunti.get(nBt).nodo - da)) : 0;
    aggiornaCursorePassi();
    mostraPasso(iPasso);
  }

  // l'orario al passo i (le celle da pos[i] in poi non sono decise)
  function orarioAlPasso(i) {
    const st = P.punti[Math.floor(i / OGNI)].slice();
    for (let j = Math.floor(i / OGNI) * OGNI + 1; j <= i; j++)
      if (P.pos[j] > 0) { st[2 * (P.pos[j] - 1)] = P.p1[j]; st[2 * (P.pos[j] - 1) + 1] = P.p2[j]; }
    const fino = P.pos[i];
    return Array.from({length: fino}, (_, q) => [st[2 * q], st[2 * q + 1]]);
  }

  function aggiornaCursorePassi() {
    if (!P || P.vuoto) { cursore.min = 0; cursore.max = 0; cursore.value = 0; tacche.replaceChildren(); return; }
    let da = parseInt(cursoreDa.value, 10), a = parseInt(cursoreA.value, 10);
    if (!(da >= P.da)) da = P.da;
    if (!(a <= P.da + P.L - 1)) a = P.da + P.L - 1;
    if (a < da) a = da;
    P.lo = da - P.da;
    P.hi = a - P.da;
    cursore.min = P.lo;
    cursore.max = P.hi;
    cursore.value = iPasso;
    tacchePassi();
  }

  // le tacche: i backtrack del gruppo (grigi nella cella di partenza, colorati nelle altre) e la risoluzione (verde)
  function tacchePassi() {
    const chiave = `passi:${P.g.k}:${P.lo}:${P.hi}`;
    if (chiave === ultimeTacche) return;
    ultimeTacche = chiave;
    const parti = [], visti = new Set(), largo = Math.max(1, P.hi - P.lo);
    const aggiungi = (i, classe, titolo) => {
      if (i < P.lo || i > P.hi) return;
      const f = (i - P.lo) / largo, k = Math.round(f * 1000) + classe;
      if (visti.has(k)) return;
      visti.add(k);
      parti.push(el('span', {class: 'tacca ' + classe, style: `left: calc(8px + (100% - 16px) * ${f})`, title: titolo}));
    };
    for (const [nodo, n] of P.bt) {
      const x = riassunti.get(n);
      aggiungi(nodo - P.da, x.pos === P.g.inizio ? 'c0' : 'c1', `backtrack ${numeroIt(n)}: ${testoSlot(x.pos)}`);
    }
    if (P.ris >= 0) aggiungi(P.ris, 'cris', 'la ricerca supera la cella');
    tacche.replaceChildren(...parti);
  }

  function vaiANodo(nodo) {
    if (!P || P.vuoto) return;
    const i = nodo - P.da;
    if (i < 0 || i >= P.L) return;
    if (i < P.lo || i > P.hi) { cursoreDa.value = P.da; cursoreA.value = P.da + P.L - 1; aggiornaCursorePassi(); }
    mostraPasso(i);
  }

  // la riga sopra il cursore per il passo i
  function rigaPasso(i) {
    if (!P || P.vuoto) return;
    const nodo = P.da + i, pos = P.pos[i], nBt = P.bt.get(nodo);
    if (nBt !== undefined) return mostraRigaAttuale(nBt);
    const indietro = i > 0 && P.pos[i - 1] >= pos;
    const messa = pos > 0 ? (P.p1[i] < 0 && P.p2[i] < 0 ? "un'ora buca" : coppia(d, [P.p1[i], P.p2[i]])) : '';
    $('riga-attuale').replaceChildren(el('tr', {class: 'backtrack scelto' + (i === P.ris ? ' risoluzione' : '')}, [
      el('td', {class: 'num', testo: i === P.ris ? '✓' : '–'}),
      el('td', {class: 'num', testo: numeroIt(nodo)}),
      el('td', {testo: pos > 0 ? testoSlot(pos - 1) : '–'}),
      el('td', {testo: indietro ? 'torna indietro' : 'avanti'}),
      el('td', {testo: (pos > 0 ? `mette ${messa}` : 'inizio') +
        (pos < lunghezzaOrario() ? ` · poi prova ${testoSlot(pos)}` : ' · orario completo') +
        (i === P.ris ? ' · la ricerca supera la cella di partenza: risolto' : '')}),
    ]));
  }

  async function mostraPasso(i) {
    if (!P || P.vuoto) return;
    iPasso = i;
    inRisoluzione = false;
    cursore.value = i;
    const nodo = P.da + i, pos = P.pos[i], nBt = P.bt.get(nodo), g = P.g;
    rigaPasso(i);
    $('posizione').textContent = `passo ${numeroIt(i + 1)} di ${numeroIt(P.L)} nel gruppo ${numeroIt(g.k + 1)} · ` +
      `nodo ${numeroIt(nodo)}` + (nBt !== undefined ? ` · backtrack ${numeroIt(nBt)}` : '') +
      (i === P.ris ? ' · risolto' : i === P.L - 1 ? ` · ultimo nodo registrato (${comeFinisce(g)[0]})` : '');
    if (nBt !== undefined) {
      scelto = nBt;
      const b = await caricaRecord(nBt);
      if (iPasso !== i || !modo.passi) return;
      aperto = b;
      apertoPrima = null;
      disegnaBacktrack(dettaglio, d, b, sceltaFiltro.value, raggiunto, null);
      return;
    }
    aperto = null;
    const orario = orarioAlPasso(i);
    const indietro = i > 0 && P.pos[i - 1] >= pos;
    const griglia = el('div');
    disegnaOrario(griglia, d, orario, {fino: pos, tornata: pos > 0 ? pos - 1 : undefined,
                                       risolta: i === P.ris ? g.inizio : undefined});
    const messa = pos > 0 ? (P.p1[i] < 0 && P.p2[i] < 0 ? "un'ora buca" : coppia(d, [P.p1[i], P.p2[i]])) : '';
    mettiGriglia(raggiunto, el('p', {class: 'tenue piccolo didascalia'}, pos > 0
      ? `Al nodo ${numeroIt(nodo)} la ricerca ${indietro ? 'è tornata indietro e ' : ''}mette ${messa} in ` +
        `${testoSlot(pos - 1)} (bordata)` + (pos < lunghezzaOrario() ? `; poi prova ${testoSlot(pos)}.` : ': orario completo.')
      : `Al nodo ${numeroIt(nodo)} la ricerca riparte dalla prima cella.`), griglia, 'td.tornata');
    dettaglio.replaceChildren(el('p', {class: 'tenue piccolo'}, 'I controlli dei docenti ci sono solo ai nodi in cui ' +
      'la ricerca si blocca (i backtrack, le tacche sotto il cursore).'));
  }

  // mette i backtrack nei gruppi e aggiorna l'elenco
  function inserisci(nuovi, daCapo) {
    const corpo = elenco.querySelector('tbody');
    const vuoto = corpo.querySelector('.vuoto');
    if (vuoto && nuovi.length) vuoto.remove();
    const toccati = new Set();
    const ultimoScelto = gruppoScelto ? gruppoScelto.ultimo : null;
    for (const b of nuovi) {
      if (!daCapo) { riassunti.set(b.n, b); numeri.push(b.n); }
      assegna(b, toccati);
    }
    const righe = [];
    for (const g of [...toccati].sort((x, y) => x.k - y.k)) {
      const tr = rigaGruppo(g);
      if (!tr.parentNode) righe.push(tr, ...g.sotto);
    }
    corpo.append(...righe);
    if (nuovi.length) ultimo = nuovi[nuovi.length - 1].n;
    $('n-registrati').textContent = numeroIt(numeri.length);
    $('n-gruppi').textContent = numeroIt(gruppi.length);
    // il gruppo scelto e' cresciuto: se il cursore arrivava alla sua fine, ci arriva ancora
    if (gruppoScelto && ultimoScelto !== null && +cursoreA.value === ultimoScelto) cursoreA.value = gruppoScelto.ultimo;
    aggiornaCursore();
    if (scelto === null && gruppi.length) scegliGruppo(gruppi[0]);
  }

  // un altro modo di raggruppare: si rifanno i gruppi con i backtrack gia' arrivati, restando sullo stesso backtrack
  sceltaModo.addEventListener('change', () => {
    fermaPlay();
    modo = MODI[sceltaModo.value] || MODI.annidati;
    try { localStorage.setItem('modo-gruppi', sceltaModo.value); } catch (e) { /* niente */ }
    mostraModo();
    if (!numeri.length) return;
    const n = scelto;
    gruppi.length = 0;
    gruppoDi.clear();
    statoModo = modo.stato();
    P = null;
    caricaId++;
    gruppoScelto = null;
    scelto = null;
    inRisoluzione = false;
    ultimeTacche = null;
    elenco.querySelector('tbody').replaceChildren();
    inserisci(numeri.map(x => riassunti.get(x)), true);
    if (n !== null && gruppoDi.has(n)) scegliGruppo(gruppoDi.get(n), n);
  });

  async function aggiungiBacktrack() {
    if (!(await caricaDati())) return;
    // il sito ne manda al massimo 5000 per volta: si chiede il resto finche' ce n'e'
    for (;;) {
      const r = await fetch(cfg.urlElenco + '?da=' + ultimo);
      const nuovi = await r.json();
      if (!nuovi.length) break;
      inserisci(nuovi);
      if (nuovi.length < 5000) break;
    }
  }

  async function apri(n) {
    if (!riassunti.has(n)) return;
    const g = gruppoDi.get(n);
    if (modo.passi) {
      if (g !== gruppoScelto) { scegliGruppo(g, n); return; }
      return vaiANodo(riassunti.get(n).nodo);
    }
    if (g !== gruppoScelto) { scegliGruppo(g, n); return; }  // un backtrack di un altro gruppo: si passa a quel gruppo
    inRisoluzione = false;
    scelto = n;
    if (n < +cursoreDa.value || n > +cursoreA.value) { cursoreDa.value = g.primo; cursoreA.value = g.ultimo; }
    aggiornaCursore();
    $('posizione').textContent = `backtrack ${numeroIt(n)} · ${numeroIt(g.membri.indexOf(n) + 1)}° di ` +
      `${numeroIt(g.membri.length)} nel gruppo ${numeroIt(g.k + 1)}` +
      (n === g.ultimo && !risolto(g) ? ` · ultimo passaggio, non risolto (${comeFinisce(g)[0]})` : '');
    mostraRigaAttuale(n);
    const i = lista.indexOf(n);
    const [b, prima] = await Promise.all([caricaRecord(n), i > 0 ? caricaRecord(lista[i - 1]) : null]);
    if (scelto === n && !inRisoluzione) {
      aperto = b; apertoPrima = prima;
      disegnaBacktrack(dettaglio, d, b, sceltaFiltro.value, raggiunto, prima);
    }
  }

  // ← e →: il backtrack precedente e il successivo in ordine di tempo (passando da un gruppo all'altro)
  function sposta(k) {
    if (modo.passi) return passo(k);
    const i = numeri.indexOf(scelto);
    if (i < 0) return;
    if (inRisoluzione && k < 0) { apri(scelto); return; }
    const j = Math.min(numeri.length - 1, Math.max(0, i + k));
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
