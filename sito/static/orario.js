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
  function mostraModo() {
    $('spiega-gruppi').textContent = modo.spiega;
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
    raggiunto.replaceChildren(el('p', {class: 'tenue piccolo'},
      `Come si risolve il gruppo ${numeroIt(g.k + 1)}: la ricerca supera ${cella} ` +
      (buca ? "con un'ora buca. " : `mettendo ${messo}. `) +
      `In verde le celle cambiate rispetto all'ultimo backtrack del gruppo (n. ${numeroIt(g.ultimo)}).`), griglia);
    requestAnimationFrame(() => {
      const box = griglia.querySelector('.scorre'), td = griglia.querySelector('td.risolta');
      if (box && td) box.scrollLeft = Math.max(0, td.offsetLeft - box.clientWidth / 2);
    });
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
    cursoreDa.value = g.primo;
    cursoreA.value = g.ultimo;
    apri(n === undefined ? g.primo : n);
  }

  // ---- il cursore: scorre i backtrack del gruppo scelto, da «Da» ad «a», e alla fine la risoluzione ----
  const cursore = $('cursore-backtrack'), cursoreDa = $('cursore-da'), cursoreA = $('cursore-a');
  let lista = [];      // i backtrack del gruppo tra «Da» e «a»
  let conRis = false;  // c'e' la posizione della risoluzione in fondo
  function aggiornaCursore() {
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
    aggiornaCursore();
    if (!inRisoluzione && !lista.includes(scelto)) apri(lista[0]);
  });
  // −10, −1, +1, +10: si sposta di tante posizioni del cursore
  function passo(k) {
    if (scelto === null) return;
    const da = inRisoluzione ? lista.length : lista.indexOf(scelto);
    const i = Math.min(+cursore.max, Math.max(0, da + k));
    if (i !== da) vaiA(i, true);
  }
  document.querySelectorAll('.riga-cursore [data-passo]').forEach(b =>
    b.addEventListener('click', () => passo(+b.dataset.passo)));
  $('cursore-tutti').addEventListener('click', () => {
    if (!gruppoScelto) return;
    cursoreDa.value = gruppoScelto.primo;
    cursoreA.value = gruppoScelto.ultimo;
    aggiornaCursore();
  });

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
    modo = MODI[sceltaModo.value] || MODI.annidati;
    try { localStorage.setItem('modo-gruppi', sceltaModo.value); } catch (e) { /* niente */ }
    mostraModo();
    if (!numeri.length) return;
    const n = scelto;
    gruppi.length = 0;
    gruppoDi.clear();
    statoModo = modo.stato();
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
    if (g !== gruppoScelto) { scegliGruppo(g, n); return; }  // un backtrack di un altro gruppo: si passa a quel gruppo
    inRisoluzione = false;
    scelto = n;
    if (n < +cursoreDa.value || n > +cursoreA.value) { cursoreDa.value = g.primo; cursoreA.value = g.ultimo; }
    aggiornaCursore();
    $('posizione').textContent = `backtrack ${numeroIt(n)} · ${numeroIt(g.membri.indexOf(n) + 1)}° di ` +
      `${numeroIt(g.membri.length)} nel gruppo ${numeroIt(g.k + 1)}` +
      (n === g.ultimo && !risolto(g) ? ` · ultimo passaggio, non risolto (${comeFinisce(g)[0]})` : '');
    mostraRigaAttuale(n);
    const b = await caricaRecord(n);
    if (scelto === n && !inRisoluzione) { aperto = b; disegnaBacktrack(dettaglio, d, b, sceltaFiltro.value, raggiunto); }
  }

  // ← e →: il backtrack precedente e il successivo in ordine di tempo (passando da un gruppo all'altro)
  function sposta(k) {
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
