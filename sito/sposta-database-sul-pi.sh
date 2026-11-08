#!/bin/bash
# Sposta il database del sito dal Mac al Raspberry. Si lancia sul Mac, dalla cartella del repository:
#
#   bash sito/sposta-database-sul-pi.sh [raspberry@192.168.1.5]
#
# 1. ferma il sito sul Pi (cosi' durante la copia nessuno scrive nel database del Mac);
# 2. copia il database del Mac in ~/orario-AAAAMMGG-HHMM.dump (resta sul Mac: e' anche la copia di sicurezza);
# 3. manda la copia al Pi e li' lancia sito/database-sul-pi.sh, che installa Postgres 18 (se non c'e'), crea il
#    database, ci carica la copia, cambia DATABASE_URL in /etc/orario-sito.env e riavvia il sito.
#
# Il database del Mac non si tocca: se qualcosa va storto basta rimettere la riga DATABASE_URL di prima (salvata in
# /etc/orario-sito.env.mac) e riavviare il sito. Il Pi chiede la password di sudo.
set -euo pipefail
PI=${1:-raspberry@192.168.1.5}
PG_DUMP=$(command -v pg_dump || echo /Applications/Postgres.app/Contents/Versions/18/bin/pg_dump)
COPIA=~/orario-$(date +%Y%m%d-%H%M).dump

echo "== 1. Fermo il sito sul Pi"
ssh -t "$PI" 'sudo systemctl stop orario-sito'

echo "== 2. Copia del database del Mac in $COPIA"
"$PG_DUMP" --format=custom --no-owner --no-privileges --dbname=orario --file="$COPIA"
ls -lh "$COPIA"

echo "== 3. Sul Pi: aggiorno il codice, carico la copia e riavvio il sito"
scp "$COPIA" "$PI:orario.dump"
ssh -t "$PI" 'cd ~/orario && git pull && bash sito/database-sul-pi.sh ~/orario.dump'

echo "Fatto. Il database del Mac e' rimasto com'era; la copia e' in $COPIA."
