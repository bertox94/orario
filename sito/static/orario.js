// Componenti comuni: griglia dell'orario e mappa dei blocchi (marce indietro / rami scartati per slot).
// I dati arrivano dal JSON del solver (--json) o dalle righe di progresso (--progresso).

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

// Mappa dei blocchi: valore(g, o, c) per ogni slot; colore proporzionale (in scala logaritmica) al massimo.
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
      if (v > 0) {
        td.classList.add('cliccabile');
        td.addEventListener('click', () => { if (apriBlocco) apriBlocco(g, o, c); });
      }
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

// Dal JSON del solver: blocchi in uno slot = marce indietro + rami scartati dopo lo slot.
function blocchiDaRisultato(diagnosi) {
  const scartati = Object.values(diagnosi.scartati || {});
  return (g, o, c) => diagnosi.marce[g][o][c] + scartati.reduce((s, t) => s + t[g][o][c], 0);
}

// Dalla riga di progresso: tabelle piatte con indice (g * ore + o) * classi + c.
function blocchiDaProgresso(dati, p) {
  const O = dati.ore.length, C = dati.classi.length;
  return (g, o, c) => {
    const i = (g * O + o) * C + c;
    return (p.marce_slot[i] || 0) + (p.scartati_slot[i] || 0);
  };
}

function numeroIt(v) { return v == null ? '-' : Number(v).toLocaleString('it-IT'); }

// Il motivo di un ramo scartato in parole: il solver scrive una frase breve (diagnosi.c, nome_motivo_scarto), qui
// spiegata per intero. La verifica sulle lezioni guarda la classe dello slot appena deciso, quindi la si puo' nominare;
// quella sulle ore dei docenti non dice quale docente. n = quante volte e' successo dopo quello slot.
function spiegaScarto(motivo, classe, n) {
  const volte = n === 1 ? '1 volta' : `${numeroIt(n)} volte`;
  const dopo = 'Con le scelte fatte fino a questo slot, ';
  const indietro = ' La ricerca ha annullato la scelta e ne ha provata un\'altra';
  if (motivo.startsWith('alla classe non bastano')) {
    return `${dopo}la classe ${classe} non può più fare lezione in tutte le ore obbligatorie che le restano: le lezioni ` +
      `che le mancano non bastano a riempirle, perché ogni docente può insegnare solo dalla sua ora di ingresso e solo ` +
      `per un numero massimo di ore al giorno nella stessa classe.${indietro} (${volte}).`;
  }
  if (motivo.startsWith('le ore che mancano a un docente')) {
    return `${dopo}a un docente restano più ore da fare di quante ne possa ancora fare nel resto della settimana ` +
      `(può insegnare solo dalla sua ora di ingresso, e in una classe solo per un massimo di ore al giorno).${indietro} (${volte}).`;
  }
  return `${motivo} (${volte}).`;
}

// Le marce indietro in parole: nello slot nessun docente si poteva mettere (i motivi per docente sono piu' sotto).
function spiegaMarce(n) {
  const volte = n === 1 ? '1 volta' : `${numeroIt(n)} volte`;
  return `In questo slot nessun docente si poteva mettere, quindi la ricerca è tornata indietro (${volte}): i motivi ` +
    `docente per docente sono in «Orario a ogni marcia indietro» (con -b) e nella diagnosi completa.`;
}

// Il dettaglio dei blocchi di uno slot (dettagli_slot del JSON, vedi printer/risultati_json.h), come elementi della
// pagina: per le marce indietro i docenti con ore nella classe e perche' ognuno non poteva; per i rami scartati la
// scelta scartata e i numeri della verifica, lezione per lezione. Le prove fatte prima che il solver scrivesse questi
// dettagli usano spiegaMarce e spiegaScarto (testo generico).
const volte = n => n === 1 ? '1 volta' : `${numeroIt(n)} volte`;
const oreDi = (fatte, tot) => `${fatte} ore su ${tot} (${tot - fatte === 1 ? 'ne manca 1' : `ne mancano ${tot - fatte}`})`;
const nomeDoc = (dati, p) => p >= 0 && dati.docenti[p] ? dati.docenti[p].nome : '?';
const coppia = (dati, p, q) => nomeDoc(dati, p) + (q >= 0 ? ' + ' + nomeDoc(dati, q) : '');

