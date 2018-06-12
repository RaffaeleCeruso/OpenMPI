# Progetto PCPC

**Studente:** Ceruso Raffaele

**Matricola:** 05225-----

**Problema assegnato:** nbody, modalità di comunicazione point-to-point, test su macchine m4.xlarge di amazon (dimensione massima del cluster: 8 nodi)

**Dati di input dei test:** 10000 particelle, DT=0.01, iterazioni totali = 10


Nella fisica, il problema &quot;nbody&quot; consiste nel prevedere i moti individuali di un gruppo di oggetti celesti che sono soggetti all'attrazione gravitazionale. Per i nostri scopi, ci siamo limitati al calcolare le posizioni dei corpi al passare del tempo, secondo una determinata legge fisica. L'utilizzo della programmazione parallela ci ha permesso di poter velocizzare l'esecuzione di un programma che calcola tali posizioni, con risultati soddisfacenti in termini di tempo.

Il programma sviluppato è in grado di equidistribuire un qualsiasi numero di particelle tra i processi MPI scelti, anche nel caso in cui il numero delle particelle non è divisibile per il numero dei processi stessi. Sono stati calcolati i tempi di esecuzione del programma  prima della computazione delle posizioni delle particelle da parte di ogni processo MPI fino alla conclusione di tutte le comunicazioni tra i vari processi MPI.

Infatti all'interno di ogni iterazione ogni processo MPI, che sia slave o master, computa i nuovi dati relativi al suo insieme di particelle e li invia al master, attraverso la modalità di comunicazione point-to-point (naturalmente il master non ha bisogno di inviare tali valori). Il master ricevute le informazioni aggiorna le posizioni di tutte le particelle ed è pronto ad inviarli nuovamente a tutti gli slave per l'iterazione successiva . 



![](C:..\PCPCProject\StrongScalability.png)

Strong Scalability

Sull'asse delle y abbiamo il tempo in millisecondi, mentre sull'asse delle x abbiamo il numero di processi MPI. I test sono stati soddisfacenti, mostrando una buona scalabilità del codice. Infatti il tempo di esecuzione è passato da circa 1700 millisecondi utilizzando un solo processo fino ad arrivare a circa 130 millisecondi con 32 processi . Unica nota da rilevare, quando il numero di processi mpi è pari a 17, il tempo di esecuzione peggiora. Questo può essere dovuto a problemi hyper-threading dei processori Intel di amazon.







![Scalability](C:..\PCPCProject\WeakScalability.png)

Weak Scalability

Sull'asse delle y abbiamo il tempo in secondi, mentre sull'asse delle x abbiamo il numero di processi mpi. In questo tipo di test ci aspettiamo una linea orizzontale, assicurandoci che aumentando la taglia dell'input e il numero di processi mpi, in modo costante, il tempo del programma rimanga invariato. Utilizzando lo scambio di messaggi previsti da mpi, ovviamente, il tempo non può essere costante perché spendiamo tempo per scambiare dati tra i processi, ma comunque il tempo segue una crescita lineare, mostrando quello che ci aspettavamo, cioè una buona gestione del carico.





**Compilazione**

Il programma sorgente è stato scritto utilizzando le librerie standard di c, quindi in fase di compilazione basta eseguire il secondo comando con l'inserimento di -lm per la libreria Math.h:

_mpicc esameCeruso.c –o (nome eseguibile) -lm_ 

Il file sorgente è stato compilato e eseguito correttamente sulle macchine di amazon.





**Esecuzione**

Supponendo che il programma compilato sia nominato &quot;eseguibile&quot;, è possibile lanciare il programma attraverso una serie di parametri:

_mpirun –np X –host Y eseguibile nParticels iteration stampa_

- dove X è il numero di processi mpi che si desidera lanciare.
- Y è la lista degli indirizzi IP interni al network di amazon delle macchine appartenenti al cluster.

**nParticels iteration e stampa** sono tre parametri opzionali, se usati, l'ordine non può essere invertito. Inoltre si possono omettere i parametri partendo sempre da quello più a sinistra. 

1. **nParticels** indica il numero di particelle totali

2. **iteration** indica il numero di iterazioni

3. **stampa** è un flag intero che settato ad 1 stampa a video le relative informazioni di ogni particella, altrimenti 0 per evitare quest'ultime. Di default **stampa** vale 0.

   
