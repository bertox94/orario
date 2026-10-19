CREATE OR REPLACE FUNCTION get_all_foo() RETURNS SETOF orari AS
$BODY$
DECLARE
r                             orari%rowtype;
    orario                        integer[];
    giorni                        integer   := (SELECT *
                                                FROM giorni);
    ore                           integer   := (SELECT *
                                                FROM ore);
    classi                        integer   := (SELECT *
                                                FROM classi);
    giorno                        integer   := 0;
    ora                           integer   := 0;
    classe                        integer   := 0;
    profs                         integer   := (SELECT *
                                                FROM professors);
    inizio_giornata               integer;
    inizio_classe                 integer;
    prof                          integer;
    sz                            integer;
    prof_gia_impegnato_stessa_ora integer   := 0;
    curr_pos                      integer;
    all_profs                     integer[] = array(select generate_series(0, profs - 1));
    free_profs                    integer[];
    start                         timestamp;
BEGIN
    start = timeofday();
INSERT INTO orari
SELECT ARRAY [pf], 1
FROM generate_series(0, profs - 1) AS pf;
RAISE NOTICE 'Completed for: giorno %, ora %, classi %', giorno, ora, classe;
    classe = classe + 1;

    LOOP
inizio_giornata = giorno * ore * classi;
        inizio_classe = inizio_giornata + ora * classi;
        curr_pos = inizio_classe + classe;
FOR orario IN
SELECT column_name
FROM orari
WHERE array_len = curr_pos
    LOOP
            --        RAISE NOTICE '% % array: %',inizio_giornata + classe + 1,
            --            inizio_giornata + (ora - 2) * classi + classe + 1,
            --            (select array_agg(orario[i])
            --             FROM generate_series(
            --                          inizio_giornata + classe + 1,-- 1 + 1 = 2 giorno 0 ora 0 classe 1
            --                          inizio_giornata +
            --                          (ora - 2) * classi + classe +
            --                          1,-- 0 -2*2 +1 + 1 = -2
            --                          classi) AS i
            --             where orario[i] is not null);

                FOR prof IN
SELECT p
FROM unnest(all_profs) AS p
WHERE p != ALL (orario[inizio_classe + 1:])  --il prof non è in altra classa la stessa ora
                      --  and p != ALL (select * from generate_series(1, 10) as i where i > 10)
                      AND p != ALL (SELECT orario[i]
                                    FROM generate_series(inizio_giornata + classe + 1,
                                                         inizio_giornata + (ora - 2) * classi + classe + 1,
                                                         classi) AS i
                                    where orario[i] is not null) --puoi scegliere p se e solo se c'era l'ora precedente oppure non ha ancora fatto un ora in questa classe
                      AND (
                        -- 1. Define the CTE inline
                        WITH existing_classes AS (SELECT DISTINCT (idx - 1) % classi AS class_id
                                                  FROM unnest(orario) WITH ORDINALITY AS t(prf, idx)
                                                  WHERE prf = prof
                                                    AND idx >= inizio_giornata + 1)
                        -- 2. Return a boolean result evaluating your two conditions
                        SELECT classe IN (SELECT class_id FROM existing_classes)
                                   OR (SELECT count(*) FROM existing_classes) < 3) -- max 3 classi diverse ogni giorno per prof
                    LOOP

INSERT INTO orari VALUES (orario || prof, curr_pos + 1);
END LOOP;
END LOOP;

        RAISE NOTICE 'Completed for: giorno %, ora %, classi %', giorno, ora, classe;
        classe = classe + 1;

        IF classe = classi THEN
            ora = ora + 1;
            classe = 0;
            IF ora = ore THEN
                giorno = giorno + 1;
                ora = 0;
end if;
end if;


        EXIT WHEN giorno = giorni;

        --Not sure if this speeds up
        --DELETE FROM orari WHERE array_len = array_length;

END LOOP;
    RETURN;
END;
$BODY$
LANGUAGE plpgsql;

truncate orari;
SELECT *
FROM get_all_foo();