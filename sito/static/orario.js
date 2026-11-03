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
function disegnaOrario(contenitore, dati, orario, blocco) {
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
      tr.appendChild(td);
    }));
    corpo.appendChild(tr);
  });
  contenitore.replaceChildren(el('table', {class: 'orario'}, [intestazione(dati), corpo]));
}

// Orario di ogni docente: una riga per docente, nella cella la classe in cui insegna (con il codocente, se c'e').
// Un docente in codocenza compare in entrambe le righe: nella cella c'e' la classe e, piccolo, l'altro docente.
function disegnaOrarioDocenti(contenitore, dati, orario) {
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
      tr.appendChild(td);
    }));
    corpo.appendChild(tr);
  });
  const legenda = el('div', {class: 'legenda'}, [
    el('span', {testo: '0'}), el('span', {class: 'scala'}),
    el('span', {testo: massimo.toLocaleString('it-IT') + ' ' + descrizione + ' (scala logaritmica)'})]);
  contenitore.replaceChildren(legenda, el('div', {class: 'scorre'}, [el('table', {class: 'orario mappa'}, [intestazione(dati), corpo])]));
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

// Gli slot dove la ricerca si e' bloccata di piu': conteggio e percentuale sul totale.
function elencoBlocchi(contenitore, dati, valore, quanti) {
  const righe = [];
  let totale = 0;
  dati.giorni.forEach((gn, g) => dati.ore.forEach((on, o) => dati.classi.forEach((cl, c) => {
    const v = valore(g, o, c);
    totale += v;
    if (v > 0) righe.push({v, nome: `${gn} ${on} ${cl.nome}`});
  })));
  righe.sort((a, b) => b.v - a.v);
  const corpo = el('tbody');
  righe.slice(0, quanti).forEach(r => corpo.appendChild(el('tr', {}, [
    el('td', {testo: r.nome}), el('td', {class: 'num', testo: numeroIt(r.v)}),
    el('td', {class: 'num', testo: (100 * r.v / totale).toLocaleString('it-IT', {maximumFractionDigits: 2}) + '%'})])));
  contenitore.replaceChildren(el('table', {class: 'dati'}, [
    el('thead', {}, [el('tr', {}, [el('th', {testo: 'Slot'}), el('th', {class: 'num', testo: 'Marce indietro'}), el('th', {class: 'num', testo: '% del totale'})])]), corpo]));
}

// Sfoglia le marce indietro registrate con -b: l'orario parziale a ogni marcia, con lo slot dove si e' bloccata.
function sfogliaMarce(contenitore, dati, url, nomiMotivi) {
  const barra = el('input', {type: 'range', min: 1, max: 1, value: 1, style: 'width: 100%'});
  const numero = el('input', {type: 'number', min: 1, value: 1, class: 'stretto'});
  const stato = el('div', {class: 'tenue piccolo'});
  const griglia = el('div', {class: 'scorre'});
  const motivi = el('div', {});
  let totale = null, n = 1, richiesta = 0;
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
    disegnaOrario(griglia, dati, r.orario, r.blocco);
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
