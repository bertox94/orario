-- Cambiare il limite di nodi dei test notturni senza perdere la storia (richiesta di Alb dell'8 novembre 2026): dopo il
-- cambio ogni test gira due volte, con il limite nuovo e con quello di prima (ferma_nodi_prima), finche' le esecuzioni
-- dei suoi ultimi 5 giri non hanno tutti e due i limiti; fino ad allora la pagina, i peggiorati e i migliorati usano il
-- limite di prima (confrontabile con la storia), poi quello nuovo e il test gira una volta sola.
alter table test_notturni add column ferma_nodi_prima bigint;  -- null: nessun cambio in corso
alter table test_notturni add column riferimento_prima jsonb;  -- il riferimento accettato con il limite di prima

-- i test la cui ultima esecuzione ha un limite diverso da quello impostato (il limite e' gia' stato cambiato): da
-- adesso girano anche con il limite di prima
with ultime as (
    select distinct on (test_id) test_id, ferma_nodi from esecuzioni_notturne
    where ferma_nodi is not null and esito is not null and esito not in ('fermato', 'interrotto')
    order by test_id, id desc)
update test_notturni t set ferma_nodi_prima = u.ferma_nodi,
    riferimento_prima = case when (t.riferimento->>'ferma_nodi')::bigint = u.ferma_nodi then t.riferimento end
from ultime u
where u.test_id = t.id
  and u.ferma_nodi <> (select valore::bigint from impostazioni where nome = 'ferma_nodi_notturni');
