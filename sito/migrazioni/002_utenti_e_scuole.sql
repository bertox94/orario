-- Piu' scuole e piu' utenti. I dati che c'erano finiscono nella scuola 1.

create table scuole (
    id    serial primary key,
    nome  text not null unique check (nome <> ''),
    creata timestamptz not null default now()
);
insert into scuole (nome) values ('Scuola');

create table utenti (
    id             serial primary key,
    nome           text not null unique check (nome <> ''),
    password_hash  text not null,
    amministratore boolean not null default false,
    attivo         boolean not null default true,
    creato         timestamptz not null default now()
);

-- Le scuole a cui un utente puo' accedere (un amministratore le vede tutte).
create table utenti_scuole (
    utente_id int not null references utenti on delete cascade,
    scuola_id int not null references scuole on delete cascade,
    primary key (utente_id, scuola_id)
);

-- Calendario: una riga per scuola (prima c'era una riga sola).
alter table calendario drop column id;
alter table calendario add column scuola_id int references scuole on delete cascade;
update calendario set scuola_id = 1;
alter table calendario alter column scuola_id set not null;
alter table calendario add primary key (scuola_id);

-- Classi e docenti: i nomi sono unici dentro la scuola.
alter table classi add column scuola_id int references scuole on delete cascade;
update classi set scuola_id = 1;
alter table classi alter column scuola_id set not null;
alter table classi drop constraint classi_nome_key;
alter table classi add unique (scuola_id, nome);

alter table docenti add column scuola_id int references scuole on delete cascade;
update docenti set scuola_id = 1;
alter table docenti alter column scuola_id set not null;
alter table docenti drop constraint docenti_nome_key;
alter table docenti add unique (scuola_id, nome);

-- Prove e pianificazioni appartengono a una scuola (le esecuzioni seguono la prova).
alter table prove add column scuola_id int references scuole on delete cascade;
update prove set scuola_id = 1;
alter table prove alter column scuola_id set not null;
create index on prove (scuola_id, tipo, creata desc);

alter table pianificazioni add column scuola_id int references scuole on delete cascade;
update pianificazioni set scuola_id = 1;
alter table pianificazioni alter column scuola_id set not null;

-- Versioni salvate dei dati di una scuola (prima di un'importazione o di un ripristino, o salvate a mano).
-- `dati` e' nel formato del solver (vedi modello.per_solver): si puo' ripristinare e scaricare.
create table versioni_dati (
    id        serial primary key,
    scuola_id int  not null references scuole on delete cascade,
    nome      text not null,
    motivo    text not null,           -- 'manuale', 'prima di importare', 'prima di ripristinare'
    creata    timestamptz not null default now(),
    utente    text,
    classi    int  not null,
    docenti   int  not null,
    ore       int  not null,           -- ore di lezione in tutto (come le conta il riepilogo)
    dati      jsonb not null
);
create index on versioni_dati (scuola_id, creata desc);
