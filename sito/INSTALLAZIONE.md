# Sito dell'orario

Due sezioni:

- **Dati della scuola**: calendario, classi, docenti (con l'ora di ingresso) e lezioni, al posto dei dati scritti in
  `inizializza_dati()`. Si possono importare i dati del programma (letti dal sorgente di `inizializza_dati()`) o un file
  JSON, ed esportarli.
- **Test in tempo reale**: un test (dati della scuola, casuali o da un file) lanciato subito e seguito mentre gira, con
  il pulsante "Ferma". A ogni backtrack la pagina mostra l'orario fino a quel punto e, per ogni docente della classe,
  i controlli della ricerca con i valori che leggono e il loro esito. A fine corsa: esito e orario trovato.

Il sito usa il solver di questo repository: lo compila da solo e lo ricompila quando i sorgenti cambiano. Ogni prova
registra il commit con cui è stata eseguita.

## Sul Mac (sviluppo)

Postgres.app (la 18) deve essere avviato. Una volta sola:

```bash
/Applications/Postgres.app/Contents/Versions/18/bin/createdb orario
python3 -m pip install --user -r sito/requirements.txt
python3 -m sito crea-utente admin --admin
```

Poi, ogni volta:

```bash
python3 -m sito avvia
```

e si apre <http://localhost:8000>. Alla prima connessione Postgres.app chiede il permesso per Python: OK.

## Database: Postgres 18 sul Mac, sito sul Raspberry

Il database sta sul Mac (Postgres.app 18) e il sito sul Raspberry, nella stessa rete locale. Se il Mac è spento o in
stop il sito mostra «Database non raggiungibile» e riprende da solo appena il Mac torna: non serve riavviare nulla.

### Sul Mac (una volta sola)

1. **Postgres.app 18** avviato, con un server sulla porta 5432. Nelle impostazioni del server («Server Settings…») si
   vede la cartella dei dati, di solito `~/Library/Application Support/Postgres/var-18`: dentro ci sono
   `postgresql.conf` e `pg_hba.conf`.
2. In `postgresql.conf` far ascoltare il server sulla rete (di default ascolta solo il Mac stesso):

   ```
   listen_addresses = '*'
   ```

3. Creare l'utente e il database (stesso terminale; la password sarà quella del punto 5):

   ```bash
   /Applications/Postgres.app/Contents/Versions/18/bin/psql -p 5432 -c "create role orario login password 'PASSWORD_DEL_DATABASE'"
   /Applications/Postgres.app/Contents/Versions/18/bin/psql -p 5432 -c "create database orario owner orario"
   ```

4. In `pg_hba.conf` ammettere solo il Raspberry, aggiungendo in fondo una riga (sostituire l'indirizzo con quello del Pi:
   sul Pi `hostname -I`):

   ```
   host    orario    orario    192.168.1.50/32    scram-sha-256
   ```

   Poi **fermare e riavviare il server** in Postgres.app (le due modifiche si leggono all'avvio).
5. Il Mac deve restare raggiungibile:
   - Impostazioni di Sistema → Rete → Firewall: se è attivo, consentire le connessioni in ingresso a Postgres
     (la prima volta macOS lo chiede; se il Pi non si collega, controllare qui).
   - Impostazioni di Sistema → Batteria / Risparmio energia: impedire lo stop automatico quando è collegato
     all'alimentazione (altrimenti di notte il sito perde il database). Postgres.app va aperto: «Apri al login».
   - L'indirizzo del Mac deve restare lo stesso: riservarlo nel router (DHCP) oppure usare il nome Bonjour
     `nome-del-mac.local` (Impostazioni → Generali → Condivisione → Nome host locale). Il Raspberry Pi OS risolve i
     nomi `.local`.

La rete locale non è cifrata: va bene a casa o a scuola su una rete fidata, e `pg_hba.conf` lascia entrare solo il Pi con
la password. Non esporre la porta 5432 a Internet.

### Sul Raspberry: controllo del collegamento

Dopo aver configurato `DATABASE_URL` (punto 4 qui sotto):

```bash
cd ~/orario && ~/orario-venv/bin/python -m sito prova-database
```

Scrive «Collegato a Postgres 18.x» oppure il motivo per cui non ci riesce (host sbagliato, password, firewall,
`pg_hba.conf`). Attesa massima per il collegamento: 5 secondi (`SITO_DB_TIMEOUT`).

## Sul Raspberry Pi 5

Raspberry Pi OS a 64 bit. Consigliati: ventola attiva (un test lungo tiene un processore al massimo) e,
almeno 4 GB di RAM (le prove lavorano in RAM, vedi sotto). La SD non si consuma: tutti i risultati stanno
nel database sul Mac.

### 1. Programmi

```bash
sudo apt update
sudo apt install -y git gcc python3-venv
```

### 2. Codice e librerie Python

```bash
git clone https://github.com/bertox94/orario.git ~/orario
python3 -m venv ~/orario-venv
~/orario-venv/bin/pip install -r ~/orario/sito/requirements.txt
```

### 3. Database

Non si installa Postgres sul Pi: il database è sul Mac (sezione «Database: Postgres 18 sul Mac», sopra). Servono
l'indirizzo del Mac e la password dell'utente `orario`.

### 4. Configurazione

Creare `/etc/orario-sito.env` (indirizzo del Mac e password della sezione sopra):

```
DATABASE_URL=postgresql://orario:PASSWORD_DEL_DATABASE@nome-del-mac.local:5432/orario
SITO_PARALLELI=3
```

Se la password contiene caratteri speciali (`@ : / # ? %`) vanno scritti in forma `%40`, `%3A`… nell'indirizzo; più semplice
scegliere una password di sole lettere e cifre. Poi renderlo leggibile solo dall'amministratore:

```bash
sudo chmod 600 /etc/orario-sito.env
```

- `SITO_LAVORO` (facoltativa): cartella temporanea per il solver compilato e per i file di una prova mentre gira.
  Default `/dev/shm/orario-sito`, cioè in RAM: sulla SD non si scrive nulla. Si svuota da sola a ogni riavvio.

### 5. Primo utente

```bash
cd ~/orario && ~/orario-venv/bin/python -m sito crea-utente admin --admin
```

Chiede una password (almeno 8 caratteri). Entrando come `admin` si creano da «amministrazione» le scuole e gli altri
utenti: ogni docente ha il suo nome e la sua password e vede solo le scuole che gli abbini; un amministratore le vede
tutte. Chi ha una sola scuola va dritto ai suoi dati, gli altri la scelgono dopo l'accesso. Dati, prove e versioni
salvate sono separati per scuola.

### 5b. Niente scritture sulla SD

Il sito non scrive file sul Pi: la chiave dei cookie, i dati, i test, i loro backtrack e le uscite del
solver vanno in Postgres; il solver lavora in RAM (`/dev/shm`). Per ridurre al minimo anche i log di sistema:

```bash
sudo mkdir -p /etc/systemd/journald.conf.d
printf '[Journal]\nStorage=volatile\nRuntimeMaxUse=50M\n' | sudo tee /etc/systemd/journald.conf.d/ram.conf
sudo systemctl restart systemd-journald
```

(i log restano fino allo spegnimento). Per le cartelle `/tmp` e `/var/log` in RAM si può aggiungere `tmpfs` in `/etc/fstab`.
Compromesso: se il Mac è spento il sito non funziona (mostra la pagina «database non raggiungibile»), e ogni riavvio del
Pi ricompila il solver (pochi secondi).

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

- Il solver è quello di `main`, con l'ingresso e l'uscita per il sito (`ingresso_uscita/`): legge i dati con
  `--input` e scrive la traccia con `--traccia`. La ricerca e i controlli sono quelli di `main`.
- I test vecchi si cancellano dall'elenco (con i loro backtrack nel database), così il database sul Mac non cresce
  inutilmente.

## Accesso da fuori casa con Tailscale (gratis fino a 6 utenti)

Tailscale crea una rete privata tra i dispositivi invitati: il sito resta invisibile a Internet, senza aprire porte sul
router. Il piano gratuito «Personal» (uso non commerciale) ha dispositivi illimitati e fino a 6 utenti. Chi deve usare il
sito installa Tailscale sul proprio telefono o computer e accede con l'indirizzo HTTPS del Pi.

1. Sul Raspberry (o sul Mac, se il sito gira lì): installare Tailscale (https://tailscale.com/download) e collegarlo
   al proprio account: `sudo tailscale up`.
2. Nella console di amministrazione Tailscale (login.tailscale.com): attivare **MagicDNS** e i **certificati HTTPS**
   (sezione DNS). Invitare gli altri utenti (Users → Invite) o condividere solo il Pi con loro (Machines → Share).
3. Pubblicare il sito sulla rete privata con HTTPS: `sudo tailscale serve --bg 8000` (con una versione diversa
   verificare la sintassi con `tailscale serve --help`). Il comando scrive l'indirizzo, per esempio
   `https://orario-pi.nome-rete.ts.net`.
4. In `/etc/orario-sito.env` aggiungere `SITO_PROXY=1`. Se il sito deve rispondere **solo** a Tailscale, in
   `orario-sito.service` cambiare `--host 0.0.0.0` in `--host 127.0.0.1`; lasciando `0.0.0.0` resta raggiungibile anche
   in chiaro sulla rete locale (`http://192.168.1.5:8000`). Poi `sudo systemctl daemon-reload && sudo systemctl restart orario-sito`.
5. Provare dal telefono con Tailscale attivo (e Wi-Fi spento) aprendo l'indirizzo del punto 3.

Il cookie di sessione è «Secure» da solo quando la pagina arriva in HTTPS, e normale in HTTP: si può entrare sia da
Tailscale sia dalla rete locale. `SITO_PROXY=1` fa leggere indirizzo e protocollo veri del client dalle intestazioni
del proxy, così il limite ai tentativi di login conta ogni persona e non il proxy.

## Sito sul Mac invece che sul Pi

Il Mac ha già Postgres, quindi basta far girare lì anche il sito: `DATABASE_URL=postgresql://orario:PASSWORD@localhost:5432/orario`.
Servono `git`, un compilatore (`xcode-select --install`) e Python 3 con `pip install -r sito/requirements.txt` in un
ambiente virtuale. Si avvia con `python3 -m sito avvia --porta 8000`. Il sito usa la cartella temporanea del sistema
al posto di `/dev/shm` (che sul Mac non c'è; nessun problema con un SSD). Per farlo partire all'accensione si usa
launchd al posto di systemd, e va impedito lo stop del Mac (Impostazioni → Batteria/Energia, oppure `caffeinate -s`),
altrimenti il sito si ferma.

## Come si usa il test in tempo reale

1. **Dati della scuola**: inserire calendario, classi, docenti (con l'ora di ingresso) e lezioni (ore, codocente, ore in
   coppia), oppure importare i dati del programma. La pagina mostra gli avvisi sui limiti evidenti.
2. **Test in tempo reale**, scegliere i dati: *dati della scuola*, *dati casuali* (seme, numero di classi, docenti in
   ritardo: stesso seme, stessi dati) o *da un file JSON* (esportato dai dati, o i dati di un test).
3. **Ricerca**: tempo massimo (0 = senza limite), quanti orari trovare, quanti backtrack registrare per esteso (gli
   altri si contano soltanto).
4. **Avvia**: la pagina del test mostra il progresso (nodi, backtrack, slot) e i backtrack man mano che arrivano.
   Un backtrack è uno slot in cui la ricerca non riesce a mettere nessun docente. Scegliendolo si vedono:
   - il controllo sullo slot (la classe può fare lezione in quell'ora?);
   - l'orario fino a quel punto, con lo slot evidenziato;
   - per ogni docente della classe, nell'ordine della coda della ricerca, i controlli di `main` con i valori che
     leggono (ingressi) e l'esito (✓ / ✗). La ricerca si ferma al primo ✗: i controlli dopo non li valuta e sono in
     grigio. Chi passa i controlli del primo docente prova le coppie (con i docenti dopo di lui nella coda con cui ha
     ore insieme) e poi da solo.
   ← e → passano da un backtrack all'altro. Alla fine c'è l'orario trovato; «Rifai il test con gli stessi dati» ripete
   esattamente la stessa ricerca.
5. **Test da verificare**: in «Test in tempo reale» c'è l'elenco dei test che non sono arrivati a una conclusione
   (fermati, finiti per il tempo, in errore); la ✕ li toglie dall'elenco.
