> Strumento storico usato per il precedente blocco del loader. La revisione
> 3.8 registra direttamente dall'eboot: usare la guida
> [DIAGNOSTICA-v3.8.md](../DIAGNOSTICA-v3.8.md). Il payload descritto sotto non
> è necessario per l'aggiornamento corrente e non viene consegnato come
> passaggio da eseguire.

# Diagnostica opzionale dell’avvio di PPSA74207

Questo piccolo payload tenta **una sola volta** l’avvio di **PPSA74207 già
registrato** e aggiunge il risultato a:

```text
/data/homebrew/PPSA74207/logs/launch-report.log
```

Registra il valore restituito da `sceSystemServiceLaunchApp` sia in esadecimale
sia come intero con segno, e richiede uno snapshot del registro kernel prima e
uno dopo il tentativo. Non è una nuova versione di `eboot.bin`.

## Uso

1. Chiudere completamente eventuali giochi e lasciare attivo ShadowMountPlus.
2. Inviare **`launch-probe-PPSA74207.elf`** al normale loader ELF già utilizzato
   sulla console, con il proprio programma e i propri IP e porta. Inviare il
   file una sola volta. Non copiarlo al posto di `eboot.bin`.
3. Attendere il risultato del tentativo e qualche secondo per la raccolta.
   Recuperare `launch-report.log` tramite il collegamento FTP già configurato.

Il collector crea soltanto la cartella di destinazione, `logs` e il report se
mancano. Il report è in **append**: le esecuzioni precedenti restano presenti.
Le due directory, solo quando vengono create da questa esecuzione, ricevono
permessi `0777` compatibili con le cartelle scrivibili del pacchetto, anche
se la normale umask restringe i permessi iniziali. I permessi di directory e
file già esistenti non vengono modificati. Un errore di preparazione o
scrittura del report impedisce il tentativo di avvio.
Questo percorso rimane lo stesso anche se la registrazione punta a un’immagine:
il report è esterno all’immagine e non richiede di scrivere dentro `/app0`.

## Registro kernel e limiti

La lettura usa esclusivamente l’operazione di ShadowMountPlus
`POST http://127.0.0.1:10101/api/v1/kernel-log`, già presente nei log della
console esaminata. È una richiesta **locale alla PS5**; non abilita accessi
dal PC e non cambia la configurazione del servizio. Ciascuna richiesta ha un
timeout di cinque secondi e richiede al massimo gli ultimi **128 KiB**. Le due
risposte JSON sono conservate integralmente, con i messaggi del kernel nel
campo `content` e gli eventuali errori nel campo `error`.

Se il servizio non è disponibile, la risposta è incompleta o l’API rifiuta la
lettura, il report indica **`KERNEL_UNAVAILABLE` / `KERNEL_CAPTURE=INCOMPLETE`**.
Il collector conserva comunque il ritorno effettivo della richiesta di avvio.
Non introduce elevazioni di privilegi o fallback che simulino una lettura
riuscita. I messaggi del registro possono includere altri processi attivi:
prima di condividere il report, controllarne il contenuto.

**`LAUNCH_REQUEST=ACCEPTED` significa soltanto che l’API ha accettato la
richiesta.** Non dimostra che il CRT, il renderer o il gameplay funzionino.
Un marker `[PPSA74207] CRT: _start entered` nel campo `content` è invece una
prova positiva dell’ingresso nel CRT. Il collector non calcola l’hash
dell’eseguibile e non identifica da solo la revisione effettivamente eseguita.
L’assenza di marker in un registro limitato non basta a provarne il mancato
avvio. Se manca anche `launch-report.log`, non è accertato che il collector
sia stato eseguito o abbia potuto scrivere: non è un risultato sul gioco.

Il codice non installa o disinstalla titoli, non monta o smonta sorgenti, non
modifica permessi esistenti, jailbreak, kstuff, configurazioni, `userdata`,
salvataggi o `gamedata`. La richiesta di avvio attiva naturalmente il normale
percorso del loader già configurato.

## Compilazione separata

Usare il PS5 Payload SDK già installato:

```sh
PS5_PAYLOAD_SDK=/percorso/del/ps5-payload-sdk \
  bash prospero/diagnostics/build.sh
```

Il risultato è `out/prospero-diagnostics/launch-probe-PPSA74207.elf`.
Lo script non richiama Ninja, non ricompila il gioco, non modifica il SDK e
non invia alcun file alla console. Richiede solo le librerie SDK
`SceSystemService` e `SceUserService`, oltre al runtime standard del payload.

Il codice di ritorno del collector è `0` per richiesta accettata con entrambe
le letture kernel disponibili, `1` per richiesta rifiutata, `2` per errore
del report, `3` per richiesta accettata con raccolta kernel incompleta e `4`
se non riesce a ottenere un utente attivo. Un errore del report prima della
richiesta o un utente non valido impediscono il tentativo di avvio.

## Verifica host

```sh
python3 prospero/diagnostics/test_launch_probe.py
```

I sette test compilano il codice C effettivo su Linux, sostituiscono le sole
API native di avvio/utente e usano un server HTTP locale temporaneo. Coprono
il titolo e l’ABI della singola richiesta, il valore negativo restituito,
gli errori o le risposte kernel incomplete, l’utente mancante, l’append del
report, il rifiuto dei symlink e i permessi delle directory nuove/esistenti.
Il server di test richiede che la porta locale `10101` sia libera.

ABI della chiamata derivata da
[`tools/controllers/launch.c` di Sonic](https://github.com/Nazky/UnleashedRecomp-Prospero/blob/a63abf2db139b04da90989024da5bc804d11323a/tools/controllers/launch.c).
L’API kernel segue
[`sm_mdbg.c` di ShadowMountPlus 1.7beta4](https://github.com/drakmor/ShadowMountPlus/blob/1.7beta4/src/sm_mdbg.c).
La compilazione e i test host non costituiscono una prova di esecuzione su PS5.