// Perche' un docente non si poteva mettere nello slot (motivo come in diagnosi.c, nome_motivo), in parole.
function perchéDocente(dati, motivo, classeOccupato) {
  return [
    classeOccupato >= 0 ? `è in ${dati.classi[classeOccupato].nome} alla stessa ora` : 'è in un\'altra classe alla stessa ora',
    'non è ancora entrato (ora di ingresso)',
    'ha già fatto oggi il massimo di ore in questa classe',
    'oggi ha già fatto lezione qui, ma non nell\'ora prima: le sue ore in una classe devono essere attaccate',
    'oggi insegna già nel massimo di classi diverse',
    'ha solo ore in coppia e nessun codocente era disponibile',
    'regola del sabato: nell\'ora prima c\'era un altro docente che deve finire le sue ore in questa classe',
  ][motivo] || 'motivo sconosciuto';
}

function dettaglioMarce(dati, det, slotNome) {
  const um = det.ultima_marcia;
  const parti = [el('h4', {testo: `Marce indietro (${volte(det.marce)}): in ${slotNome} nessun docente si poteva mettere`}),
    el('p', {testo: `All'ultima marcia indietro la classe aveva fatto ${oreDi(um.ore_classe_fatte, det.ore_classe)}; ` +
      (um.docenti_con_ore === 1 ? '1 docente aveva ancora ore in questa classe.' : `${um.docenti_con_ore} docenti avevano ancora ore in questa classe.`)})];
  if (um.docenti && um.docenti.length) {
    const corpo = el('tbody', {}, um.docenti.map(([p, m, cl]) => {
      const d = dati.docenti[p];
      const extra = m === 1 && d && d.ingresso ? ` (entra alle ${dati.ore[d.ingresso]})` : '';
      return el('tr', {}, [el('td', {testo: nomeDoc(dati, p), style: 'white-space: nowrap'}), el('td', {testo: perchéDocente(dati, m, cl) + extra})]);
    }));
    parti.push(el('table', {class: 'dati'}, [el('thead', {}, [el('tr', {}, [el('th', {testo: 'Docente'}), el('th', {testo: 'Perché non si poteva mettere'})])]), corpo]));
  } else {
    parti.push(el('p', {class: 'tenue', testo: 'I motivi docente per docente si vedono con «Motivi degli scarti» attivo.'}));
  }
  return parti;
}

