#!/bin/bash
# Sul Raspberry: Postgres 18 sul Pi, con il database del sito caricato da una copia del Mac. Di solito lo lancia
# sito/sposta-database-sul-pi.sh dal Mac; a mano, dalla cartella del repository sul Pi:
#
#   bash sito/database-sul-pi.sh ~/orario.dump
#
# Installa Postgres 18 (dal repository ufficiale di Postgres: la copia viene dal 18 del Mac), crea l'utente del
# database con lo stesso nome dell'utente del Pi (si entra senza password, solo da questo computer) e il database
# «orario», ci carica la copia, poi in /etc/orario-sito.env mette DATABASE_URL verso il database del Pi (la riga di
# prima resta in /etc/orario-sito.env.mac) e riavvia il sito. Se il database «orario» sul Pi ha gia' delle tabelle si
# ferma senza toccare niente.
set -euo pipefail
COPIA=${1:?Uso: bash sito/database-sul-pi.sh COPIA.dump}
[ -f "$COPIA" ] || { echo "Non trovo $COPIA"; exit 1; }
UTENTE=$(id -un)
BIN=/usr/lib/postgresql/18/bin

echo "== Postgres 18"
if [ ! -x "$BIN/postgres" ]; then
    sudo apt-get update
    # il repository ufficiale di Postgres (apt.postgresql.org): Raspberry Pi OS ha una versione piu' vecchia
    sudo apt-get install -y curl ca-certificates
    sudo install -d /usr/share/postgresql-common/pgdg
    sudo curl -fsSL -o /usr/share/postgresql-common/pgdg/apt.postgresql.org.asc https://www.postgresql.org/media/keys/ACCC4CF8.asc
    . /etc/os-release
    echo "deb [signed-by=/usr/share/postgresql-common/pgdg/apt.postgresql.org.asc] https://apt.postgresql.org/pub/repos/apt $VERSION_CODENAME-pgdg main" \
        | sudo tee /etc/apt/sources.list.d/pgdg.list > /dev/null
    sudo apt-get update
    sudo apt-get install -y postgresql-18
fi
PORTA=$(pg_lsclusters --no-header | awk '$1 == "18" && $2 == "main" {print $3}')
[ -n "$PORTA" ] || { echo "Non trovo il server Postgres 18 (pg_lsclusters)"; exit 1; }
sudo systemctl enable --now postgresql
echo "Postgres 18 sulla porta $PORTA"

echo "== Utente e database"
psql_admin() { sudo -u postgres "$BIN/psql" -p "$PORTA" -tAc "$1"; }
[ "$(psql_admin "select 1 from pg_roles where rolname = '$UTENTE'")" = 1 ] || sudo -u postgres "$BIN/createuser" -p "$PORTA" "$UTENTE"
if [ "$(psql_admin "select 1 from pg_database where datname = 'orario'")" = 1 ]; then
    TABELLE=$(sudo -u postgres "$BIN/psql" -p "$PORTA" -d orario -tAc "select count(*) from pg_tables where schemaname = 'public'")
    [ "$TABELLE" = 0 ] || { echo "Il database orario sul Pi ha gia' $TABELLE tabelle: non carico niente."; exit 1; }
else
    sudo -u postgres "$BIN/createdb" -p "$PORTA" -O "$UTENTE" orario
fi

echo "== Carico la copia"
"$BIN/pg_restore" -p "$PORTA" --no-owner --no-privileges --exit-on-error --dbname=orario "$COPIA"
"$BIN/psql" -p "$PORTA" -d orario -tAc "select 'scuole: ' || count(*) from scuole"
"$BIN/psql" -p "$PORTA" -d orario -c "analyze" > /dev/null

echo "== Configurazione del sito"
URL="postgresql:///orario?host=/var/run/postgresql&port=$PORTA"
[ -f /etc/orario-sito.env.mac ] || sudo cp /etc/orario-sito.env /etc/orario-sito.env.mac
sudo sed -i "s#^DATABASE_URL=.*#DATABASE_URL=$URL#" /etc/orario-sito.env
# il sito parte dopo Postgres
if [ -f /etc/systemd/system/orario-sito.service ] && ! grep -q postgresql.service /etc/systemd/system/orario-sito.service; then
    sudo sed -i 's/^After=network-online.target$/After=network-online.target postgresql.service/; s/^Wants=network-online.target$/Wants=network-online.target postgresql.service/' /etc/systemd/system/orario-sito.service
    sudo systemctl daemon-reload
fi
DATABASE_URL=$URL ~/orario-venv/bin/python -m sito prova-database

echo "== Riavvio il sito"
sudo systemctl restart orario-sito
sleep 5
systemctl --no-pager --lines=5 status orario-sito || true
echo "Fatto: il sito usa il database del Pi ($URL)."
