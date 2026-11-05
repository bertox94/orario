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
// "Ven 12 C06"
function testoPos(d, pos) { const s = slotDi(d, pos); return `${d.giorni[s.giorno]} ${d.ore[s.ora]} ${nomeClasse(d, s.classe)}`; }

// ---- la griglia dell'orario ----

// opz.fino: le posizioni da fino in poi non sono ancora decise; opz.blocco: lo slot del backtrack; opz.cambiate (Set),
// opz.risolta, opz.tornata, opz.ritorno, opz.attuale: celle da evidenziare (vedi stile.css)
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
      if (pos === opz.ritorno) classi.push('ritorno');
      if (pos === opz.attuale) classi.push('attuale');
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
      return `ore fatte in questa classe: ${i.ore_svolte} su ${i.ore_da_fare}, ne restano ${i.ore_da_fare - i.ore_svolte}`;
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
    el('td', {class: 'nome', title: nome}, descrizioneControllo(nome, d) +
      (nome === 'deve_ancora_svolgere_ore_in_questa_classe' ? ' (ore che restano su quelle da fare)' : '')),
    ...docenti.map(x => {
      const k = x.controlli.findIndex(c => c.nome === nome);
      if (k < 0) return el('td', {class: 'tenue', testo: '–'});
      const c = x.controlli[k];
      const valutato = x.fermato_a < 0 || k <= x.fermato_a;
      const ingressi = descriviIngressi(c.nome, c.ingressi, d);
      // le ore che restano in questa classe: in ogni casella "restano X su Y"
      const ore = nome === 'deve_ancora_svolgere_ore_in_questa_classe' && c.ingressi.ore_da_fare !== undefined
        ? el('span', {class: 'piccolo', testo: `${c.ingressi.ore_da_fare - c.ingressi.ore_svolte} su ${c.ingressi.ore_da_fare}`})
        : null;
      return el('td', {class: (!valutato ? 'non-valutato' : c.uscita ? 'si' : 'no') + (k === x.fermato_a ? ' fermo' : ''),
          title: ingressi + (valutato ? '' : ' (la ricerca non lo valuta: si è già fermata)')},
        k === x.fermato_a ? [el('strong', {testo: '✗ '}), ore || el('span', {class: 'piccolo', testo: ingressi})]
                          : ore ? [c.uscita ? '✓ ' : '✗ ', ore] : (c.uscita ? '✓' : '✗'));
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

// Cambia il contenuto senza che la pagina si sposti: il cursore delle tabelle larghe si aggiunge subito (non dopo,
// facendo cambiare l'altezza) e la posizione verticale resta quella di prima.
function sostituisci(contenitore, ...figli) {
  const alto = window.scrollY;
  contenitore.replaceChildren(...figli);
  aggiornaCursori();
  if (window.scrollY !== alto) window.scrollTo(window.scrollX, alto);
}

// Mette nel contenitore la didascalia e la griglia dell'orario senza spostare niente: lo scorrimento orizzontale della
// griglia resta quello di prima e la pagina resta dov'e'. La griglia scorre fino alla cella sel (la casella del
// problema) la prima volta e quando si e' appena scelto un gruppo (centraProssima).
let centraProssima = false;
function mettiGriglia(contenitore, didascalia, griglia, sel) {
  const centra = centraProssima;
  centraProssima = false;
  // se c'e' gia' una griglia della stessa forma si cambiano solo le righe: il riquadro che scorre resta lo stesso,
  // cosi' si puo' scorrere (anche col dito) mentre il play va avanti, senza che lo scorrimento si interrompa
  const vecchiaTab = contenitore.querySelector('table.orario'), nuovaTab = griglia.querySelector('table.orario');
  const vecchiaP = contenitore.querySelector('p.didascalia');
  didascalia.title = didascalia.textContent;  // se non sta in due righe, intera passandoci sopra
  if (vecchiaTab && nuovaTab && vecchiaP && vecchiaTab.style.width === nuovaTab.style.width &&
      vecchiaTab.tBodies[0].rows.length === nuovaTab.tBodies[0].rows.length) {
    vecchiaTab.replaceChild(nuovaTab.tBodies[0], vecchiaTab.tBodies[0]);
    vecchiaP.replaceWith(didascalia);
    if (centra) centraCella(contenitore.querySelector('.scorre'), vecchiaTab.querySelector(sel));
    return;
  }
  const vecchia = contenitore.querySelector('.scorre');
  const sinistra = vecchia ? vecchia.scrollLeft : null;
  sostituisci(contenitore, didascalia, griglia);
  const box = griglia.querySelector('.scorre');
  if (box && sinistra !== null && !centra) box.scrollLeft = sinistra;
  else centraCella(box, griglia.querySelector(sel));
}
function centraCella(box, td) {
  if (box && td) box.scrollLeft = Math.max(0, td.offsetLeft - box.clientWidth / 2);
}

// Un backtrack: la griglia "l'orario fino a qui" va in sopra, i controlli in contenitore.
// filtro: 'tutti', 'con-ore' (nasconde chi ha esaurito le ore in questa classe), 'esauriti' (nasconde chi ne ha ancora).
// prima: il backtrack precedente sul cursore (se c'e'): in verde le celle cambiate da allora, e bordata la prima
// (la cella fin dove la ricerca e' tornata indietro per cambiare una scelta).
// passo (passo passo): {didascalia, bordata, ritorno}: la didascalia al posto di quella solita e, se ci sono, la cella
// bordata (quella messa nello stesso record del backtrack) e quella in cui e' finito l'ultimo backtracking.
function disegnaBacktrack(contenitore, d, b, filtro, sopra, prima, passo) {
  const s = slotDi(d, b.pos);
  const parti = [];
  // il titolo e la frase vanno sopra il cursore, tra l'elenco dei gruppi e il cursore
  sostituisci(document.getElementById('intestazione-backtrack'),
    el('h3', {}, `Backtrack n. ${numeroIt(b.n)}: ${testoPos(d, b.pos)}`),
    el('p', {class: 'tenue piccolo'}, [
      `Al nodo ${numeroIt(b.nodo)} della ricerca la classe ${nomeClasse(d, s.classe)} non ha potuto avere nessun ` +
      `docente in quest'ora. `,
      b.buca_dopo ? "Siamo oltre le ore obbligatorie: dopo la ricerca prova l'ora buca, quindi non è un vicolo cieco."
                  : 'Siamo nelle ore obbligatorie: la ricerca torna indietro e cambia una scelta precedente.']));
  if (b.incoerente) parti.push(el('div', {class: 'avviso errore'},
    'Attenzione: secondo la traccia una combinazione passerebbe tutti i controlli, ma la ricerca non l\'ha messa. ' +
    'La traccia non rispecchia la ricerca: va segnalato.'));
  const griglia = el('div');
  const cambiate = new Set();
  if (prima) for (let p = 0; p < b.pos; p++)
    if (p >= prima.orario.length || p >= prima.pos || JSON.stringify(b.orario[p]) !== JSON.stringify(prima.orario[p]))
      cambiate.add(p);
  const tornata = passo ? passo.bordata : cambiate.size ? Math.min(...cambiate) : undefined;
  disegnaOrario(griglia, d, b.orario, {fino: b.pos, blocco: b.pos, cambiate, tornata,
                                        ritorno: passo ? passo.ritorno : undefined});
  const testoCambi = !prima ? '' : !cambiate.size ? ` Nessuna cella cambiata dal backtrack precedente (n. ${numeroIt(prima.n)}).`
    : ` In verde ${cambiate.size === 1 ? 'la cella cambiata' : `le ${numeroIt(cambiate.size)} celle cambiate`} dal ` +
      `backtrack precedente (n. ${numeroIt(prima.n)}); bordata la prima, ${testoPos(d, tornata)}: la ricerca è tornata ` +
      `fin lì e ha cambiato scelta.`;
  mettiGriglia(sopra, el('p', {class: 'tenue piccolo didascalia'}, passo ? passo.didascalia
    : `Al backtrack n. ${numeroIt(b.n)}: ${testoPos(d, b.pos)} (in rosso).` + testoCambi), griglia, 'td.blocco');
  const mostrati = b.docenti.filter(x => filtro === 'con-ore' ? !haEsauritoOre(x)
                                       : filtro === 'esauriti' ? haEsauritoOre(x) : true);
  if (b.docenti.length) parti.push(el('h4', {testo: 'I docenti della classe, nell\'ordine in cui la ricerca li prova'}));
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
  // la classe (lo slot), dopo i docenti
  parti.push(el('h4', {testo: 'Lo slot'}));
  parti.push(tabellaControlli([b.slot], b.slot.uscita ? -1 : 0, d));
  sostituisci(contenitore, ...parti);
}

// ---- i dati del test: classi, docenti e codocenze ----
// Per ogni classe le ore da fare e quante ne restano buche; per ogni docente le classi in cui insegna (con le ore, da
// solo e in codocenza con chi) e un grafo delle codocenze: docenti su un cerchio, un filo per coppia, spessore = ore
// insieme. Viene dalle cattedre del record "dati" della traccia ({docente, classe, ore, da_solo, codocenze: [{con,
// ore}]}, ogni codocenza scritta da tutte e due le parti). I dati non cambiano: si disegna una volta sola.
function disegnaDatiCaso(contenitore, d) {
  if (!contenitore || contenitore.dataset.fatto || !d || !d.cattedre) return;
  contenitore.dataset.fatto = '1';
  const nome = i => nomeDoc(d, i);
  const perDocente = d.docenti.map(() => ({tot: 0, classi: []}));
  const coppie = new Map();  // "a+b" (a < b) -> ore insieme, in tutte le classi
  const lezioni = d.classi.map(() => 0);  // ore di lezione di ogni classe (una codocenza e' un'ora sola)
  for (const x of d.cattedre) {
    perDocente[x.docente].tot += x.ore;
    perDocente[x.docente].classi.push(x);
    lezioni[x.classe] += x.da_solo;
    for (const k of x.codocenze) {
      lezioni[x.classe] += k.ore / 2;  // la stessa codocenza c'e' anche nella cattedra dell'altro
      if (x.docente < k.con) coppie.set(x.docente + '+' + k.con, (coppie.get(x.docente + '+' + k.con) || 0) + k.ore);
    }
  }

  // le classi: ore da fare, slot della settimana e ore buche (slot che resteranno vuoti)
  const slot = d.giorni.length * d.ore.length;
  let oreTot = 0, lezTot = 0;
  const riga = (celle, stile) => el('tr', {style: stile}, celle.map(([v, num, titolo]) =>
    el('td', {class: num ? 'num' : null, title: titolo, testo: v})));
  const righeClassi = d.classi.map((cl, c) => {
    oreTot += cl.ore; lezTot += lezioni[c];
    return riga([[cl.nome], [cl.ore, 1], [lezioni[c], 1], [slot, 1], [slot - cl.ore, 1]]);
  });
  righeClassi.push(riga([['Totale'], [oreTot, 1], [lezTot, 1], [slot * d.classi.length, 1],
    [slot * d.classi.length - oreTot, 1]], 'font-weight: 600'));
  const th = (testo, titolo, num) => el('th', {class: num ? 'num' : null, title: titolo, testo});
  const tabellaClassi = el('div', {class: 'scorre alta', style: 'flex: 1 1 320px'}, el('table', {class: 'dati'}, [
    el('thead', {}, el('tr', {}, [th('Classe'), th('Ore', 'ore di lezione che la classe deve fare', 1),
      th('Lezioni', 'somma delle ore delle sue lezioni (una codocenza conta un\'ora)', 1),
      th('Slot', 'slot della settimana (giorni × ore)', 1),
      th('Buche', 'slot che resteranno vuoti (solo dopo le ore obbligatorie)', 1)])),
    el('tbody', {}, righeClassi)]));

  // i docenti, in ordine alfabetico: una riga per ogni classe in cui insegnano (nome e ore totali nella prima)
  const righeDocenti = [];
  perDocente.map((s, i) => [s, i]).filter(([s]) => s.tot)
    .sort((x, y) => nome(x[1]).localeCompare(nome(y[1]), 'it')).forEach(([s, i]) => {
      s.classi.sort((x, y) => x.classe - y.classe).forEach((x, k) => {
        const tr = el('tr', {class: k ? 'seguito' : null});
        if (!k) tr.append(el('td', {style: 'white-space: nowrap', rowspan: s.classi.length, testo: nome(i)}),
          el('td', {class: 'num', rowspan: s.classi.length, title: 'ore totali, in tutte le classi', testo: s.tot}));
        tr.append(el('td', {testo: nomeClasse(d, x.classe)}), el('td', {class: 'num', testo: x.ore}),
          el('td', {class: 'num', testo: x.da_solo || '—'}),
          el('td', {testo: x.codocenze.map(k => `${nome(k.con)} ×${k.ore}`).join(' · ') || '—'}));
        righeDocenti.push(tr);
      });
    });
  const tabellaDocenti = el('div', {class: 'scorre alta', style: 'flex: 2 1 460px'}, el('table', {class: 'dati'}, [
    el('thead', {}, el('tr', {}, [th('Docente'), th('Ore', 'in tutte le classi', 1), th('Classe'),
      th('Ore', 'ore in questa classe (da solo e in codocenza)', 1), th('Da solo', 'ore da solo in questa classe', 1),
      th('In codocenza con', 'con chi insegna in questa classe, e per quante ore')])),
    el('tbody', {}, righeDocenti)]));

  // il grafo delle codocenze: solo i docenti che ne hanno
  const coinvolti = [...new Set([...coppie.keys()].flatMap(k => k.split('+').map(Number)))].sort((a, b) => a - b);
  let grafo = null;
  if (coinvolti.length) {
    const L = 420, R = L / 2 - 58, cx = L / 2, cy = L / 2, ns = 'http://www.w3.org/2000/svg';
    const punto = i => {
      const a = 2 * Math.PI * coinvolti.indexOf(i) / coinvolti.length - Math.PI / 2;
      return [cx + R * Math.cos(a), cy + R * Math.sin(a), a];
    };
    const svg = document.createElementNS(ns, 'svg');
    svg.setAttribute('viewBox', `0 0 ${L} ${L}`);
    svg.setAttribute('class', 'grafo-codocenze');
    const figlio = (tag, attributi, testo) => {
      const e = document.createElementNS(ns, tag);
      for (const [k, v] of Object.entries(attributi)) e.setAttribute(k, v);
      if (testo !== undefined) e.textContent = testo;
      svg.appendChild(e);
      return e;
    };
    for (const [k, ore] of coppie) {
      const [a, b] = k.split('+').map(Number), [x1, y1] = punto(a), [x2, y2] = punto(b);
      const linea = figlio('line', {x1, y1, x2, y2, class: 'filo', 'stroke-width': Math.min(1 + ore, 7)});
      const titolo = document.createElementNS(ns, 'title');
      titolo.textContent = `${nome(a)} + ${nome(b)}: ${ore} ore insieme`;
      linea.appendChild(titolo);
    }
    for (const i of coinvolti) {
      const [x, y, a] = punto(i);
      figlio('circle', {cx: x, cy: y, r: 4, class: 'nodo'});
      figlio('text', {x: x + 14 * Math.cos(a), y: y + 14 * Math.sin(a) + 4, 'font-size': 12,
        'text-anchor': Math.cos(a) > 0.3 ? 'start' : Math.cos(a) < -0.3 ? 'end' : 'middle'}, nome(i));
    }
    const senza = perDocente.filter((s, i) => s.tot && !coinvolti.includes(i)).length;
    grafo = el('div', {style: 'flex: 1 1 300px; min-width: 280px'}, [svg, el('p', {class: 'tenue piccolo',
      testo: 'Grafo delle codocenze: un filo per coppia, più spesso = più ore insieme (passaci sopra per i numeri).' +
        (senza === 1 ? ' Un docente senza codocenze non è nel grafo.' :
         senza ? ` ${senza} docenti senza codocenze non sono nel grafo.` : '')})]);
  }
  contenitore.replaceChildren(el('div', {class: 'dati-caso'}, [tabellaClassi, tabellaDocenti, ...(grafo ? [grafo] : [])]));
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
  const intestazione = $('intestazione-backtrack');  // titolo e frase del backtrack (vuota se non e' un backtrack)
  const sceltaFiltro = $('filtro-docenti');
  let aperto = null;        // il backtrack disegnato, per ridisegnarlo quando cambia il filtro
  let apertoPrima = null;   // il backtrack precedente sul cursore (per le celle cambiate)
  let apertoPasso = null;   // nel passo passo: cosa ha messo la ricerca nello stesso nodo del backtrack
  try { const f = localStorage.getItem('orario.filtroDocenti'); if (f) sceltaFiltro.value = f; } catch (e) {}
  sceltaFiltro.addEventListener('change', () => {
    try { localStorage.setItem('orario.filtroDocenti', sceltaFiltro.value); } catch (e) {}
    if (aperto) disegnaBacktrack(dettaglio, d, aperto, sceltaFiltro.value, raggiunto, apertoPrima, apertoPasso);
  });

  async function caricaDati() {
    if (d) return d;
    const r = await fetch(cfg.urlDati);
    const x = await r.json();
    if (x && x.classi) d = x;
    return d;
  }

  const testoSlot = pos => testoPos(d, pos);

  // la riga sopra il cursore: celle [N., Nodo, Slot, Poi, testo]
  const td = (testo, num) => el('td', {class: num ? 'num' : null, testo});
  const rigaAttuale = (celle, classe) =>
    $('riga-attuale').replaceChildren(el('tr', {class: 'backtrack scelto' + (classe ? ' ' + classe : '')}, celle));

  // le celle di un backtrack (nella riga sopra il cursore)
  function celleBacktrack(b) {
    const motivi = Object.entries(b.riassunto).sort((a, b) => b[1] - a[1])
      .map(([k, v]) => el('span', {class: 'motivo', title: k}, `${v} × ${motivoBreve(k)}`));
    return [td(numeroIt(b.n), true), td(numeroIt(b.nodo), true), td(testoSlot(b.pos)),
            td(b.buca_dopo ? 'segue ora buca' : 'torna indietro'), el('td', {}, motivi)];
  }

  // ---- i gruppi di backtrack ----
  // Modi di raggrupparli, scelti dal menu sopra l'elenco:
  // - annidati: un gruppo prende i backtrack successivi nella stessa cella finche' la ricerca non fa un backtrack piu'
  //   avanti (ha superato la cella: il gruppo e' risolto). Un backtrack in una cella precedente (la ricerca torna
  //   indietro per sbloccare la cella e si blocca anche li') apre un gruppo a se', dentro quello di prima;
  // - per cella: tutti i backtrack nella stessa cella stanno insieme; il gruppo e' risolto se dopo il suo ultimo
  //   backtrack la ricerca fa un backtrack piu' avanti della cella;
  // - consecutivi: un gruppo parte dalla cella in cui la ricerca si blocca e prende tutti i backtrack successivi,
  //   anche nelle celle precedenti, finche' la ricerca non supera la cella di partenza;
  // - passo passo e passo passo per cella: i gruppi consecutivi, con il cursore che scorre la ricerca invece dei
  //   backtrack (vedi piu' sotto).
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
  MODI.passi = {...MODI.consecutivi, conCelle: false, passi: true,
    spiega: "Passo passo: i gruppi sono quelli consecutivi; scegliendone uno, il cursore scorre ogni nodo della " +
      "ricerca, dal primo backtrack del gruppo fino a quando la ricerca supera la cella di partenza (o fin dove ci " +
      "sono nodi registrati)."};
  MODI.celle = {...MODI.passi, perCella: true,
    spiega: "Passo passo per cella: i gruppi sono quelli consecutivi; scegliendone uno, ogni passo del cursore è una " +
      "cosa sola che fa la ricerca: un nodo decide una cella e ci mette una coppia (con il numero del tentativo), " +
      "oppure in una cella nessun docente passa i controlli (il backtrack). Il gruppo parte da prima del suo primo " +
      "backtrack: dall'ultima volta che la ricerca è entrata nella cella più indietro in cui il gruppo torna."};
  const sceltaModo = $('modo-gruppi');
  try { const m = localStorage.getItem('modo-gruppi'); if (MODI[m]) sceltaModo.value = m; } catch (e) { /* niente */ }
  let modo = MODI[sceltaModo.value] || MODI.annidati;
  let statoModo = modo.stato();
  const assegna = (b, toccati) => modo.assegna(statoModo, b, toccati);
  const SPIEGA_CURSORE = $('spiega-cursore').textContent;
  function mostraModo() {
    $('spiega-gruppi').textContent = modo.spiega;
    if (modo.perCella) {
      $('spiega-cursore').textContent = "Passo passo per cella: il cursore va dal nodo «Da» al nodo «a». Nell'orario " +
        "raggiunto, in azzurro la cella di questo passo (in rosso se nessun docente ci sta: il backtrack); bordata in " +
        "verde la cella in cui è finito l'ultimo backtracking, cioè dove la ricerca è tornata a cambiare scelta. Le " +
        "tacche sotto il cursore sono i backtrack e, in verde, il passo in cui la ricerca supera la cella di partenza.";
      $('tabella-gruppi').classList.toggle('con-celle', false);
      return;
    }
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
    rigaAttuale([td('✓', true), td('–', true), td(cella), td('risolta'), td('caricamento…')], 'risoluzione');
    const ultimoBt = await caricaRecord(g.ultimo);
    const dopo = g.risoltoDa !== null ? (await caricaRecord(g.risoltoDa)).orario : cfg.orario;
    if (!inRisoluzione || gruppoScelto !== g) return;
    const fino = g.inizio + 1;
    const cambiate = new Set();
    for (let p = 0; p < fino; p++)
      if (p >= ultimoBt.orario.length || JSON.stringify(dopo[p]) !== JSON.stringify(ultimoBt.orario[p])) cambiate.add(p);
    const buca = !dopo[g.inizio] || (dopo[g.inizio][0] < 0 && dopo[g.inizio][1] < 0);
    const messo = buca ? "un'ora buca" : coppia(d, dopo[g.inizio]);
    rigaAttuale([td('✓', true), td('–', true), td(cella), td('risolta'),
      td(`${cella}: ${messo} · ${numeroIt(cambiate.size)} cell${cambiate.size === 1 ? 'a cambiata' : 'e cambiate'} ` +
         `dall'ultimo backtrack del gruppo (n. ${numeroIt(g.ultimo)})`)], 'risoluzione');
    const griglia = el('div');
    disegnaOrario(griglia, d, dopo, {fino, cambiate, risolta: g.inizio});
    mettiGriglia(raggiunto, el('p', {class: 'tenue piccolo didascalia'},
      `Come si risolve il gruppo ${numeroIt(g.k + 1)}: la ricerca supera ${cella} ` +
      (buca ? "con un'ora buca. " : `mettendo ${messo}. `) +
      `In verde le celle cambiate rispetto all'ultimo backtrack del gruppo (n. ${numeroIt(g.ultimo)}).`), griglia,
      'td.risolta');
    aperto = null;
    intestazione.replaceChildren();
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
    const tr = el('tr', {class: 'backtrack' + (g === gruppoScelto ? ' scelto' : ''),
                         onclick: () => { centraProssima = true; scegliGruppo(g); }}, [
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
    const colori = coloriCelle(g);
    g.sotto = !modo.conCelle || g.celle.size < 2 ? [] : celleOrdinate(g).map(([p, c]) => el('tr', {class: 'sottoriga',
        onclick: ev => { ev.stopPropagation(); centraProssima = true; scegliGruppo(g, c.primo); }}, [
      el('td'),
      el('td', {}, [el('span', {class: 'pallino c' + colori.get(p)}), testoSlot(p) +
        (p === g.inizio ? ' (partenza)' : '')]),
      el('td', {class: 'num', testo: `dal ${numeroIt(c.primo)}`}),
      el('td', {class: 'num', testo: numeroIt(c.quanti)}),
      el('td'), el('td', {class: 'col-celle'}), el('td'),
    ]));
    if (tr.parentNode) tr.after(...g.sotto);
    return tr;
  }

  // le celle di un gruppo, dalla cella di partenza all'indietro, e il loro colore (0 = partenza, poi 1-4 a giro)
  const celleOrdinate = g => [...g.celle.entries()].sort((a, b) => b[0] - a[0]);
  const coloriCelle = g => new Map(celleOrdinate(g).map(([p], k) => [p, k === 0 ? 0 : 1 + (k - 1) % 4]));

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

  // Le tacche sotto il cursore, tutte di un colore per cella; quelle troppo vicine si fondono.
  // - gruppi consecutivi: i backtrack del gruppo in celle diverse da quella di partenza (la ricerca e' tornata
  //   indietro fin li'), del colore della loro cella;
  // - passo passo: i backtrack (grigi nella cella di partenza, colorati nelle altre) e, in verde, la risoluzione.
  const tacche = $('tacche-cursore');
  let ultimeTacche = '';
  function disegnaTacche() {
    const g = gruppoScelto, min = +cursore.min, max = +cursore.max;
    const conTacche = modo.passi ? P && !P.vuoto : g && modo.conCelle && g.celle.size > 1;
    const chiave = conTacche ? `${sceltaModo.value}:${g.k}:${g.membri.length}:${lista[0]}:${min}:${max}` : '';
    if (chiave === ultimeTacche) return;
    ultimeTacche = chiave;
    const parti = [], visti = new Set();
    const tacca = (i, colore, titolo) => {
      if (i < min || i > max || max <= min) return;
      const f = (i - min) / (max - min), k = Math.round(f * 1000) + colore;
      if (visti.has(k)) return;
      visti.add(k);
      parti.push(el('span', {class: 'tacca ' + colore, style: `left: calc(8px + (100% - 16px) * ${f})`, title: titolo}));
    };
    if (chiave && modo.passi) {
      for (const [nodo, n] of P.bt) {
        const x = riassunti.get(n);
        tacca(passoDelNodo(nodo), x.pos === g.inizio ? 'c0' : 'c1', `backtrack ${numeroIt(n)}: ${testoSlot(x.pos)}`);
      }
      if (P.ris >= 0) tacca(P.primo[P.ris], 'cris', 'la ricerca supera la cella');
    } else if (chiave) {
      const colori = coloriCelle(g);
      lista.forEach((n, i) => {
        const p = riassunti.get(n).pos;
        if (p !== g.inizio) tacca(i, 'c' + colori.get(p), `${numeroIt(n)}: ${testoSlot(p)}`);
      });
    }
    tacche.replaceChildren(...parti);
  }
  function mostraRigaAttuale(n) {
    const b = riassunti.get(n);
    if (b) rigaAttuale(celleBacktrack(b));
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
  // −100, −10, +10, +100 (e ← → nel passo passo): si sposta di tante posizioni del cursore
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
  // «Tutto il gruppo»: il cursore va da un capo all'altro del gruppo (backtrack, o nodi nel passo passo)
  $('cursore-tutti').addEventListener('click', () => {
    if (!gruppoScelto || (modo.passi && (!P || P.vuoto))) return;
    [cursoreDa.value, cursoreA.value] = modo.passi ? [P.da, P.da + P.L - 1] : [gruppoScelto.primo, gruppoScelto.ultimo];
    aggiornaCursore();
  });

  // ---- passo passo: il cursore scorre la ricerca ----
  // La traccia ha un record per nodo (una chiamata print_orario(pos)): [pos, p1, p2] = la ricerca entra nella cella pos
  // dopo che la chiamata della cella pos - 1 ci ha messo la coppia p1, p2. Le celle da pos in poi non sono decise.
  // Partendo da un orario noto e applicando i record uno dopo l'altro si ricostruisce l'orario a ogni passo; ogni OGNI
  // record se ne tiene una copia, per saltare in fretta. Due modi:
  // - passo passo: un passo per record, dal primo backtrack del gruppo; ai backtrack anche la cella messa nello stesso
  //   record;
  // - passo passo per cella: un passo per ogni cosa che fa la ricerca, una cella alla volta: «il nodo N decide la
  //   cella X e ci mette ...» (con il numero del tentativo) oppure «la cella X non ha nessun docente» (il backtrack).
  //   Il gruppo parte dall'ultima volta, prima del suo primo backtrack, in cui la ricerca e' entrata nella cella piu'
  //   indietro in cui il gruppo torna: cosi' si vede anche il primo tentativo.
  // P: {g, da (il nodo del record 0), L (quanti record), pos, p1, p2, ris (record che supera la cella, o -1), punti,
  //     bt (nodo -> backtrack), S (quanti passi), er (passo -> record), et (passo -> 0 messa, 1 backtrack),
  //     dec (passo -> record del nodo che decide, -1 se e' prima), tent (passo -> numero del tentativo),
  //     rit (passo -> cella in cui e' finito l'ultimo backtracking, -1 se non ce n'e' ancora uno),
  //     primo (record -> il suo primo passo), lo, hi (il tratto del cursore, in passi)}
  const OGNI = 1000;
  let P = null, iPasso = 0, caricaId = 0;
  const lunghezzaOrario = () => d.classi.length * d.ore.length * d.giorni.length;

  // i record dei nodi da da ad a, fin dove sono registrati senza buchi
  async function leggiNodi(da, a) {
    const r = await (await fetch(`${cfg.urlNodi}?da=${da}&a=${a}`)).json();
    let L = Math.max(0, Math.min(a, r.ultimo === null ? da - 1 : r.ultimo) - da + 1);
    const pos = new Int32Array(L).fill(-1), p1 = new Int32Array(L), p2 = new Int32Array(L);
    for (const [pda, passi] of r.pezzi) for (let j = 0; j < passi.length; j++) {
      const i = pda + j - da;
      if (i >= 0 && i < L) { pos[i] = passi[j][0]; p1[i] = passi[j][1]; p2[i] = passi[j][2]; }
    }
    let k = 0;
    while (k < L && pos[k] >= 0) k++;
    return {pos, p1, p2, L: k};
  }

  async function apriGruppoPassi(g, nBt) {
    const id = ++caricaId;
    $('posizione').textContent = 'caricamento dei passi…';
    const base = await caricaRecord(g.primo);
    let da = g.nodoPrimo;
    const fineRis = g.risoltoDa !== null ? riassunti.get(g.risoltoDa).nodo : null;
    let {pos, p1, p2, L} = await leggiNodi(da, fineRis !== null ? fineRis : da + 2000000);
    if (id !== caricaId) return;
    // la risoluzione: dopo l'ultimo backtrack del gruppo, il primo nodo oltre la cella di partenza
    let ris = -1;
    for (let i = g.nodoUltimo - da + 1; i < L; i++) if (pos[i] > g.inizio) { ris = i; break; }
    if (ris >= 0) L = ris + 1;
    // per cella: si parte da prima, dall'ultimo ingresso nella cella piu' indietro in cui il gruppo torna
    let celleDa = Infinity;  // da questa cella in poi l'orario del primo backtrack non vale piu'
    if (L && modo.perCella) {
      for (let i = 0; i < L; i++) if (pos[i] > 0) celleDa = Math.min(celleDa, pos[i] - 1);
      const daPrima = Math.max(1, da - 200000);
      const prima = await leggiNodi(daPrima, da - 1);
      if (id !== caricaId) return;
      let m = -1;
      if (prima.L === da - daPrima) for (let j = prima.L - 1; j >= 0; j--) if (prima.pos[j] === celleDa) { m = j; break; }
      if (m >= 0) {
        const unisci = (x, y) => { const z = new Int32Array(prima.L - m + L); z.set(x.subarray(m, prima.L)); z.set(y.subarray(0, L), prima.L - m); return z; };
        [pos, p1, p2] = [unisci(prima.pos, pos), unisci(prima.p1, p1), unisci(prima.p2, p2)];
        if (ris >= 0) ris += prima.L - m;
        L += prima.L - m;
        da = daPrima + m;
      } else celleDa = Infinity;
    }
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
      intestazione.replaceChildren();
      return;
    }
    // le copie dell'orario ogni OGNI record (dall'orario del primo backtrack: valgono le celle prima di celleDa)
    const N = lunghezzaOrario();
    const st = new Int32Array(2 * N).fill(-1);
    (base.orario || []).forEach((c, q) => { if (c && q < celleDa) { st[2 * q] = c[0]; st[2 * q + 1] = c[1]; } });
    const punti = [];
    for (let i = 0; i < L; i++) {
      if (pos[i] > 0) { st[2 * (pos[i] - 1)] = p1[i]; st[2 * (pos[i] - 1) + 1] = p2[i]; }
      if (i % OGNI === 0) punti.push(st.slice());
    }
    const bt = new Map();
    for (const n of g.membri) { const x = riassunti.get(n); if (x.nodo >= da && x.nodo < da + L) bt.set(x.nodo, n); }
    // i passi
    const er = [], et = [], dec = [], tent = [];
    const chiamata = new Map();  // cella -> record dell'ultima chiamata che la decide (la pila della ricerca)
    const tentativi = new Map(); // record di una chiamata -> quante coppie ha messo finora
    for (let i = 0; i < L; i++) {
      if (!modo.perCella) { er.push(i); et.push(bt.has(da + i) ? 1 : 0); dec.push(-1); tent.push(0); continue; }
      const c = pos[i] - 1;
      if (i > 0 && c >= 0) {  // il record 0 e' l'ingresso nella prima cella: la sua coppia viene da prima
        const q = chiamata.has(c) ? chiamata.get(c) : -1, t = (tentativi.get(q) || 0) + 1;
        tentativi.set(q, t);
        er.push(i); et.push(0); dec.push(q); tent.push(t);
      }
      chiamata.set(pos[i], i);
      if (bt.has(da + i)) { er.push(i); et.push(1); dec.push(i); tent.push(0); }
    }
    const S = er.length, primo = new Int32Array(L + 1).fill(S);
    // per cella: a ogni passo, la cella in cui e' finito l'ultimo backtracking (la prima messa dopo un backtrack)
    const rit = new Int32Array(S).fill(-1);
    for (let k = 0, ultimo = -1, dopoBt = false; k < S; k++) {
      if (et[k] === 1) dopoBt = true;
      else if (dopoBt) { ultimo = pos[er[k]] - 1; dopoBt = false; }
      rit[k] = ultimo;
    }
    for (let k = S - 1; k >= 0; k--) primo[er[k]] = k;
    for (let i = L - 1; i >= 0; i--) if (primo[i] > primo[i + 1]) primo[i] = primo[i + 1];
    P = {g, da, L, pos, p1, p2, ris, punti, bt, S, er, et, dec, tent, rit, primo, lo: 0, hi: S - 1};
    cursoreDa.value = da;
    cursoreA.value = da + L - 1;
    iPasso = nBt !== undefined && riassunti.has(nBt) ? passoDelNodo(riassunti.get(nBt).nodo) : 0;
    aggiornaCursorePassi();
    mostraPasso(iPasso);
  }

  // il passo di un nodo: il suo backtrack, se lo e', se no il suo primo passo
  function passoDelNodo(nodo) {
    const i = Math.min(P.L - 1, Math.max(0, nodo - P.da));
    let k = Math.min(P.primo[i], P.S - 1);
    if (P.bt.has(P.da + i)) while (k < P.S - 1 && P.et[k] !== 1 && P.er[k] === i) k++;
    return k;
  }

  // al record i: cosa c'e' nella cella pos - 1, e se ci si e' arrivati tornando indietro
  const messaAl = i => P.pos[i] <= 0 ? '' : P.p1[i] < 0 && P.p2[i] < 0 ? "un'ora buca" : coppia(d, [P.p1[i], P.p2[i]]);
  const indietroAl = i => i > 0 && P.pos[i - 1] >= P.pos[i];
  const testoPasso = i => `Al nodo ${numeroIt(P.da + i)} la ricerca ${indietroAl(i) ? 'è tornata indietro e ' : ''}` +
    `mette ${messaAl(i)} in ${testoSlot(P.pos[i] - 1)} (bordata)`;
  // per cella: il nodo che decide al passo k, e il tentativo
  const nodoCheDecide = k => P.dec[k] >= 0 ? `il nodo ${numeroIt(P.da + P.dec[k])}` : 'un nodo precedente';
  const tentativo = k => P.tent[k] === 1 ? '1° tentativo' : `${P.tent[k]}° tentativo`;

  // l'orario al record i (le celle da pos[i] in poi non sono decise)
  function orarioAlPasso(i) {
    const st = P.punti[Math.floor(i / OGNI)].slice();
    for (let j = Math.floor(i / OGNI) * OGNI + 1; j <= i; j++)
      if (P.pos[j] > 0) { st[2 * (P.pos[j] - 1)] = P.p1[j]; st[2 * (P.pos[j] - 1) + 1] = P.p2[j]; }
    const fino = P.pos[i];
    return Array.from({length: fino}, (_, q) => [st[2 * q], st[2 * q + 1]]);
  }

  function aggiornaCursorePassi() {
    if (!P || P.vuoto) { cursore.min = 0; cursore.max = 0; cursore.value = 0; disegnaTacche(); return; }
    let da = parseInt(cursoreDa.value, 10), a = parseInt(cursoreA.value, 10);
    if (!(da >= P.da)) da = P.da;
    if (!(a <= P.da + P.L - 1)) a = P.da + P.L - 1;
    if (a < da) a = da;
    P.lo = Math.min(P.primo[da - P.da], P.S - 1);
    P.hi = Math.max(P.lo, P.primo[a - P.da + 1] - 1);
    cursore.min = P.lo;
    cursore.max = P.hi;
    cursore.value = iPasso;
    disegnaTacche();
  }

  function vaiANodo(nodo) {
    if (!P || P.vuoto) return;
    if (nodo < P.da || nodo >= P.da + P.L) return;
    const k = passoDelNodo(nodo);
    if (k < P.lo || k > P.hi) { [cursoreDa.value, cursoreA.value] = [P.da, P.da + P.L - 1]; aggiornaCursorePassi(); }
    mostraPasso(k);
  }

  // la riga sopra il cursore per il passo k
  function rigaPasso(k) {
    if (!P || P.vuoto) return;
    const i = P.er[k], nodo = P.da + i, pos = P.pos[i], risolve = i === P.ris;
    if (P.et[k] === 1) return mostraRigaAttuale(P.bt.get(nodo));
    const poi = pos < lunghezzaOrario() ? testoSlot(pos) : null;
    if (modo.perCella) return rigaAttuale([td(risolve ? '✓' : '–', true),
      td(P.dec[k] >= 0 ? numeroIt(P.da + P.dec[k]) : '?', true), td(testoSlot(pos - 1)), td(tentativo(k)),
      td(`mette ${messaAl(i)}` + (poi ? ` · poi il nodo ${numeroIt(nodo)} decide ${poi}` : ' · orario completo') +
         (risolve ? ' · la ricerca supera la cella di partenza: risolto' : ''))], risolve ? 'risoluzione' : '');
    rigaAttuale([td(risolve ? '✓' : '–', true), td(numeroIt(nodo), true), td(pos > 0 ? testoSlot(pos - 1) : '–'),
      td(indietroAl(i) ? 'torna indietro' : 'avanti'),
      td((pos > 0 ? `mette ${messaAl(i)}` : 'inizio') + (poi ? ` · poi prova ${poi}` : ' · orario completo') +
         (risolve ? ' · la ricerca supera la cella di partenza: risolto' : ''))], risolve ? 'risoluzione' : '');
  }

  async function mostraPasso(k) {
    if (!P || P.vuoto) return;
    iPasso = k;
    inRisoluzione = false;
    cursore.value = k;
    const i = P.er[k], nodo = P.da + i, pos = P.pos[i], g = P.g;
    const nBt = P.et[k] === 1 ? P.bt.get(nodo) : undefined;
    const chi = modo.perCella && nBt === undefined && P.dec[k] >= 0 ? P.da + P.dec[k] : nodo;
    rigaPasso(k);
    $('posizione').textContent = `passo ${numeroIt(k + 1)} di ${numeroIt(P.S)} nel gruppo ${numeroIt(g.k + 1)} · ` +
      `nodo ${numeroIt(chi)}` + (nBt !== undefined ? ` · backtrack ${numeroIt(nBt)}` : '') +
      (i === P.ris ? ' · risolto' : k === P.S - 1 ? ` · ultimo nodo registrato (${comeFinisce(g)[0]})` : '');
    if (nBt !== undefined) {
      scelto = nBt;
      const b = await caricaRecord(nBt);
      if (iPasso !== k || !modo.passi) return;
      aperto = b;
      apertoPrima = null;
      apertoPasso = modo.perCella
        ? {didascalia: `Il nodo ${numeroIt(nodo)} decide ${testoSlot(pos)}: nessun docente passa i controlli, ` +
            `backtrack n. ${numeroIt(nBt)} (in rosso). La ricerca torna indietro.`,
           ritorno: P.rit[k] >= 0 ? P.rit[k] : undefined}
        : pos > 0 ? {didascalia: `${testoPasso(i)}; poi ${testoSlot(pos)} resta senza docente: backtrack n. ` +
            `${numeroIt(nBt)} (in rosso).`, bordata: pos - 1} : null;
      disegnaBacktrack(dettaglio, d, b, sceltaFiltro.value, raggiunto, null, apertoPasso);
      return;
    }
    aperto = null;
    intestazione.replaceChildren();
    const orario = orarioAlPasso(i);
    const griglia = el('div');
    const cella = pos > 0 ? pos - 1 : undefined;
    disegnaOrario(griglia, d, orario, modo.perCella
      ? {fino: pos, attuale: cella, ritorno: P.rit[k] >= 0 ? P.rit[k] : undefined,
         risolta: i === P.ris ? g.inizio : undefined}
      : {fino: pos, tornata: cella, risolta: i === P.ris ? g.inizio : undefined});
    const poi = pos < lunghezzaOrario() ? testoSlot(pos) : null;
    const testo = modo.perCella
      ? `${nodoCheDecide(k).replace(/^./, c => c.toUpperCase())} decide ${testoSlot(pos - 1)} (${tentativo(k)}): ` +
        `mette ${messaAl(i)} (in azzurro)` + (poi ? `; poi il nodo ${numeroIt(nodo)} decide ${poi}.` : ': orario completo.')
      : pos > 0 ? testoPasso(i) + (poi ? `; poi prova ${poi}.` : ': orario completo.')
      : `Al nodo ${numeroIt(nodo)} la ricerca riparte dalla prima cella.`;
    mettiGriglia(raggiunto, el('p', {class: 'tenue piccolo didascalia'}, testo), griglia,
                 modo.perCella ? 'td.attuale' : 'td.tornata');
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
    disegnaDatiCaso($('dati-caso'), d);  // mentre il test gira, i dati arrivano dopo
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
    if (g !== gruppoScelto) { scegliGruppo(g, n); return; }  // un backtrack di un altro gruppo: si passa a quel gruppo
    if (modo.passi) return vaiANodo(riassunti.get(n).nodo);
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
      aperto = b; apertoPrima = prima; apertoPasso = null;
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
  disegnaDatiCaso($('dati-caso'), d);

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
