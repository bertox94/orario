-- Per trovare subito un test notturno con gli stessi dati (generare migliaia di test senza confrontare i dati di
-- tutti i test uno per uno): un indice sull'impronta dei dati.
create index if not exists test_notturni_stessi_dati on test_notturni (scuola_id, solutore, md5(dati::text));
