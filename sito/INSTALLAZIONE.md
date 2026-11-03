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

Raspberry Pi OS a 64 bit. Consigliati: ventola attiva (le prove notturne tengono i processori al massimo per ore) e,
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
SITO_SPAZIO_MB=20480
```

Se la password contiene caratteri speciali (`@ : / # ? %`) vanno scritti in forma `%40`, `%3A`… nell'indirizzo; più semplice
scegliere una password di sole lettere e cifre. Poi renderlo leggibile solo dall'amministratore:

```bash
sudo chmod 600 /etc/orario-sito.env
```

- `SITO_LAVORO` (facoltativa): cartella temporanea per il solver compilato e per i file di una prova mentre gira.
  Default `/dev/shm/orario-sito`, cioè in RAM: sulla SD non si scrive nulla. Si svuota da sola a ogni riavvio.
- `SITO_PARALLELI`: casi in parallelo nei test notturni (il Pi 5 ha 4 processori: 3 lascia libero il sito).
- `SITO_SPAZIO_MB`: spazio massimo, **nel database sul Mac**, per le marce indietro di una prova notturna.

### 5. Primo utente

```bash
cd ~/orario && ~/orario-venv/bin/python -m sito crea-utente admin --admin
```

Chiede una password (almeno 8 caratteri). Entrando come `admin` si creano da «amministrazione» le scuole e gli altri
utenti: ogni docente ha il suo nome e la sua password e vede solo le scuole che gli abbini; un amministratore le vede
tutte. Chi ha una sola scuola va dritto ai suoi dati, gli altri la scelgono dopo l'accesso. Dati, prove e versioni
salvate sono separati per scuola. Con il sito già in uso con la vecchia password unica, al primo avvio quella password
diventa l'utente `admin` e i dati finiscono nella «Scuola» 1.

### 5b. Niente scritture sulla SD

Il sito non scrive file sul Pi: la chiave dei cookie, i dati, le prove, le marce indietro compresse e le uscite del
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

- I test in tempo reale e quelli notturni possono girare insieme, ma si contendono i processori.
- Con `-b` il solver scrive le marce indietro in formato compatto e compresso (`orario.txt.gz`, circa 4 byte per marcia)
  e si ferma da solo quando supera lo spazio assegnato (`--max-mb`), scrivendo comunque diagnosi e risultati.
- "Rifai identico dal JSON" rilancia un caso con esattamente gli stessi dati e lo stesso ordine della coda dei docenti:
  la ricerca è identica.
- Le prove vecchie si cancellano dalla loro pagina (con i loro file nel database). Le marce indietro (`-b`) occupano
  spazio sul Mac: si cancellano le prove che non servono più.
- Il database cresce con le prove: controllare ogni tanto lo spazio del Mac (la pagina «Test notturni» mostra la
  dimensione del database).

## Come si usa il test manuale (Test in tempo reale)

1. **Dati della scuola**: inserire calendario, classi, docenti (con l'ora di ingresso) e lezioni (ore, codocente, ore in
   coppia). La pagina mostra gli avvisi sui limiti evidenti.
2. **Test in tempo reale**, scegliere il caso:
   - *Dati della scuola*: i dati veri così come sono;
   - *Dati della scuola, modificati per questa prova*: si apre «Dati modificabili per la prova». Si possono cambiare
     l'**ora di ingresso di ogni docente**, le **ore di ogni lezione**, il **codocente** e le **ore in coppia**, e togliere
     lezioni. I **totali** di ogni classe e di ogni docente si aggiornano subito, con l'avviso se una classe ha meno
     ore delle obbligatorie o più degli slot, o se un docente ha più ore di quante ne può fare dalla sua ora di ingresso.
     I dati veri non cambiano; la prova conserva la propria copia e la si può rifare identica;
   - *Dati casuali*: seme, numero di classi, docenti in ritardo.
3. **Ricerca**: tempo massimo, quanti orari trovare, ordine delle classi, motivi degli scarti, marce indietro (`-b`, con
   lo spazio massimo in MB, che finisce nel database).
4. **Parametri della ricerca**: ore obbligatorie al giorno (5), ore massime di un docente nella stessa classe al giorno (3),
   classi diverse per docente al giorno (3).
5. **Avvia**: la pagina mostra progresso e mappa dei blocchi; alla fine si apre il caso con esito, diagnosi, orario
   (anche per docente) e file da scaricare (dati, uscita, orari, marce indietro `.gz`, JSON). «Rifai identico» ripete
   esattamente la stessa ricerca; «Rilancia in tempo reale» riporta le opzioni nel modulo.
6. **Capire dove si blocca**: nella pagina di un caso, «Slot con più marce indietro» mostra, per ogni slot, quante marce
   indietro e che percentuale del totale. Se la prova ha usato `-b` (fase 3 dei notturni, o la casella nel test in tempo
   reale) c'è anche «Orario a ogni marcia indietro»: si sfogliano le marce una per una (barra, ±1, ±100, ±1000, prima e
   ultima) e per ognuna si vede l'orario parziale in quel momento, lo slot bloccato (✕), il punto della sequenza in percentuale e, con i motivi attivi (casella «Motivi degli scarti»), per ogni docente con ore da fare in quella classe il problema per cui non si può andare avanti.
7. **Casi dei test notturni da verificare**: in «Test in tempo reale» sotto il modulo c'è l'elenco degli ultimi casi non
   risolti dei notturni; «Verifica» li carica nel modulo (stessi dati e opzioni, anche per i dati della scuola) per rifarli
   a mano con altri parametri, tempo o ordine delle classi. Lo stesso pulsante «Verifica a mano» c'è nella pagina di ogni prova notturna.
8. **Test notturni**: batterie di casi (casuali e/o dati della scuola) in tre fasi, subito, a un'ora o ogni settimana;
   si confrontano due prove tra loro.
