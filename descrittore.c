#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "newsemaphore.h"
#include "newmetodo.h"

typedef struct individuo{
	char tipo;
    char nome[100];
    unsigned long genoma;
	int sem_id;
	int shm_id;
	int qab_id;
	int qba_id;
	int qagest_id;
	int qbgest_id;
	int qconf_id;
	int qnascite_id;
	int init_people;
}individuo_t;

typedef struct dati_finali{
	int processiA;
	int processiB;
	int processiViventi;
	pid_t maxnPid;	
	char maxnTipo;
	char maxnNome[100];
	int maxnGenoma;
	pid_t maxgPid;
	char maxgTipo;
	char maxgNome[100];
	int maxgGenoma;
	int tempoScaduto;
}dati_finali_t;

typedef struct mymsg{		//struct per l'utilizzo della coda per i dati
	long type;
	pid_t pid;
	char tipo;
	char nome[100];
	unsigned long genoma;
}mymsg_t;

typedef struct myconf{		//struct per l'utilizzo della coda per la conferma
	long type;
	char risp;
}myconf_t;

typedef struct msgnascite{	//coda per contenere tutte le nascite
	long type;
	pid_t pid;
}msgnascite_t;

individuo_t set_individuo(char* argomento[], individuo_t individuo);

//--------------------------------------------------------------MAIN-----------------------------------------------------------
int main(int argc, char *argv[]){
	//ignoro SIGTERM
	sigset_t newmask_sigterm, oldmask_sigterm;
	sigemptyset(&newmask_sigterm);	//insieme vuoto
	sigaddset(&newmask_sigterm, SIGTERM);	//aggiunge SIGTERM alla maschera
	//da qui bisogna ignorare SIGTERM
	if(sigprocmask(SIG_BLOCK, &newmask_sigterm, &oldmask_sigterm)==-1){	//setta la nuova maschera e memorizza la vecchia
		perror("ERRORE SIGPROCMASK settaggio");
		exit(-1);
	}
	
	int sem_stat;
	msgnascite_t qnascite;
	individuo_t processo;
	dati_finali_t *datiFinali;

	processo=set_individuo(&argv[0],processo);
	
	printf("PID: %d, TIPO: %c, NOME: %s, GENOMA: %li\n", getpid(), processo.tipo, processo.nome, processo.genoma);
	
	//lo metto nella coda dei nati
	qnascite.type=getpid();
	qnascite.pid=getpid();
	if(msgsnd(processo.qnascite_id, &qnascite, (sizeof(qnascite)-sizeof(long)), 0)==-1){
		perror("ERRORE msgsnd qnascite");
		exit(-1);
	}
	//aggiornare la SHARED MEMORY
	datiFinali=(dati_finali_t*)shmat(processo.shm_id, NULL, 0);//attacca la SM alla memoria virtuale del processo chiamante
	
	if(processo.tipo=='A')
		datiFinali->processiA++;
	else
		datiFinali->processiB++;
	datiFinali->processiViventi++;
	//controlliamo se è l'ultimo figlio che deve nascere, se lo è liberiamo il semaforo 1
	if(datiFinali->processiViventi == processo.init_people){
		sem_stat=releaseSem(processo.sem_id, 1);
		if(sem_stat==-1){
			perror("ERRORE releaseSem 1");
		}
	}
		
	if(strlen(processo.nome)>strlen(datiFinali->maxnNome)){
		datiFinali->maxnPid=getpid();
		datiFinali->maxnTipo=processo.tipo;
		int x;
		for(x=0; processo.nome[x]!='\0'; x++)
			datiFinali->maxnNome[x]=processo.nome[x];
		datiFinali->maxnNome[x]='\0';
		datiFinali->maxnGenoma=processo.genoma;
	}
	if(processo.genoma>datiFinali->maxgGenoma){
		datiFinali->maxgPid=getpid();
		datiFinali->maxgTipo=processo.tipo;
		int x;
		for(x=0; processo.nome[x]!='\0'; x++)
			datiFinali->maxgNome[x]=processo.nome[x];
		datiFinali->maxgNome[x]='\0';
		datiFinali->maxgGenoma=processo.genoma;
	}
	
	//rilasciare il semaforo
	sem_stat=releaseSem(processo.sem_id, 0);
	if(sem_stat==-1){
		perror("ERRORE releaseSem 0");
		exit(-1);
	}
	
	//ora sblocco SIGTERM
	if(sigprocmask(SIG_SETMASK, &oldmask_sigterm, NULL)==-1){	//reset della maschera
		perror("ERRORE SIGPROCMASK reset");
		exit(-1);
	}
	
	
	
//---------------------------------------------------vita dei processi-----------------------------------------------------------
	mymsg_t qab, qba, qagest, qbgest;
	myconf_t qconf;
	//nel processo A qba è il richiedente per il processo in esecuzione
	//occupa e rilascia subito il semaforo perchè così "da il via" quando sono stati tutti creati
	if(processo.init_people!=1){	//non viene eseguito per newtwins xk non deve aspettare nessuno e newlife per terminazione
		sem_stat=reserveSem(processo.sem_id, 1);	//resta qui bloccato finchè tutti i fratelli sono stati creati
		if(sem_stat==-1){
			perror("ERRORE reserveSem 1");
			exit(-1);
		}
		sem_stat=releaseSem(processo.sem_id, 1);
		if(sem_stat==-1){
			perror("ERRORE releaseSem 1");
			exit(-1);
		}
	}
	
//-----------------------------------------------------------processi A-------------------------------------------------------
	if(processo.tipo=='A'){		//rimane in attesa di essere contattato da un processo B.
		//ignoro SIGTERM
		sigset_t newmask_sigterm, oldmask_sigterm;
		sigemptyset(&newmask_sigterm);	//insieme vuoto
		sigaddset(&newmask_sigterm, SIGTERM);	//aggiunge SIGTERM alla maschera
		//da qui bisogna ignorare SIGTERM
		if(sigprocmask(SIG_BLOCK, &newmask_sigterm, &oldmask_sigterm)==-1){	//setta la nuova maschera e memorizza la vecchia			
			perror("ERRORE SIGPROCMASK settaggio");
			exit(-1);
		}
		
		//scrittura sulla coda AB ( A pubblica le proprie info per farle vedere ai B)
		//USIAMO IL PID COME 'INDICE DI RICERCA'
		qab.type=getpid();
		qab.pid=getpid();
		qab.tipo=processo.tipo;
		strcpy(qab.nome, processo.nome);
		qab.genoma=processo.genoma;
		if(msgsnd(processo.qab_id, &qab, (sizeof(qab)-sizeof(long)), 0)==-1){
			if(datiFinali->tempoScaduto!=1)
				perror("ERRORE msgsnd processo A qab");
			exit(-1);
			
		}
		
		int accettato=0;
		while(!accettato){
			//da qui bisogna ignorare SIGTERM (serve a reimpostarlo quando viene tolto alla fine del ciclo per eventuali segnali pendenti)
			if(sigprocmask(SIG_BLOCK, &newmask_sigterm, &oldmask_sigterm)==-1){	//setta la nuova maschera e memorizza la vecchia
				perror("ERRORE SIGPROCMASK settaggio");
				exit(-1);
			}
			//attende di ricevere il messaggio (dalla coda qba) con i dati di chi gli vuole chiedere l'accoppiamento
			if(msgrcv(processo.qba_id, &qba, (sizeof(qba)-sizeof(long)), getpid(), 0)==-1){
				if(datiFinali->tempoScaduto!=1)
					perror("ERRORE msgrcv processo A qba");
				exit(-1);
			}
			
			printf("RICHIEDENTE LETTO DALLA CODA> pid: %d, tipo: %c, nome: %s, genoma: %li\n", qba.pid, qba.tipo, qba.nome, qba.genoma);
			//ora il processo A ha ricevuto le caratteristiche del B
			//DEVE DECIDERE SE ACCETTARE LA PROPOSTA O RIFIUTARLA
				//-se il processo B ha un genoma multiplo del processo A, il processo A acconsente sempre
			if((qba.genoma%processo.genoma)==0){
				//A accetta
				accettato=1;
				qagest.type=getpid();
				qagest.pid=getpid();
				qagest.tipo=processo.tipo;
				strcpy(qagest.nome,processo.nome);
				qagest.genoma=processo.genoma;
				//comunica al gestore che ha trovato un B che gli va bene 
				if(msgsnd(processo.qagest_id, &qagest, (sizeof(qagest)-sizeof(long)), 0)==-1){
					if(datiFinali->tempoScaduto!=1)
						perror("ERRORE msgsnd qagest");
					exit(-1);
				}
				//comunica al processo B che ha accettato la sua proposta
				qconf.type=qba.pid;		//gli rimanda il pid del processo B
				qconf.risp='s';
				if(msgsnd(processo.qconf_id, &qconf, (sizeof(qconf)-sizeof(long)), 0)==-1){
					if(datiFinali->tempoScaduto!=1)
						perror("ERRORE msgsnd qconf");
					exit(-1);
				}
			
			}else{
				//-altrimenti effettua la propria decisione per massimizzare l'MCD tra il proprio genoma e quello del processo che lo ha contattato
				int max_div, maxcd;
				maxcd=mcd(qba.genoma, processo.genoma);
				max_div=maxdiv(processo.genoma);
				if(maxcd==max_div){
					accettato=1;
					qagest.type=getpid();
					qagest.pid=getpid();
					qagest.tipo=processo.tipo;
					strcpy(qagest.nome,processo.nome);
					qagest.genoma=processo.genoma;
					//Prima di mandare i propri dati al gestore e terminare deve togliersi dalla coda dei vivi
					if(msgrcv(processo.qnascite_id, &qnascite, (sizeof(qnascite)-sizeof(long)), getpid(), 0)==-1){
						if(datiFinali->tempoScaduto!=1)
							perror("ERRORE msgrcv lettura coda dei vivi processo A");
						exit(-1);
					}	
					//comunica al gestore il pid del processo di cui ha accettato la proposta
					if(msgsnd(processo.qagest_id, &qagest, (sizeof(qagest)-sizeof(long)), 0)==-1){
						if(datiFinali->tempoScaduto!=1)
							perror("ERRORE msgsnd qagest");
						exit(-1);
					}
					//comunica al processo B che ha accettato la sua proposta
					qconf.type=qba.pid;
					qconf.risp='s';
					if(msgsnd(processo.qconf_id, &qconf, (sizeof(qconf)-sizeof(long)), 0)==-1){
						if(datiFinali->tempoScaduto!=1)
							perror("ERRORE msgsnd qconf");
						exit(-1);
					}						
				}else{
					//comunica al processo B che NON ha accettato la sua proposta
					qconf.type=qba.pid;
					qconf.risp='n';
					if(msgsnd(processo.qconf_id, &qconf, (sizeof(qconf)-sizeof(long)), 0)==-1){
						if(datiFinali->tempoScaduto!=1)
							perror("ERRORE msgsnd qconf");
						exit(-1);
					}
					if(sigprocmask(SIG_SETMASK, &oldmask_sigterm, NULL)==-1){	//reset della maschera
						perror("ERRORE SIGPROCMASK reset");
						exit(-1);
					}	
				}
			}
		}
		//il processo A si è accoppiato, si è tolto dalla coda dei vivi, è uscito dal ciclo, ora sblocca SIGTERM e termina
		if(sigprocmask(SIG_SETMASK, &oldmask_sigterm, NULL)==-1){	//reset della maschera
			perror("ERRORE SIGPROCMASK reset");
			exit(-1);
		}
	
//-------------------------------------------------------------processi B-------------------------------------------------------
	}else{//sono un processo B, devo contattare un processo A.
		//consulto le info dei processi A presenti e contatto quello che può darmi dei figli con genoma alto (MCD tra i due genomi)
		//ignoro SIGTERM 
		sigset_t newmask_sigterm, oldmask_sigterm;
		sigemptyset(&newmask_sigterm);	//insieme vuoto
		sigaddset(&newmask_sigterm, SIGTERM);	//aggiunge SIGTERM alla maschera
		int accettato=0;
		while(!accettato){
			//da qui bisogna ignorare SIGTERM
			if(sigprocmask(SIG_BLOCK, &newmask_sigterm, &oldmask_sigterm)==-1){	//setta la nuova maschera e memorizza la vecchia
				perror("ERRORE SIGPROCMASK settaggio");
				exit(-1);
			}
			if(msgrcv(processo.qab_id, &qab, (sizeof(qab)-sizeof(long)), 0, 0)==-1){
				if(datiFinali->tempoScaduto!=1)
					perror("ERRORE msgrcv processo B qab");
				exit(-1);
			}
			printf("STRUTTURA LETTA DALLA CODA> pid: %d, tipo: %c, nome: %s, genoma: %li\n", qab.pid, qab.tipo, qab.nome, qab.genoma);
			//sleep(2);		//aggiungere questo commento per rendere l'output leggibile
			
			//analizziamo il messaggio letto per vedere se fa al caso nostro, altrimenti rimettiamolo nella coda			
			if(processo.genoma>qab.genoma){
				//scrittura sulla coda delle proprie info per farle vedere al processo A che ha scelto
				qba.type=qab.pid;
				qba.pid=getpid();
				qba.tipo=processo.tipo;
				strcpy(qba.nome, processo.nome);
				qba.genoma=processo.genoma;
				if(msgsnd(processo.qba_id, &qba, (sizeof(qba)-sizeof(long)), 0)==-1){
					if(datiFinali->tempoScaduto!=1)
						perror("ERRORE msgsnd processo B qba");
					exit(-1);
				}	//gli ho inviato i miei dettagli per vedere se accetta
				//leggo la coda con la risposta, se ha accettato avviso il gestore e termino
				if(msgrcv(processo.qconf_id, &qconf, (sizeof(qconf)-sizeof(long)), getpid(), 0)==-1){
					if(datiFinali->tempoScaduto!=1)
						perror("ERROR msgrcv processo B qconf");
					exit(-1);
				}
				if(qconf.risp=='s'){		//HA ACCETTATO :)
					accettato=1;
					printf("QCONF.RISP=====%c\n", qconf.risp);
					//comunica al gestore che è stato accettato
					qbgest.type=qab.pid;
					qbgest.pid=getpid();
					qbgest.tipo=processo.tipo;
					strcpy(qbgest.nome, processo.nome);
					qbgest.genoma=processo.genoma;
					//Prima di mandare i propri dati al gestore e terminare deve togliersi dalla coda dei vivi
					if(msgrcv(processo.qnascite_id, &qnascite, (sizeof(qnascite)-sizeof(long)), getpid(), 0)==-1){
						if(datiFinali->tempoScaduto!=1)
							perror("ERRORE msgrcv lettura coda dei vivi processo B");
						exit(-1);
					}
		
					if(msgsnd(processo.qbgest_id, &qbgest, (sizeof(qbgest)-sizeof(long)), 0)==-1){
						if(datiFinali->tempoScaduto!=1)
							perror("ERRORE msgsnd processo B qbgest");
						exit(-1);
					}
					
				}else{						//NON HA ACCETTATO :(
				//SE NON HA ACCETTATO LO RIMETTO IN CODA qab (abbassa il genoma per favorire l'accoppiamento) E VADO AL PROSSIMO
					printf("QCONF.RISP=====%c\n", qconf.risp);
					qab.type=qab.pid;
					if(qab.genoma>2)
						qab.genoma--;	//abbassa il genoma per rendere più probabile l'accoppiamento
					if(msgsnd(processo.qab_id, &qab, (sizeof(qab)-sizeof(long)), 0)==-1){
						if(datiFinali->tempoScaduto!=1)
							perror("ERRORE msgsnd processo B qab");
						exit(-1);
					}
					if(sigprocmask(SIG_SETMASK, &oldmask_sigterm, NULL)==-1){	//reset della maschera
						perror("ERRORE SIGPROCMASK reset");
						exit(-1);
					}
				}
				
			}else{//rimettiamo prescelto nella coda qab
				qab.type=qab.pid;
				if(msgsnd(processo.qab_id, &qab, (sizeof(qab)-sizeof(long)), 0)==-1){
					if(datiFinali->tempoScaduto!=1)
						perror("ERRORE msgsnd processo B qab");
					exit(-1);
				}
				if(sigprocmask(SIG_SETMASK, &oldmask_sigterm, NULL)==-1){	//reset della maschera
					perror("ERRORE SIGPROCMASK reset");
					exit(-1);
				}
				
			}
		}		
		//processo B si è accoppiato, ha informato il gestore, ora sblocca SIGTERM e termina
		if(sigprocmask(SIG_SETMASK, &oldmask_sigterm, NULL)==-1){	//reset della maschera
			perror("ERRORE SIGPROCMASK reset");
			exit(-1);
		}	
	}
	return 0;
}

individuo_t set_individuo(char* argomento[], individuo_t individuo){
	individuo.tipo=*argomento[1];
	int w;
	char *temp;
	temp=argomento[2];
	for(w=0; temp[w]!='\0';w++)
		individuo.nome[w]=temp[w];
	individuo.nome[w]='\0';
	individuo.genoma=(unsigned)atoi(argomento[3]);
	individuo.sem_id=(unsigned)atoi(argomento[4]);
	individuo.shm_id=(unsigned)atoi(argomento[5]);
	individuo.qab_id=(unsigned)atoi(argomento[6]);
	individuo.qba_id=(unsigned)atoi(argomento[7]);
	individuo.qagest_id=(unsigned)atoi(argomento[8]);
	individuo.qbgest_id=(unsigned)atoi(argomento[9]);
	individuo.qconf_id=(unsigned)atoi(argomento[10]);
	individuo.qnascite_id=(unsigned)atoi(argomento[11]);
	individuo.init_people=(unsigned)atoi(argomento[12]);
	return individuo;
}








