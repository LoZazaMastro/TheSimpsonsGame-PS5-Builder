# Runtime per PS5

## Heap, contesti e diagnostica — revisione 3.8

`heap_prospero.c` è la copia attribuita di `platform/src/heap.c` del pin SDK
`b5efad528e0ac1b8289f39d72a0e890ff68ff0a6`. L'originale ha SHA-256
`286575e91e5eb8e0b00e1d10d6df395a0c3bbf322ac21bf5dbc6540c1daa9c2c`.
Le sole differenze funzionali sono le dichiarazioni atomiche dei tre
contatori campionati dal worker diagnostico: segmenti, byte mappati e picco.
Il lock della tabella, l'algoritmo di allocazione, le arene, i footer e la
proprietà dei blocchi rimangono quelli dell'SDK. Il campione è approssimativo,
senza un lock aggiuntivo nel lettore. Il runner dell'allocatore verifica il
confronto reversibile con l'originale e le letture concorrenti.

`heap_wrap_prospero.c` conserva il corpo delle dodici API del wrapper SDK,
con soli nomi/linkage interni adattati e record prima/dopo le chiamate.
Il file `dlmalloc/SOURCE` documenta la modifica limitata della guardia
smallbin e il provider locale del lock. Il link usa oggetti espliciti per
`heap_prospero.c`, `heap_wrap_prospero.c` e `mspace.c`, verificando l'assenza
dei provider concorrenti nell'archivio SDK. Il codice SDK conserva copyright
Mihawk e GPL-3.0-or-later; il testo integrale è in
[LICENSE-SDK-GPL-3.0.txt](LICENSE-SDK-GPL-3.0.txt).

I contesti Fiber PS5 sono implementati in
`tools/rexglue-sdk/src/core/fiber_prospero.cpp`, con salvataggio esplicito dei
registri ABI, MXCSR/x87 e stack diretto con guardie. Il guest usa direttamente
l'oggetto assembly `_setjmp.o` estratto dalla libc del SDK abbinato. Gli
import setjmp/longjmp interni al driver RADV Sonic rimangono separati.

Il recorder, i checkpoint e le statistiche sono nel core ReXGlue. Gli eventi
di allocazione vanno in memoria statica; metriche e snapshot audio sono
scritti dal thread di monitoraggio. La destinazione del crash viene aperta
dal CRT prima dei costruttori. Contratti, frequenze e limiti sono descritti
in [DIAGNOSTICA-v3.8.md](../DIAGNOSTICA-v3.8.md); i test e i report della 3.8
sono in `prospero/tests` e `prospero/validation`.

Le sezioni seguenti documentano i componenti mantenuti dalle revisioni
precedenti. Le prove storiche conservano gli hash della versione a cui si
riferiscono; i report attuali specificano quali sorgenti sono stati rieseguiti.

## Provenienza del runtime TLS

