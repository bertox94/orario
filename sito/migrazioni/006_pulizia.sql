-- Pulizia: via quello che il sito (seconda versione, sul solver di main) non usa piu'.
-- Restano utenti, scuole, dati della scuola, versioni salvate dei dati, impostazioni e i test del solver attuale.

-- I file delle esecuzioni del solver precedente (dati.txt, uscita, orari, ricerca passo passo compressa).
drop table file_esecuzione;

-- I test fatti con il solver precedente, notturni e in tempo reale (con le loro esecuzioni): i test di questo sito
-- hanno sempre max_backtrack nelle opzioni.
delete from prove where tipo = 'notturna' or not (opzioni ? 'max_backtrack');

-- Le prove notturne ricorrenti e il ricordo di quelle predefinite.
drop table pianificazioni;
delete from impostazioni where nome like 'pianificazione_predefinita_%';

-- Colonne non piu' usate: la mappa dei blocchi durante la prova, l'ora delle prove notturne, la cartella dei file e la
-- diagnosi completa del solver precedente.
alter table prove drop column vista, drop column programmata_per;
alter table esecuzioni drop column cartella, drop column diagnosi_testo;
