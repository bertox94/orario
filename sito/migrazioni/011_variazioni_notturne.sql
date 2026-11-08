-- I test notturni peggiorati e migliorati: alla fine di ogni giro, per ogni test eseguito, l'ultima esecuzione si
-- confronta con le precedenti (quelle ancora tenute, al massimo 4). Una riga per test e per tipo, con i numeri
-- dell'ultima esecuzione e di quella con cui si confronta: resta anche quando le esecuzioni si cancellano (se ne
-- tengono solo le ultime 5) e si toglie solo a mano dalla pagina.
create table variazioni_notturne (
    id         serial primary key,
    scuola_id  int  not null references scuole on delete cascade,
    test_id    int  not null references test_notturni on delete cascade,
    tipo       text not null check (tipo in ('peggiorato', 'migliorato')),
    quando     timestamptz not null default now(),   -- l'ultima volta che e' successo
    commit     text,                                 -- la versione del repository del giro
    dopo       jsonb not null,                       -- l'ultima esecuzione: {inizio, esito, nodi, backtrack, secondi}
    prima      jsonb not null,                       -- quella con cui si confronta (la migliore o la peggiore prima)
    unique (test_id, tipo)
);
create index on variazioni_notturne (scuola_id, tipo);
