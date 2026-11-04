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
```

Se la password contiene caratteri speciali (`@ : / # ? %`) vanno scritti in forma `%40`, `%3A`… nell'indirizzo; più semplice
scegliere una password di sole lettere e cifre. Poi renderlo leggibile solo dall'amministratore:

```bash
sudo chmod 600 /etc/orario-sito.env
```

- `SITO_LAVORO` (facoltativa): cartella temporanea per il solver compilato e per i file di una prova mentre gira.
  Default `/dev/shm/orario-sito`, cioè in RAM: sulla SD non si scrive nulla. Si svuota da sola a ogni riavvio.
- `SITO_PARALLELI`: casi in parallelo nei test notturni (il Pi 5 ha 4 processori: 3 lascia libero il sito).

### 5. Primo utente

```bash
cd ~/orario && ~/orario-venv/bin/python -m sito crea-utente admin --admin
```

Chiede una password (almeno 8 caratteri). Entrando come `admin` si creano da «amministrazione» le scuole e gli altri
utenti: ogni docente ha il suo nome e la sua password e vede solo le scuole che gli abbini; un amministratore le vede
tutte. Chi ha una sola scuola va dritto ai suoi dati, gli altri la scelgono dopo l'accesso. Dati, prove e versioni
salvate sono separati per scuola.

### 5b. Niente scritture sulla SD

Il sito non scrive file sul Pi: la chiave dei cookie, i dati, le prove, la ricerca passo passo compressa e le uscite del
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
- La ricerca passo passo (`-b --passi N`) finisce in `orario.txt.gz`, compresso: pochi byte per passo, e la registrazione
  si ferma da sola dopo N passi, quindi un file pesa al massimo qualche MB anche se la ricerca va avanti per ore.
- "Rifai identico dal JSON" rilancia un caso con esattamente gli stessi dati e lo stesso ordine della coda dei docenti:
  la ricerca è identica.
- Le prove vecchie si cancellano dalla loro pagina (con i loro file nel database), così il database sul Mac non cresce
  inutilmente.
- Il database cresce con le prove: controllare ogni tanto lo spazio del Mac (la pagina «Test notturni» mostra la
  dimensione del database).

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
altrimenti sito e prove notturne si fermano.

## La ricerca passo passo

Nel test in tempo reale, spuntando «Registra la ricerca passo passo» il solver scrive l'orario parziale a ogni passo
della computazione, fino al numero di passi scelto nel campo accanto (predefinito 100.000; oltre, la ricerca continua
senza registrare). Lo stesso fa la fase 3 dei test notturni per i casi non finiti. Nella pagina del caso compare la sezione
«La ricerca passo passo»: uno slider dal passo 0 (orario vuoto) all'ultimo passo registrato, con i pulsanti ±1, ±100 e
±1000. Lo slot ✕ è quello che la ricerca sta per decidere. La stessa sezione compare già nella pagina della prova mentre la
ricerca gira, con i passi registrati fino a quel momento (lo slider si allunga da solo).

Il pulsante «▶ Avvia l'animazione» fa scorrere i passi in modo fluido, dal passo in cui sei (o dall'inizio, se sei in
fondo); il cursore «Velocità» la regola da 1 a 2.500 passi al secondo, anche mentre gira. «⏸ Pausa» (o qualunque comando
a mano) la ferma e mostra lo stato esatto di quel passo. I passi si scaricano a blocchi di sole differenze, quindi anche
l'animazione veloce non pesa sul Raspberry.

Passando il mouse su una cella (o toccandola, su iPad e telefono) si apre la finestra con la coda dei docenti di quello
slot. Per ogni docente c'è l'esito di tutti i vincoli, uno per riga: libero, già entrato, massimo di ore, ore attaccate,
massimo di classi al giorno, codocente, regola del sabato (✔ rispettato, ✘ non rispettato, – non si applica). Il
pulsante in cima nasconde o mostra queste frasi. La finestra si scorre con la rotella o con il dito; con il mouse resta
aperta se ci porti sopra il puntatore, e si chiude con ✕ o toccando fuori. Un passo in cui non c'era nessun docente
utilizzabile è segnalato, con i motivi. I «blocchi» sono solo le vere marce indietro: i vicoli ciechi e i rami scartati
dalla potatura; l'ora buca, in cui la ricerca prosegue con lo slot vuoto, si vede nella riga di stato ma non è un blocco.
I pulsanti sotto la barra saltano da un blocco all'altro e durante l'animazione la pagina scorre per tenere in vista la
cella che si sta decidendo. Nel passo di un ramo scartato la finestra mostra le verifiche di potatura, una riga ciascuna
(lezioni della classe, ore dei docenti), con il dettaglio di quella fallita: la causa è la scelta fatta nello slot prima,
evidenziato nell'orario, non i docenti dello slot in esame, la cui coda non è mai stata provata.

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
   - *Dati casuali*: seme, numero di classi, docenti in ritardo;
   - *Da un file JSON*: un file esportato dai dati della scuola (anche di un'altra scuola) o il `risultato.json` di una
     prova; la prova usa quei dati senza toccare quelli della scuola.
3. **Ricerca**: tempo massimo, quanti orari trovare, registrazione passo passo (con il numero di passi). Sotto «Parametri
   avanzati»: ore obbligatorie al giorno (5), ore massime di un docente nella stessa classe al giorno (3), classi diverse
   per docente al giorno (3) e l'ordine delle classi (un seed). I motivi di scarto dei docenti si registrano sempre.
4. **Avvia**: la pagina mostra progresso e mappa dei blocchi; alla fine si apre il caso con esito, diagnosi, orario
   (anche per docente) e file da scaricare (dati, uscita, orari, ricerca passo passo `.gz`, JSON). «Rifai identico» ripete
   esattamente la stessa ricerca; «Rilancia in tempo reale» riporta le opzioni nel modulo.
5. **Capire dove si blocca**: nella pagina di un caso usa «La ricerca passo passo» (vedi sopra): lo slider, l'animazione, i
   blocchi e la finestra con la coda dei docenti e i vincoli.
6. **Casi dei test notturni da verificare**: in «Test in tempo reale» sotto il modulo c'è l'elenco degli ultimi casi non
   risolti dei notturni; «Verifica» li carica nel modulo (stessi dati e opzioni, anche per i dati della scuola) per rifarli
   a mano con altri parametri, tempo o ordine delle classi. Lo stesso pulsante «Verifica a mano» c'è nella pagina di ogni prova notturna.
7. **Test notturni**: batterie di casi (casuali, dati della scuola e/o i dati di un file JSON caricato) in tre fasi,
   subito, a un'ora o ogni settimana (per le prove ricorrenti i dati del file restano nella pianificazione); si
   confrontano due prove tra loro. Ogni scuola nasce con una prova ricorrente predefinita, «Notturna»: ogni notte alle
   22, i semi 1–20 con 8 classi (normali e con ritardi) più i dati della scuola, fasi 60 / 600 / 600 s, fase 3 passo
   passo. Si può sospendere, cambiare o cancellare: non viene ricreata. Mentre una prova gira, la sua pagina mostra i
   casi in esecuzione (fase, tempo usato, nodi, marce indietro, rami scartati, slot) aggiornati ogni 2 secondi.