function dettaglioScartoLezioni(dati, det, s, slotNome) {
  const classe = dati.classi[det.classe].nome;
  const fineSett = s.fino_ora === dati.ore.length - 1;
  const entro = fineSett ? 'entro la fine della settimana' : `entro le ${dati.ore[s.fino_ora]}`;
  const parti = [el('h4', {testo: `Ramo scartato (${volte(s.volte)}): ${coppia(dati, s.scelta[0], s.scelta[1])} in ${slotNome}`}),
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
  return [el('h4', {testo: `Ramo scartato (${volte(s.volte)}): dopo l'ultima scelta dell'ora, ${coppia(dati, s.scelta[0], s.scelta[1])} in ${slotNome}`}),
    el('p', {testo: `Finita quest'ora, ${d ? d.nome : 'un docente'} aveva ancora ${s.ore_rimaste} ore da fare ${dove}, ma nel tempo rimasto ` +
      `ne poteva fare al massimo ${s.ore_possibili}` + (d && d.ingresso ? ` (entra alle ${dati.ore[d.ingresso]})` : '') +
      (s.classe_docente >= 0 ? ', perché in una classe può fare solo un massimo di ore al giorno, attaccate.' : '.') +
      ` La classe dello slot aveva fatto ${oreDi(s.ore_classe_fatte, det.ore_classe)}. La ricerca ha provato un'altra scelta.`})];
}

// Gli slot dove la ricerca si e' bloccata di piu', dal JSON del solver: una riga compatta per slot (arrivi, marce
// indietro, rami scartati, ore della classe, peso sul totale); cliccandola si apre il dettaglio. Si vedono i primi
// `quanti`, gli altri con "Mostra tutti". apriBlocco(g, o, c) apre il dettaglio di uno slot (dal clic sulla mappa).
let apriBlocco = null;
function elencoBlocchi(contenitore, dati, diagnosi, quanti, dettagli) {
  const scartati = Object.entries(diagnosi.scartati || {});  // [motivo, tabella [g][o][c]]
  const perSlot = {};
  (dettagli || []).forEach(d => { perSlot[`${d.giorno},${d.ora},${d.classe}`] = d; });
  const righe = [];
  let totale = 0;
  dati.giorni.forEach((gn, g) => dati.ore.forEach((on, o) => dati.classi.forEach((cl, c) => {
    const marce = diagnosi.marce[g][o][c];
    const rami = scartati.reduce((s, [, t]) => s + t[g][o][c], 0);
    totale += marce + rami;
    if (marce + rami) righe.push({g, o, c, marce, rami, det: perSlot[`${g},${o},${c}`], nome: `${gn} ${on} ${cl.nome}`});
  })));
  if (!righe.length) {
    contenitore.replaceChildren(el('p', {class: 'tenue', testo: 'Nessun blocco: la ricerca e\' arrivata all\'orario senza mai tornare indietro.'}));
    return;
  }
  righe.sort((a, b) => (b.marce + b.rami) - (a.marce + a.rami));
  const corpo = el('tbody');
  const dettaglio = r => {
    if (!r.det) {  // prova vecchia: testo generico
      return [el('p', {testo: (r.marce ? [spiegaMarce(r.marce)] : []).concat(scartati.filter(([, t]) => t[r.g][r.o][r.c] > 0)
        .map(([m, t]) => spiegaScarto(m, dati.classi[r.c].nome, t[r.g][r.o][r.c]))).join(' ')})];
    }
    const parti = r.det.ultima_marcia ? dettaglioMarce(dati, r.det, r.nome) : [];
    r.det.scarti.forEach(s => parti.push(...(s.da_riempire !== undefined ? dettaglioScartoLezioni(dati, r.det, s, r.nome)
                                                                         : dettaglioScartoDocenti(dati, r.det, s, r.nome))));
    return parti;
  };
  righe.forEach((r, i) => {
    const fatte = r.det ? (r.det.ultima_marcia ? r.det.ultima_marcia.ore_classe_fatte : r.det.scarti[0].ore_classe_fatte) : null;
    const freccia = el('span', {class: 'freccia', testo: '▸'});
    const riga = el('tr', {class: 'blocco'}, [
      el('td', {style: 'white-space: nowrap'}, [freccia, el('span', {testo: ' ' + r.nome})]),
      el('td', {class: 'num', testo: r.det ? numeroIt(r.det.visite) : '-'}),
      el('td', {class: 'num', testo: numeroIt(r.marce)}), el('td', {class: 'num', testo: numeroIt(r.rami)}),
      el('td', {class: 'num', style: 'white-space: nowrap', testo: fatte === null ? '-' : `${fatte} / ${r.det.ore_classe}`}),
      el('td', {class: 'num', testo: (100 * (r.marce + r.rami) / totale).toLocaleString('it-IT', {maximumFractionDigits: 2}) + '%'})]);
    const cella = el('td', {colspan: 6, class: 'contenuto-dettaglio'});
    const sotto = el('tr', {class: 'dettaglio-blocco'}, [cella]);
    sotto.hidden = true;
    r.apri = (forza) => {
      const aperto = forza === undefined ? sotto.hidden : forza;
      if (aperto && !cella.childNodes.length) cella.replaceChildren(...dettaglio(r));
      sotto.hidden = !aperto;
      freccia.textContent = aperto ? '▾' : '▸';
    };
    riga.addEventListener('click', () => r.apri());
    if (i >= quanti) riga.hidden = true;
    r.riga = riga;
    corpo.append(riga, sotto);
  });
  righe[0].apri(true);
  const th = (testo, titolo, num) => el('th', {class: num ? 'num' : '', testo, title: titolo || ''});
  const tabella = el('table', {class: 'dati blocchi'}, [
    el('thead', {}, [el('tr', {}, [th('Slot (clic per il dettaglio)'),
      th('Arrivi', 'Quante volte la ricerca è arrivata a questo slot. A ogni arrivo prova più docenti, quindi i rami scartati possono essere più degli arrivi', true),
      th('Marce indietro', 'Volte in cui nessun docente si poteva mettere in questo slot', true),
      th('Rami scartati', 'Volte in cui, dopo aver deciso questo slot, la verifica ha visto che l\'orario non si poteva più completare', true),
      th('Ore della classe', 'Ore della classe già in orario all\'ultimo blocco, su quelle della settimana', true),
      th('% dei blocchi', 'Marce indietro e rami scartati di questo slot, sul totale', true)])]), corpo]);
  const parti = [tabella];
  if (righe.length > quanti) {
    const tutti = el('button', {class: 'secondario', type: 'button', testo: `Mostra tutti gli slot con blocchi (${righe.length})`});
    tutti.addEventListener('click', () => { righe.forEach(r => { r.riga.hidden = false; }); tutti.remove(); });
    parti.push(tutti);
  }
  contenitore.replaceChildren(...parti);
  apriBlocco = (g, o, c) => {
    const r = righe.find(x => x.g === g && x.o === o && x.c === c);
    if (!r) return;
    r.riga.hidden = false;
    r.apri(true);
    r.riga.scrollIntoView({block: 'center', behavior: 'smooth'});
  };
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
  const nascondi = () => { clearTimeout(timer); pannello.style.display = 'none'; cellaAperta = null; };
  chiudi.addEventListener('click', ev => { ev.stopPropagation(); nascondi(); });
  pannello.addEventListener('click', ev => ev.stopPropagation());
  document.addEventListener('click', () => { if (cellaAperta) nascondi(); });  // tocco fuori: chiude
  const C = dati.classi.length, O = dati.ore.length;
  return fonte => (g, o, c, td, ev) => {
    clearTimeout(timer);
    if (g == null) { nascondi(); return; }
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
      if (window.innerWidth >= 760) {
        const x = td.getBoundingClientRect();
        pannello.style.left = Math.max(8, Math.min(x.right + 8, window.innerWidth - 680)) + 'px';
        pannello.style.top = Math.max(8, Math.min(x.top, window.innerHeight - 440)) + 'px';
      } else {
        pannello.style.left = pannello.style.top = '';
      }
      contenuto.textContent = 'Calcolo della coda...';
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
        } catch (e) { cache[chiave] = {errore: 'Analisi non disponibile.'}; }
      }
      if (pannello.style.display === 'none' || cellaAperta !== td) return;
      mostraAnalisi(contenuto, dati, cache[chiave], f.orario[g][o][c], `${dati.giorni[g]} ${dati.ore[o]} ${dati.classi[c].nome}`,
        f.limite != null && pos === f.limite, c, o);
    }, tocco ? 0 : 250);
  };
}

