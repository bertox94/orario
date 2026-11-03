# Sito dell'orario

Tre sezioni:

- **Dati della scuola**: calendario, classi, docenti (con l'ora di ingresso) e lezioni, al posto dei dati scritti in
  `inizializza_dati()`. "Verifica" lancia il solver con `--solo-verifica` e dice subito se i dati sono impossibili e
  perché. Si possono importare i dati del programma o un file JSON, ed esportarli.
- **Test in tempo reale**: un caso (dati della scuola o casuali) lanciato subito e seguito mentre gira: nodi, marce
  indietro, mappa dei blocchi aggiornata ogni secondo, pulsante "Ferma". A fine corsa: orario trovato, diagnosi, file.
- **Test notturni**: batterie di casi in tre fasi (come `tests/casuali.py`), da avviare subito, a un'ora data o ogni
  notte (prove ricorrenti). Una prova alla volta; storico e confronto tra due prove.

Il sito usa il solver di questo repository: lo compila da solo e lo ricompila quando i sorgenti cambiano. Ogni prova
registra il commit con cui è stata eseguita.

## Sul Mac (sviluppo)

Postgres.app deve essere avviato. Una volta sola:

```bash
/Applications/Postgres.app/Contents/Versions/18/bin/createdb orario
python3 -m pip install --user -r sito/requirements.txt
python3 -m sito imposta-password
```

Poi, ogni volta:

```bash
python3 -m sito avvia
```

e si apre <http://localhost:8000>. Alla prima connessione Postgres.app chiede il permesso per Python: OK.

## Sul Raspberry Pi 5

Raspberry Pi OS a 64 bit. Consigliati: ventola attiva (le prove notturne tengono i processori al massimo per ore) e,
se si usa `-b` nei test notturni, un SSD per la cartella dei dati del sito.

### 1. Programmi

```bash
sudo apt update
sudo apt install -y git gcc python3-venv postgresql
```

### 2. Codice e librerie Python

```bash
git clone https://github.com/bertox94/orario.git ~/orario
python3 -m venv ~/orario-venv
~/orario-venv/bin/pip install -r ~/orario/sito/requirements.txt
```

### 3. Database

```bash
sudo -u postgres createuser --pwprompt orario
sudo -u postgres createdb -O orario orario
```

(`createuser` chiede la password del database: serve al punto 4.)

### 4. Configurazione

Creare `/etc/orario-sito.env` (con la password del punto 3):

```
DATABASE_URL=postgresql://orario:PASSWORD_DEL_DATABASE@localhost/orario
SITO_DATI=/home/UTENTE/orario-dati
SITO_PARALLELI=3
SITO_SPAZIO_MB=20480
```

e renderlo leggibile solo dall'amministratore:

```bash
sudo chmod 600 /etc/orario-sito.env
```

- `SITO_DATI`: dove vanno eseguibili, risultati delle prove e password del sito. Se c'è un SSD, metterla lì.
- `SITO_PARALLELI`: casi in parallelo nei test notturni (il Pi 5 ha 4 processori: 3 lascia libero il sito).
- `SITO_SPAZIO_MB`: spazio massimo per le marce indietro di una prova notturna (comunque non oltre il 90% del libero).

### 5. Password del sito

Con la stessa `SITO_DATI` del punto 4 (la password si salva lì):

```bash
cd ~/orario && SITO_DATI=/home/UTENTE/orario-dati ~/orario-venv/bin/python -m sito imposta-password
```

### 6. Servizio

In `sito/orario-sito.service` sostituire `UTENTE` con il proprio utente, poi:

```bash
sudo cp ~/orario/sito/orario-sito.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now orario-sito
```

Il sito parte all'accensione ed è su `http://<nome-del-pi>.local:8000` per tutta la rete locale.
Log: `journalctl -u orario-sito -f`.

### Aggiornare

```bash
cd ~/orario && git pull
sudo systemctl restart orario-sito
```

Il database si aggiorna da solo al riavvio (migrazioni in `sito/migrazioni/`); il solver si ricompila al primo test.
Una prova in corso al momento del riavvio risulta "interrotta".

## Note

- I test in tempo reale e quelli notturni possono girare insieme, ma si contendono i processori.
- Con `-b` il solver scrive le marce indietro in formato compatto e compresso (`orario.txt.gz`, circa 4 byte per marcia)
  e si ferma da solo quando supera lo spazio assegnato (`--max-mb`), scrivendo comunque diagnosi e risultati.
- "Rifai identico dal JSON" rilancia un caso con esattamente gli stessi dati e lo stesso ordine della coda dei docenti:
  la ricerca è identica.
- Le prove vecchie si cancellano dalla loro pagina (con i loro file).
