-- I dati dei test notturni una volta per caso: prima ogni test (un caso con un solver) aveva la sua copia dei dati,
-- quindi lo stesso caso con 5 solver erano 5 copie. Ora i dati sono in dati_notturni, uno per caso, e i test li
-- indicano con dati_id. I dati si spostano, non si perde niente; prima, per sicurezza, una copia del database.
create table dati_notturni (
    id        serial primary key,
    scuola_id int   not null references scuole on delete cascade,
    impronta  text  not null,                      -- md5(dati::text): due casi con gli stessi dati sono uno solo
    dati      jsonb not null,                      -- i dati per --input
    unique (scuola_id, impronta)
);
insert into dati_notturni (scuola_id, impronta, dati)
    select distinct on (scuola_id, md5(dati::text)) scuola_id, md5(dati::text), dati from test_notturni;
alter table test_notturni add column dati_id int references dati_notturni;
update test_notturni t set dati_id = d.id from dati_notturni d
    where d.scuola_id = t.scuola_id and d.impronta = md5(t.dati::text);
alter table test_notturni alter column dati_id set not null;
drop index if exists test_notturni_stessi_dati;
alter table test_notturni drop column dati;
create index on test_notturni (scuola_id, solutore, dati_id);
create index on test_notturni (dati_id);