`emutls.c` e i quattro header `int_*.h` provengono da
[LLVM compiler-rt 18.1.8](https://github.com/llvm/llvm-project/tree/llvmorg-18.1.8/compiler-rt/lib/builtins).
`upstream.json` registra il tag, i percorsi e gli hash Git dei file originali.
Gli header, `LICENSE.TXT` e `CREDITS.TXT` sono copie integrali; le modifiche locali
a `emutls.c` sono contrassegnate nei commenti. Si applica la licenza Apache 2.0
con LLVM Exceptions inclusa in questa directory.

## Perché serve un oggetto compilato per la console

Il gioco e le librerie PS5 usano `-femulated-tls`. L'archivio dei builtins della
distribuzione LLVM Linux contiene anche una propria implementazione `emutls.o`,
che può soddisfare il link ma usa i tipi pthread di Linux. Nel binario v2 erano
presenti un `pthread_once_t` di 4 byte e un mutex di 40 byte. L'SDK PS5 fissato a
`b5efad528e0ac1b8289f39d72a0e890ff68ff0a6` richiede rispettivamente 16 e 8 byte;
il campo mutex della struttura once è all'offset 8. Queste differenze rendono
il runtime host incompatibile, anche quando il linker non rileva errori.

CMake compila questo file C con gli header PS5 e lo passa al linker come
oggetto esplicito prima degli archivi. La definizione forte di
`__emutls_get_address` impedisce l'estrazione del membro Linux. Il simbolo è
hidden; `-fno-visibility-from-dllstorageclass` è limitato a questo file, perché
la politica predefinita del target Sony altrimenti sovrascrive l'attributo di
visibilità. `-fno-plt` mantiene le chiamate esterne tramite GOT. Gli altri
builtins aritmetici rimangono nell'archivio del compilatore.

## Modifiche alla gestione dei distruttori

L'algoritmo di allocazione e indicizzazione è quello LLVM. La variante PS5:

- verifica staticamente dimensioni e offset dei tipi pthread del target;
- termina in modo esplicito se `pthread_setspecific` fallisce;
- durante la distruzione del thread ripubblica temporaneamente il suo array
  TLS, chiama `ps5p_run_thread_destructors()` del corrente `libps5platform`,
  recupera nuovamente l'array e solo allora libera gli oggetti;
- svuota la chiave pthread prima di liberare la memoria, evitando ulteriori
  richiami su memoria già liberata.

Il recupero dell'array dopo i distruttori è necessario: un distruttore può
accedere a una variabile TLS nuova e provocare `realloc`. Il registro C++ conserva l'algoritmo del pin SDK; la revisione 3.7
usa `cxa_prospero.c` con pubblicazione atomica del flag ready e un unico provider
`__cxa_thread_atexit_impl`. L'hook interno è dichiarato con la stessa firma
usata in `platform/src/threads.c`; un aggiornamento dell'SDK deve ricontrollarla.

Gli oggetti del percorso di allocazione nell'SDK fissato (`heap.o`,
`heap_wrap.o`, `mspace.o`, `placement.o` e `threads.o`) non referenziano
`__emutls_get_address`: la cache degli heap per thread usa direttamente chiavi
pthread. Il mutex emutls viene rilasciato prima delle allocazioni. Non è emerso
un ciclo statico tra emutls e i wrapper malloc; le implementazioni native del
sistema e il comportamento sulla console restano da verificare sul dispositivo.

## Verifiche mirate

Dalla radice del repository, con LLVM 18 nel `PATH`:

```sh
python3 prospero/tests/run_emutls_host.py --sdk-source /percorso/PS5-PayloadSDK
python3 prospero/tests/run_link_smoke.py
```

Il primo comando compila il runtime di produzione e `cxa_prospero.c` con
pthread host, AddressSanitizer e UndefinedBehaviorSanitizer. `--registry-source`
consente di selezionare esplicitamente la baseline SDK per un confronto.
Verifica otto thread concorrenti, valori iniziali e zero, allineamenti fino a
4096 byte, crescita di 256 controlli durante la distruzione e distruttori
annidati. Ripete la prova in tre processi con ordine di creazione delle chiavi
opposto e svuotamento esplicito del registro, come nei wrapper PS5.

Se il contenitore impedisce a LeakSanitizer di enumerare `/proc/.../task`,
`--no-leak-check` disabilita soltanto quel controllo e lo dichiara nell'output.
AddressSanitizer e UndefinedBehaviorSanitizer rimangono attivi.

Il secondo comando richiede le variabili `PS5_PAYLOAD_SDK`,
`SONIC_PS5_REFERENCE` e l'archivio RADV già costruito. Collega un piccolo titolo
con libc++, unwind, RADV e questo oggetto TLS esplicito; controlla le dimensioni
pthread nel risultato finale, l'unicità del runtime e l'assenza di chiamate PLT.
È una verifica di compilazione e link: non esegue il binario PS5 e non prova
che CE-107694-7 sia risolto.

## Stack dei thread in memoria diretta

