create table orari
(
    column_name integer[],
    array_len   integer
);

alter table orari
    owner to albertotiraboschi;

create index orari_array_len_index
    on orari (array_len);

create table classi
(
    num integer
);

alter table classi
    owner to albertotiraboschi;

create table giorni
(
    num integer
);

alter table giorni
    owner to albertotiraboschi;

create table ore
(
    num integer
);

alter table ore
    owner to albertotiraboschi;

create table professors
(
    num integer
);

alter table professors
    owner to albertotiraboschi;