// Sfoglia le marce indietro registrate con -b: l'orario parziale a ogni marcia, con lo slot dove si e' bloccata.
function sfogliaMarce(contenitore, dati, url, nomiMotivi) {
  const barra = el('input', {type: 'range', min: 1, max: 1, value: 1, style: 'width: 100%'});
  const numero = el('input', {type: 'number', min: 1, value: 1, class: 'stretto'});
  const stato = el('div', {class: 'tenue piccolo'});
  const griglia = el('div', {class: 'scorre'});
  const motivi = el('div', {});
  let totale = null, n = 1, richiesta = 0, ultima = null;
  // passando il mouse su una cella: la coda dei docenti in quel punto (vedi codaAlPassaggio)
  const passaggio = codaAlPassaggio(dati, url.replace(/marce$/, 'analisi'))(
    () => ultima && {chiave: 'marcia' + n, orario: ultima.orario, ordine: ultima.ordine, limite: ultima.pos});
  const bottone = (testo, delta) => el('button', {class: 'secondario', type: 'button', testo, onclick: () => vai(n + delta)});
  async function vai(nuovo) {
    n = Math.max(1, totale ? Math.min(totale, nuovo) : nuovo);
    barra.value = numero.value = n;
    const mia = ++richiesta;
    stato.textContent = 'Carico la marcia ' + n + '...';
    const r = await (await fetch(`${url}?n=${n}`)).json();
    if (mia !== richiesta) return;
    if (r.errore) { stato.textContent = r.errore; griglia.replaceChildren(); motivi.replaceChildren(); return; }
    n = r.n; barra.value = numero.value = n;
    const dove = `${dati.giorni[r.blocco[0]]} ${dati.ore[r.blocco[1]]} ${dati.classi[r.blocco[2]].nome}`;
    stato.textContent = `Marcia ${numeroIt(n)}` + (totale ? ` di ${numeroIt(totale)} (${(100 * n / totale).toLocaleString('it-IT', {maximumFractionDigits: 2})}%)` : '') +
      ` · nello slot ${r.pos} di ${r.slot}: nessun docente utilizzabile in ${dove} (✕)`;
    ultima = r;
    disegnaOrario(griglia, dati, r.orario, r.blocco, passaggio);
    // perche' non si puo' andare avanti: ogni docente con ore da fare in quella classe e il suo ostacolo
    if (!r.motivi || !r.motivi.length) {
      motivi.replaceChildren(el('p', {class: 'tenue piccolo', testo: r.motivi ? 'Nessun docente aveva ancora ore da fare in questa classe.' : ''}));
    } else {
      const per = {};
      r.motivi.forEach(([p, m]) => (per[m] = per[m] || []).push(dati.docenti[p].nome));
      motivi.replaceChildren(el('h3', {testo: `Perché non si può andare avanti in ${dove}`}), el('ul', {}, Object.entries(per).map(([m, nomi]) =>
        el('li', {}, [el('strong', {testo: nomiMotivi[m] + ': '}), el('span', {testo: nomi.join(', ')})]))));
    }
  }
  barra.addEventListener('change', () => vai(+barra.value));
  numero.addEventListener('change', () => vai(+numero.value));
  contenitore.replaceChildren(el('div', {style: 'display:flex;gap:8px;flex-wrap:wrap;align-items:center'}, [
    bottone('⏮', -1e12), bottone('−1000', -1000), bottone('−100', -100), bottone('−1', -1), numero,
    bottone('+1', 1), bottone('+100', 100), bottone('+1000', 1000), bottone('⏭', 1e12)]), barra, stato, motivi, griglia);
  fetch(url + '?totale=1').then(r => r.json()).then(t => {
    totale = t.totale; barra.max = totale; vai(n);
  });
  vai(1);
}

