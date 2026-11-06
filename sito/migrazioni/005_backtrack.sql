-- I backtrack registrati di un test, dalla traccia del solver (--traccia), uno per riga. Si scrivono mentre il test
-- gira, cosi' la pagina del test li mostra subito.
create table backtrack (
    esecuzione_id int     not null references esecuzioni on delete cascade,
    n             int     not null,          -- numero del backtrack nella ricerca (da 1)
    nodo          bigint  not null,          -- a quale nodo della ricerca
    pos           int     not null,          -- posizione dello slot nella ricerca (giorno, ora, classe)
    giorno        int     not null,
    ora           int     not null,
    classe        int     not null,          -- indice della classe nei dati del test
    buca_dopo     boolean not null,          -- dopo si prova l'ora buca: non e' un vicolo cieco
    riassunto     jsonb   not null,          -- {controllo: quanti docenti si sono fermati li'} (per l'elenco)
    record        jsonb   not null,          -- il record della traccia: controlli di ogni docente e orario fin li'
    primary key (esecuzione_id, n)
);
