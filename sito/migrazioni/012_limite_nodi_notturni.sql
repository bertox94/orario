-- Il limite di nodi dei test notturni non e' piu' di ogni test ma uno solo, in alto nella pagina (impostazione
-- 'ferma_nodi_notturni'): ogni esecuzione lo legge quando parte e lo scrive nella sua riga (ferma_nodi), e un
-- riferimento vale per il limite con cui e' stato fissato. Non si cancella niente: i test che diventano uguali (stessi
-- dati, solver e orari, prima con limiti diversi) restano, ma ne resta attivo uno solo.

-- il limite di partenza: quello dei test attivi piu' frequente (se non ce ne sono, 50.000.000), in milioni da 1 a 100
-- (nella pagina si sceglie in milioni)
insert into impostazioni (nome, valore)
    select 'ferma_nodi_notturni', (greatest(1, least(100, round(coalesce(
        (select (opzioni->>'ferma_nodi')::bigint from test_notturni where attivo and opzioni ? 'ferma_nodi'
         group by 1 order by count(*) desc, 1 limit 1), 50000000) / 1000000.0))) * 1000000)::bigint::text
on conflict (nome) do nothing;

alter table esecuzioni_notturne add column ferma_nodi bigint;
update esecuzioni_notturne n set ferma_nodi = (t.opzioni->>'ferma_nodi')::bigint
    from test_notturni t where t.id = n.test_id;
update test_notturni set riferimento = riferimento || jsonb_build_object('ferma_nodi', (opzioni->>'ferma_nodi')::bigint)
    where riferimento is not null and opzioni ? 'ferma_nodi';
update test_notturni set opzioni = opzioni - 'ferma_nodi';

-- dei test attivi diventati uguali ne resta attivo uno: quello con il limite di partenza, se c'e', se no il primo
update test_notturni t set attivo = false
from (
    select id, row_number() over (
        partition by scuola_id, solutore, dati_id, opzioni
        order by ((riferimento->>'ferma_nodi') is not distinct from
                  (select valore from impostazioni where nome = 'ferma_nodi_notturni')) desc, id) as k
    from test_notturni where attivo) v
where t.id = v.id and v.k > 1;
