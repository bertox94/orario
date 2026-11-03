// Test manuale: totali di ore e controlli, ricalcolati mentre si modificano i dati.
(function () {
  const lez = document.getElementById('t-lezioni');
  if (!lez) return;
  const td = document.getElementById('t-docenti'), tc = document.getElementById('t-classi');
  const giorni = +td.dataset.giorni, ore = +td.dataset.ore, minimo = +tc.dataset.minimo, massimo = +tc.dataset.massimo;

  function ricalcola() {
    const perClasse = {}, perDocente = {};
    lez.querySelectorAll('tbody tr').forEach(r => {
      if (r.querySelector('[name^=x_]').checked) return;
      const n = +r.querySelector('[name^=ore_]').value || 0;
      const cod = r.querySelector('[name^=cod_]').value;
      const oc = cod ? (+r.querySelector('[name^=oc_]').value || 0) : 0;
      perClasse[r.dataset.classe] = (perClasse[r.dataset.classe] || 0) + n;
      perDocente[r.dataset.docente] = (perDocente[r.dataset.docente] || 0) + n;
      if (cod) perDocente[cod] = (perDocente[cod] || 0) + oc;
      r.querySelector('[name^=oc_]').disabled = !cod;
      r.querySelector('[name^=oc_]').style.opacity = cod ? 1 : 0.4;
    });
    tc.querySelectorAll('tbody tr').forEach(r => {
      const n = perClasse[r.dataset.id] || 0;
      r.querySelector('.somma').textContent = n;
      r.querySelector('.avviso').textContent = n < minimo ? 'meno di ' + minimo + ' ore' : n > massimo ? 'piu\' di ' + massimo + ' ore' : 'ok';
    });
    td.querySelectorAll('tbody tr').forEach(r => {
      const n = perDocente[r.dataset.id] || 0;
      const ing = +r.querySelector('select').value;
      const disp = Math.max(0, ore - ing) * giorni;
      r.querySelector('.somma').textContent = n;
      r.querySelector('.avviso').textContent = n > disp ? 'ne ha al massimo ' + disp + ' dall\'ingresso' : 'ok';
    });
  }
  document.getElementById('modifiche').addEventListener('input', ricalcola);
  document.getElementById('modifiche').addEventListener('change', ricalcola);
  ricalcola();
})();