// Sfoglia i passi della ricerca registrati con --passi: l'orario parziale a ogni passo, dal passo 0 (orario vuoto).
// La sezione resta nascosta finche' non si sa che c'e' almeno un passo registrato.
function sfogliaPassi(sezione, contenitore, dati, url, nomiMotivi) {
  const barra = el('input', {type: 'range', min: 0, max: 0, value: 0, style: 'width: 100%'});
  const numero = el('input', {type: 'number', min: 0, value: 0, class: 'stretto'});
  const stato = el('div', {class: 'tenue piccolo'});
  const griglia = el('div', {class: 'scorre'});
  const motivi = el('div', {});
  const infoVicoli = el('div', {class: 'tenue piccolo'});
  let totale = null, n = 0, richiesta = 0, vicoli = [], vicoliTroncati = false;
  const di = () => totale ? ` di ${numeroIt(totale)}` : '';
  const bottone = (testo, delta) => el('button', {class: 'secondario', type: 'button', testo, onclick: () => vai(n + delta)});
  async function vai(nuovo) {
    n = Math.max(0, totale !== null ? Math.min(totale, nuovo) : nuovo);
    barra.value = numero.value = n;
    motivi.replaceChildren();
    if (n === 0) {  // il passo 0 non e' nel file: e' l'orario vuoto, prima che la ricerca cominci
      richiesta++;
      stato.textContent = `Passo 0${di()}: orario vuoto, la ricerca non è ancora cominciata.`;
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
    const dove = r.pos >= r.slot ? 'orario completo'
      : `la ricerca sta decidendo ${dati.giorni[r.blocco[0]]} ${dati.ore[r.blocco[1]]} ${dati.classi[r.blocco[2]].nome} (✕)`;
    stato.textContent = `Passo ${numeroIt(n)}${di()} · ${numeroIt(r.pos)} slot su ${numeroIt(r.slot)} assegnati; ${dove}`
      + (r.bloccato ? ' · ⚠ VICOLO CIECO: nessun docente utilizzabile qui, la ricerca torna indietro' : '');
    stato.style.fontWeight = r.bloccato ? '600' : '';
    if (r.bloccato) {
      const per = {};
      (r.motivi || []).forEach(([p, m]) => (per[m] = per[m] || []).push(dati.docenti[p].nome));
      const voci = Object.entries(per).map(([m, nomi]) => el('li', {}, [el('strong', {testo: (nomiMotivi[m] || m) + ': '}), el('span', {testo: nomi.join(', ')})]));
      motivi.replaceChildren(el('h3', {testo: 'Perché non si può andare avanti'}),
        voci.length ? el('ul', {}, voci) : el('p', {class: 'tenue piccolo', testo: r.motivi ? 'Nessun docente aveva ancora ore da fare in questa classe.' : 'Motivi non registrati: attiva «Motivi degli scarti» nelle opzioni.'}));
    }
    disegnaOrario(griglia, dati, r.orario, r.pos >= r.slot ? null : r.blocco, null, r.passaggi);
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
  numero.addEventListener('change', () => vai(+numero.value));
  contenitore.replaceChildren(el('div', {style: 'display:flex;gap:8px;flex-wrap:wrap;align-items:center'}, [
    bottone('⏮', -1e12), bottone('−1000', -1000), bottone('−100', -100), bottone('−1', -1), numero,
    bottone('+1', 1), bottone('+100', 100), bottone('+1000', 1000), bottone('⏭', 1e12)]),
    el('div', {style: 'display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-top:6px'}, [
      el('button', {class: 'secondario', type: 'button', testo: '◀ vicolo cieco precedente', onclick: () => {
        const prima = vicoli.filter(v => v < n); if (prima.length) vai(prima[prima.length - 1]); }}),
      el('button', {class: 'secondario', type: 'button', testo: 'vicolo cieco successivo ▶', onclick: () => {
        const dopo = vicoli.find(v => v > n); if (dopo) vai(dopo); }}), infoVicoli]),
    barra, stato, motivi, griglia);
  fetch(url + '?totale=1').then(r => r.json()).then(t => {
    totale = t.totale || 0;
    if (!totale) return;  // nessuna riga P nel file: la sezione resta nascosta
    if (sezione) sezione.style.display = '';
    barra.max = totale;
    vai(0);
    fetch(url + '?vicoli=1').then(r => r.json()).then(v => {
      vicoli = v.vicoli || []; vicoliTroncati = !!v.troncato;
      infoVicoli.textContent = vicoli.length
        ? `${numeroIt(vicoli.length)}${vicoliTroncati ? '+' : ''} ` + (vicoli.length === 1 && !vicoliTroncati ? 'vicolo cieco' : 'vicoli ciechi') + ` su ${numeroIt(totale)} passi` + (vicoli.length < 30 ? ` (passi ${vicoli.join(', ')})` : '')
        : 'Nessun vicolo cieco nei passi registrati: la ricerca è sempre andata avanti.';
    }).catch(() => {});
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

// Il contenuto del pannello al passaggio del mouse: scelto nello slot, coda in ordine di priorita', esito di ognuno.
function mostraAnalisi(pannello, dati, a, scelto, titolo, bloccato, classe, ora) {
  if (!a || a.errore) { pannello.textContent = (a && a.errore) || 'Analisi non disponibile.'; return; }
  const [p, q] = scelto;
  const nome = i => dati.docenti[i].nome;
  const testa = el('div', {}, [el('strong', {testo: titolo}),
    el('span', {class: 'tenue', testo: bloccato ? ' · qui la ricerca si è bloccata'
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
    corpo.appendChild(el('tr', {class: stato === 'escluso' ? '' : stato}, [
      el('td', {testo: nome(c.docente), style: 'white-space: nowrap'}), el('td', {}, [barra]), el('td', {class: 'num', testo: c.priorita}),
      el('td', {class: 'num', title: 'ore ancora da fare / ore totali in questa classe', testo: `${c.ore_classe}/${c.ore_totali_classe}`}),
      el('td', {testo: esito})]));
  });
  pannello.replaceChildren(testa,
    a.lezione_ammessa ? el('span') : el('div', {class: 'tenue piccolo', testo: 'In questo slot la classe non può più fare lezione: può solo restare vuoto.'}),
    el('p', {class: 'tenue piccolo', testo: 'Coda dei docenti per priorità (ore ancora da sistemare nella settimana); a parità di priorità l\'ordine dipende dalla ricerca. «Ore in classe» = ancora da fare / totali.'}),
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
