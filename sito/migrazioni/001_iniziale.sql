-- Struttura iniziale del database del sito.

-- Dati della scuola -----------------------------------------------------------------------------------------------

-- Una sola riga: i nomi dei giorni e delle ore, nell'ordine.
create table calendario (
    id     int primary key default 1 check (id = 1),
    giorni text[] not null,
    ore    text[] not null
);
insert into calendario (giorni, ore)
values ('{Lun,Mar,Mer,Gio,Ven,Sab}', '{08,09,10,11,12,13}');

create table classi (
    id   serial primary key,
    nome text not null unique check (nome <> '')
);

create table docenti (
    id       serial primary key,
    nome     text not null unique check (nome <> ''),
    ingresso int  not null default 0 check (ingresso >= 0) -- indice della prima ora in cui puo' insegnare
);

-- Una lezione: il docente fa `ore` ore a settimana nella classe; in `ore_codocente` di queste c'e' anche il
-- codocente (le altre le fa da solo). Come set_classe_insegnante_ore_cod nel solver.
create table lezioni (
    id            serial primary key,
    classe_id     int not null references classi on delete cascade,
    docente_id    int not null references docenti on delete cascade,
    ore           int not null check (ore > 0),
    codocente_id  int references docenti on delete cascade,
    ore_codocente int not null default 0 check (ore_codocente >= 0 and ore_codocente <= ore),
    check (codocente_id is null or codocente_id <> docente_id),
    check ((codocente_id is null) = (ore_codocente = 0))
);
create index on lezioni (classe_id);

-- Prove del solver --------------------------------------------------------------------------------------------------

-- Una prova: un test in tempo reale (un caso) o una batteria notturna (molti casi, in fasi).
create table prove (
    id              serial primary key,
    tipo            text not null check (tipo in ('live', 'notturna')),
    nome            text,
    stato           text not null,          -- in coda, in corso, finita, annullata, interrotta, errore
    opzioni         jsonb not null,
    commit          text,                    -- versione del solver (git) con cui e' stata eseguita
    creata          timestamptz not null default now(),
    programmata_per timestamptz,             -- prove notturne: quando partire
    inizio          timestamptz,
    fine            timestamptz,
    riepilogo       jsonb,
    messaggio       text
);
create index on prove (tipo, creata desc);

-- Un'esecuzione del solver: un caso in una fase di una prova.
create table esecuzioni (
    id             serial primary key,
    prova_id       int  not null references prove on delete cascade,
    caso           text not null,            -- nome leggibile, es. "seme 7 ritardi"
    caso_dati      jsonb not null,           -- il caso come lo vuole sito/solver.py
    fase           int  not null default 1,
    opzioni        jsonb,
    esito          text,                     -- null finche' e' in corso
    finito         boolean,
    marce          bigint,
    trovati        bigint,
    secondi        double precision,
    diagnosi       text,
    diagnosi_testo text,
    risultato      jsonb,                    -- il JSON scritto dal solver (--json)
    cartella       text,                     -- dove sono i file (dati.txt, orario, marce indietro)
    inizio         timestamptz not null default now(),
    fine           timestamptz
);
create index on esecuzioni (prova_id, caso, fase);

-- Prove notturne che si ripetono: ogni giorno indicato, all'ora indicata, si crea una prova con queste opzioni.
create table pianificazioni (
    id          serial primary key,
    nome        text not null,
    ora         time not null,
    giorni      int[] not null,              -- giorni della settimana: 1 = lunedi' ... 7 = domenica
    opzioni     jsonb not null,
    attiva      boolean not null default true,
    ultima_data date                         -- ultimo giorno in cui ha creato una prova
);
