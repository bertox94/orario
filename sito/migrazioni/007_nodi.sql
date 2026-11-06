-- I nodi della ricerca di un test (traccia del solver con --max-nodi), per scorrerla passo passo nella pagina del
-- test. Un pezzo per riga della traccia: i nodi da "da" in poi, ognuno [pos, docente1, docente2] (la posizione a cui
-- arriva la ricerca e la coppia appena messa nella posizione prima).
create table nodi (
    esecuzione_id int    not null references esecuzioni on delete cascade,
    da            bigint not null,               -- il primo nodo del pezzo
    quanti        int    not null,               -- quanti nodi nel pezzo
    passi         jsonb  not null,               -- [[pos, p1, p2], ...]
    primary key (esecuzione_id, da)
);
