-- Un caso non risolto dei test notturni si puo' togliere dall'elenco «da verificare» (pagina del test in tempo reale):
-- la X lo segna come verificato e non viene piu' proposto. L'esecuzione e i suoi file restano.
alter table esecuzioni add column verificato boolean not null default false;