`threads_prospero.c` deriva da
[`platform/src/threads.c` dell'SDK PS5](https://github.com/mihawk-99/PS5_PayloadSDK/blob/b5efad528e0ac1b8289f39d72a0e890ff68ff0a6/platform/src/threads.c),
al commit `b5efad528e0ac1b8289f39d72a0e890ff68ff0a6`.
[`placement.h`](https://github.com/mihawk-99/PS5_PayloadSDK/blob/b5efad528e0ac1b8289f39d72a0e890ff68ff0a6/platform/src/placement.h)
è una copia integrale dello stesso commit. Entrambi conservano il copyright
di Mihawk e la licenza **GPL-3.0-or-later** dichiarata nei file; la licenza LLVM
descritta sopra riguarda i file TLS e non sostituisce quella del codice SDK.
`placement.c` resta nell'archivio SDK. Il registro dei distruttori è la
copia locale attribuita `cxa_prospero.c`, con sola pubblicazione atomica del
flag ready; la copia sorgente/binaria dell'SDK resta invariata.

La versione SDK usa memoria diretta soltanto per richieste inferiori a 2 MiB.
La variante locale conserva le richieste ReXGlue di 4 e 16 MiB e assegna anche
questi stack in memoria diretta. Solo il backing fisico viene arrotondato a
64 KiB; la dimensione utile richiesta resta identica. Lo stack esplicito del
chiamante rimane di sua proprietà, gli attributi pubblici vengono conservati
e la guardia rimane non mappata. Se la memoria diretta non è disponibile,
la creazione nativa riceve la stessa dimensione utile e può restituire un errore.

La cache riutilizza soltanto stack con backing e guardia compatibili e trattiene
al massimo 32 stack e 64 MiB di backing. Gli stack ancora in uso conservano la
dimensione richiesta e non rientrano in quel limite della cache. MXCSR, distruttori
C++, `pthread_exit` e il reaper degli stack detached seguono il percorso SDK.
Il join conserva l'identità del record iniziale: se il sistema riutilizza un
`pthread_t` dopo il join nativo, il cleanup non può prelevare il record del nuovo
thread. Un join fallito nel reaper trattiene l'allocazione fino all'uscita del
processo, senza smappare uno stack di cui non è provata la terminazione.

CMake passa il file C come oggetto esplicito prima degli archivi SDK. Le cinque
definizioni pubbliche del wrapper impediscono di estrarre anche il membro
`threads.o` originale. Anche `cxa.o` viene sostituito dal provider locale
compatibile, verificato nel link; gli altri componenti SDK restano attivi.

Dalla radice del repository, con GCC e pthread Linux disponibili:

```sh
python3 prospero/tests/run_threads_prospero_host.py \
  --sdk-root /percorso/PS5-PayloadSDK --cc gcc
```

Il runner compila i wrapper locale e originale con il modello kernel dell'SDK,
`placement.c` e `cxa.c` reali. I 23 processi verificano le dimensioni 2/4/16 MiB,
un frame effettivo di 3 MiB negli stack da 4/16 MiB, guardia, attributi, riuso e
limiti della cache, distruttori, join/detach e i fallimenti dei passaggi nativi.
La regressione di riuso ID inserisce un nuovo thread fra join nativo e cleanup,
e intercetta il confronto degli ID soltanto nel test: l'originale libera il
record sbagliato, la variante locale conserva entrambi i record corretti.
La riga `FAIL` relativa a questo originale nella capture è il fallimento atteso
che dimostra la regressione; il runner completo termina con codice 0.

[Rapporto e comandi esatti](../validation/thread-stacks-v3.4.json) e
[stdout/stderr della prova finale](../validation/thread-stacks-v3.4-host.txt)
registrano gli hash dei sorgenti verificati e dell'oggetto compilato per PS5.
I test host eseguono pthread Linux su mapping reali del modello SDK: non provano
il budget di memoria diretta, gli export delle API o lo scheduler della console.
Questa modifica non dimostra una correzione dei testi mancanti, del frame rate
o dell'abort osservato durante il caricamento delle pipeline.

## Pubblicazione del registro TLS — revisione 3.7

`cxa_prospero.c` deriva dal `platform/src/cxa.c` del pin SDK b5efad5, con
copyright Mihawk e GPL-3.0-or-later conservati. Il solo cambiamento funzionale
è un `atomic_bool` sempre lock-free: store release dopo inizializzazione
della chiave/atexit e load acquire nei controlli del flag. ThreadSanitizer
riproduce la race originale con il drain di un thread che non ha registrato
distruttori. Il provider locale passa TSAN, prove di distruttori annidati,
atexit, tre ordini emutls ASan/UBSan e compilazione PS5 con ABI e simboli C
confrontati. CMake lo passa come oggetto esplicito; `libps5platform.a(cxa.o)`
non deve essere estratto. Evidenza: `validation/cxa-publication-full-audit.json`
e `validation/target-runtime-v3.7.json` (percorsi relativi a `prospero`).
