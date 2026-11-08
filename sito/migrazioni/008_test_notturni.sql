-- I test notturni: ogni notte gli stessi dati con gli stessi solver, fermati sempre allo stesso nodo (--ferma-nodi), e
-- i numeri della ricerca confrontati con quelli accettati. Una modifica che non deve cambiare la ricerca deve dare
-- gli stessi numeri; se cambiano la pagina lo mostra e il cambiamento si accetta a mano.

-- Un test: dati (salvati, cosi' non cambiano se cambia il generatore), solver, opzioni e i valori accettati.
create table test_notturni (
    id            serial primary key,
    scuola_id     int   not null references scuole on delete cascade,
    nome          text  not null,                  -- es. "seme 3, 16 classi, ritardi"
    solutore      text  not null,
    caso          jsonb not null,                  -- da dove vengono i dati: {tipo: casuale, seme, classi, ritardi} o file
    dati          jsonb not null,                  -- i dati per --input
    opzioni       jsonb not null,                  -- {orari, ferma_nodi}
    attivo        boolean not null default true,   -- false: non si esegue piu' (resta con le sue esecuzioni)
    riferimento   jsonb,                           -- i valori accettati: {esito, nodi, backtrack, pos_max, orario}
    accettato     timestamptz,                     -- quando sono stati accettati
    accettato_da  text,                            -- da chi ('primo giro' per la prima esecuzione)
    creato        timestamptz not null default now()
);
create index on test_notturni (scuola_id);

-- Un giro: tutti i test attivi di una scuola, uno dopo l'altro (di notte o con «Esegui adesso»).
create table giri_notturni (
    id        serial primary key,
    scuola_id int  not null references scuole on delete cascade,
    stato     text not null,                       -- in corso, finito, interrotto
    motivo    text not null,                       -- notte, a mano
    commit    text,                                -- versione del repository
    inizio    timestamptz not null default now(),
    fine      timestamptz
);

-- Un test in un giro: i numeri della ricerca e il confronto con il riferimento di quel momento.
create table esecuzioni_notturne (
    id         serial primary key,
    giro_id    int  not null references giri_notturni on delete cascade,
    test_id    int  not null references test_notturni on delete cascade,
    inizio     timestamptz not null default now(),
    secondi    double precision,
    esito      text,                               -- null mentre gira
    nodi       bigint,
    backtrack  bigint,
    pos_max    int,
    orario     text,                               -- impronta del primo orario trovato
    errore     text,
    confronto  text,                               -- uguale, diverso, nuovo (non c'era riferimento), errore
    differenze jsonb                               -- [{campo, atteso, trovato}]
);
create index on esecuzioni_notturne (test_id, id);
create index on esecuzioni_notturne (giro_id);

-- l'ora dei test di notte (si cambia dalla pagina)
insert into impostazioni (nome, valore) values ('ora_test_notturni', '02:00') on conflict do nothing;
