-- Sul Raspberry la SD non si consuma: i file delle prove (descrizione dei dati, orari, marce indietro compresse, uscita
-- del solver) stanno qui, a pezzi, e il Pi lavora solo in una cartella temporanea in RAM.

create table file_esecuzione (
    esecuzione_id int   not null references esecuzioni on delete cascade,
    nome          text  not null,
    n             int   not null,          -- numero del pezzo, da 0
    dati          bytea not null,
    primary key (esecuzione_id, nome, n)
);

-- Chiave per firmare i cookie di sessione (prima era un file nella cartella dei dati del sito).
create table impostazioni (
    nome   text primary key,
    valore text not null
);

-- Fotografia dei dati usati da una prova (formato --input del solver): sostituisce il file dati_scuola.json.
alter table prove add column dati_scuola jsonb;
-- Dati del caso per disegnare la mappa dei blocchi mentre la ricerca gira (prima vista.json).
alter table prove add column vista jsonb;
