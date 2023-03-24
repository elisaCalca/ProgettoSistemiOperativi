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

#include "errlib.h"
#include "newlife.h"
#include "newsemaphore.h"
#include "newmetodo.h"

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

typedef struct mymsg{		//struct per l'utilizzo della coda
	long type;
	pid_t pid;
	char tipo;
	char nome[100];
	unsigned long genoma;
}mymsg_t;

typedef struct msgnascite{	//coda per contenere tutte le nascite
	long type;
	pid_t pid;
}msgnascite_t;

int tempoScaduto=0;
int aggiornamento=0;
int birth_death, sim_time, qnascite_id;
pid_t lettore;

void handler_sim_time(int sig);
void sigalrm_receiver(void);

//----------------------------------------------------------MAIN---------------------------------------------------------------
int main(int argc, char* argv[]){
	pid_t childTerm, childtw, childlettore;
	int maxcd;
	int init_people, sem_stat, sem_id, shm_id, qab_id, qba_id, qagest_id, qbgest_id, qconf_id;
	dati_finali_t *datiFinali;
	mymsg_t qagest, qbgest;
	msgnascite_t qnascite;
	
	sigalrm_receiver();//installa l'handler per SIGALRM
	srand((unsigned)time(NULL));
	birth_death=2+((int)rand()%3);//tra 2 e 2+2
	printf("BIRTH_DEATH %d\n", birth_death);
	sim_time=birth_death+((int)rand()%(41+birth_death)); //tra birth_death e 40+birth_death (tra 2 e 44)
	printf("SIM_TIME %d\n", sim_time);
	alarm(birth_death);	//dopo birth_death secondi invia SIGALRM (l'handler interviene)
	
	//creo un numero iniziale init_people di individui
	init_people=2+((int)(rand()%101));	//considera l'intervallo tra 2 e 2+100

	//ignoro SIGTERM
	sigset_t newmask_sigterm, oldmask_sigterm;
	sigemptyset(&newmask_sigterm);	//insieme vuoto
	sigaddset(&newmask_sigterm, SIGTERM);	//aggiunge SIGTERM alla maschera
	//da qui bisogna ignorare SIGTERM
	if(sigprocmask(SIG_BLOCK, &newmask_sigterm, &oldmask_sigterm)==-1){	//setta la nuova maschera e memorizza la vecchia
		perror("ERRORE SIGPROCMASK settaggio");
		exit(-1);
	}
	
	//il gestore crea il set di SEMAFORI e lo inizializza per evitare RACE CONDITION
	//stessa cosa per l'area di SHARED MEMORY
	//stessa cosa per la CODA DI MESSAGGI 
//---------------------------------------------------------SEMAPHORE-----------------------------------------------------------	
	key_t key=IPC_PRIVATE;
	sem_id=semget(key, 2, IPC_CREAT | IPC_EXCL | 0666);	//genero il set (2 è la quantità che ce ne sono) il primo è zero 
	if(sem_id==-1){  //il semaforo era già esistente
		sem_id=semget(key, 3, 0666);	//ne preleviamo l'id
		if(sem_id==-1){
			perror("ERRORE semget");
			exit(-1);
		}
	}
	//semaforo 0 inizializzato libero
	sem_stat=initSemAvailable(sem_id, 0);
	if(sem_stat==-1){
		perror("ERRORE initSemAvailable");
		exit(-1);
	}
	//semaforo 1 inizializzato occupato (diventerà libero quando il numero di processiViventi sarà == init_people)
	sem_stat=initSemInUse(sem_id, 1);
	if(sem_stat==-1){
		perror("ERRORE initSemInUse");
		exit(-1);
	}
	
//-------------------------------------------------------MESSAGE QUEUE---------------------------------------------------------
	qab_id=msgget(key, IPC_CREAT | 0666);
	if(qab_id==-1){
		perror("ERRORE msgget q_ab");
		exit(-1);
	}
	
	qba_id=msgget(key, IPC_CREAT | 0666);
	if(qba_id==-1){
		perror("ERRORE msgget q_ba");
		exit(-1);
	}
	
	qagest_id=msgget(key, IPC_CREAT | 0666);
	if(qagest_id==-1){
		perror("ERRORE msgget qa_gest");
		exit(-1);
	}
	
	qbgest_id=msgget(key, IPC_CREAT | 0666);
	if(qbgest_id==-1){
		perror("ERRORE msgget qb_gest");
		exit(-1);
	}
	
	qconf_id=msgget(key, IPC_CREAT | 0666);
	if(qconf_id==-1){
		perror("ERRORE msgget qconf_id");
		exit(-1);
	}
	
	qnascite_id=msgget(key, IPC_CREAT | 0666);
	if(qnascite_id==-1){
		perror("ERRORE msgget qnascite_id");
		exit(-1);
	}
	
//-------------------------------------------------------SHARED MEMORY---------------------------------------------------------
	shm_id=shmget(key, sizeof(dati_finali_t), IPC_CREAT | IPC_EXCL | 0666);
	if(shm_id==-1){	
		perror("ERRORE shmget");
		exit(-1);
	}
	datiFinali=(dati_finali_t*)shmat(shm_id, NULL, 0);//attacca il segmento di SM alla memoria virtuale del processo chiamante
	
	datiFinali->processiA=0;
	datiFinali->processiB=0;
	datiFinali->processiViventi=0;
	datiFinali->maxnPid=0;
	datiFinali->maxnTipo='\0';
	datiFinali->maxnNome[0]='\0';
	datiFinali->maxnGenoma=0;
	datiFinali->maxgPid=0;
	datiFinali->maxgTipo='\0';
	datiFinali->maxgNome[0]='\0';
	datiFinali->maxgGenoma=0;
	datiFinali->tempoScaduto=0;

	newlife(init_people, sem_id, shm_id, qab_id, qba_id, qagest_id, qbgest_id, qconf_id, qnascite_id);
	//fino a qui ho generato il primo gruppo di figli n. init_people
	
	childlettore=fork();
	
	sigset_t newmask_sigalrm, oldmask_sigalrm;
	sigemptyset(&newmask_sigalrm);	//insieme vuoto
	sigaddset(&newmask_sigalrm, SIGALRM);	//aggiunge SIGALRM alla maschera
		
	while(tempoScaduto==0){
		if(childlettore==0){	//SIAMO NEL FIGLIO CHE SI OCCUPA DELLA LETTURA DELLA CODA DEGLI ACCOPPIAMENTI
			//da qui bisogna ignorare SIGALRM (un'interruzione su birth_death causerebbe la non generazione di un figlio già commissionato)
			if(sigprocmask(SIG_BLOCK, &newmask_sigalrm, &oldmask_sigalrm)==-1){	//setta la nuova maschera e memorizza la vecchia
				perror("ERRORE SIGPROCMASK settaggio");
				exit(-1);
			}
			lettore=getpid();
			if(msgrcv(qagest_id, &qagest, (sizeof(qagest)-sizeof(long)), 0, 0)==-1){
				if(datiFinali->tempoScaduto!=1)
					perror("ERRORE msgrcv gestore qagest");
				exit(-1);
			}
			if(msgrcv(qbgest_id, &qbgest, (sizeof(qbgest)-sizeof(long)), qagest.pid, 0)==-1){
				if(datiFinali->tempoScaduto!=1)
					perror("ERRORE msgrcv gestore qbgest");
				exit(-1);
			}
			printf("ACCOPPIANTE \"A\" LETTO DALLA CODA> pid: %d, tipo: %c, nome: %s, genoma: %li\n", qagest.pid, qagest.tipo, qagest.nome, qagest.genoma);
			printf("ACCOPPIANTE \"B\" LETTO DALLA CODA> pid: %d, tipo: %c, nome: %s, genoma: %li\n", qbgest.pid, qbgest.tipo, qbgest.nome, qbgest.genoma);	
			//i processi ricevuti sono terminati. Devono nascere due figli con caratteristiche basate su quelli che sono terminati
			maxcd=mcd(qagest.genoma, qbgest.genoma);
			childtw=fork();
			if(childtw==0){
				newtwins(1, sem_id, shm_id, qab_id, qba_id, qagest_id, qbgest_id, qconf_id, qagest.nome, maxcd, qnascite_id);
				exit(EXIT_SUCCESS);
			}
			childtw=fork();
			if(childtw==0){
				newtwins(1, sem_id, shm_id, qab_id, qba_id, qagest_id, qbgest_id, qconf_id, qbgest.nome, maxcd, qnascite_id);
				exit(EXIT_SUCCESS);
			}
		}
		
		if((aggiornamento==1)&&(childlettore!=0)){
			childTerm=fork();
			if(childTerm==0){
				if(msgrcv(qnascite_id, &qnascite, (sizeof(qnascite)-sizeof(long)), 0, 0)==-1){
					perror("ERRORE msgrcv handler birth_death");
					exit(-1);
				}
				kill(qnascite.pid, SIGTERM);
				childtw=fork();
				if(childtw==0){
					newlife(1, sem_id, shm_id, qab_id, qba_id, qagest_id, qbgest_id, qconf_id, qnascite_id);
					exit(EXIT_SUCCESS);
				}
				printf("\n\nHO UCCISO UN PROCESSO E NE HO GENERATO UN ALTRO, la situazione attuale è questa:\n");
				printf("TEMPO RIMANENTE: %d secondi\n", sim_time);
				printf("N. PROCESSI A: %d\n", datiFinali->processiA);
				printf("N. PROCESSI B: %d\n", datiFinali->processiB);
				printf("N. PROCESSI VISSUTI: %d\n", datiFinali->processiViventi);
				printf("\nPROCESSO CON IL NOME PIÙ LUNGO>\n");
				printf("PID: %d, TIPO: %c, NOME: %s, GENOMA: %d\n", datiFinali->maxnPid, datiFinali->maxnTipo, datiFinali->maxnNome, datiFinali->maxnGenoma);
				printf("\nPROCESSO CON IL GENOMA PIÙ LUNGO>\n");
				printf("PID: %d, TIPO: %c, NOME: %s, GENOMA: %d\n", datiFinali->maxgPid, datiFinali->maxgTipo, datiFinali->maxgNome, datiFinali->maxgGenoma);
				printf("\n\n");
					
				exit(EXIT_SUCCESS);
			}
			aggiornamento=0;
		}			
	}
	
	datiFinali->tempoScaduto=1;
	//Bisogna far terminare il processo che si occupa della lettura degli accoppianti
	if(childlettore==0){
		exit(EXIT_SUCCESS);
	}

	//LA SIMULAZIONE È TERMINATA
	//occupare il semaforo 0
	sem_stat=reserveSem(sem_id, 0);
	if(sem_stat==-1){
		perror("ERRORE reserveSem");
		exit(-1);
	}
	while((msgrcv(qnascite_id, &qnascite, (sizeof(qnascite)-sizeof(long)), 0, IPC_NOWAIT)!=-1)&&(errno!=ENOMSG)){
		printf("UCCIDERE %d\n", qnascite.pid);
		kill(qnascite.pid, SIGKILL);
	}
	
//---------------------------------------------------AGGIORNAMENTO UTENTE------------------------------------------------------

	printf("\n\n");
	printf("TEMPO SCADUTO\n");
	printf("N. PROCESSI A: %d\n", datiFinali->processiA);
	printf("N. PROCESSI B: %d\n", datiFinali->processiB);
	printf("N. PROCESSI VISSUTI: %d\n", datiFinali->processiViventi);
	printf("\nPROCESSO CON IL NOME PIÙ LUNGO>\n");
	printf("PID: %d, TIPO: %c, NOME: %s, GENOMA: %d\n", datiFinali->maxnPid, datiFinali->maxnTipo, datiFinali->maxnNome, datiFinali->maxnGenoma);
	printf("\nPROCESSO CON IL GENOMA PIÙ LUNGO>\n");
	printf("PID: %d, TIPO: %c, NOME: %s, GENOMA: %d\n", datiFinali->maxgPid, datiFinali->maxgTipo, datiFinali->maxgNome, datiFinali->maxgGenoma);
	printf("\n\n");
	if(shmctl(shm_id, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rilascio shared memory");
	if(semctl(sem_id, 0, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rimozione semaforo 0 e 1");
	if(msgctl(qab_id, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rimozione message queue ab");
	if(msgctl(qba_id, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rimozione message queue ba");
		
	if(msgctl(qagest_id, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rimozione message queue agest");
		
	if(msgctl(qbgest_id, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rimozione message queue bgest");
		
	if(msgctl(qconf_id, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rimozione message queue qconf");
		
	if(msgctl(qnascite_id, IPC_RMID, NULL)==-1)
		perror("ERRORE GESTORE rimozione message queue qnascite");
			
		 
	printf("\nIL GESTORE TERMINA QUI\n");
	return 0;

}


//CODICE DELL'HANDLER PER GESTIRE GLI ALRM 
void sigalrm_receiver(void){
	int ris_sig;
	struct sigaction sim_timeAct;
	sim_timeAct.sa_handler=&handler_sim_time;	//inserisco il puntatore all'handler da eseguire nella struttura sim_timeAct
	sim_timeAct.sa_flags=SA_NODEFER; 	//SA_NODEFER consente invocazioni annidate
	sigemptyset(&sim_timeAct.sa_mask);		//non nasconde segnali
	
	ris_sig=sigaction(SIGALRM, &sim_timeAct, NULL);	//il vecchio handler non mi serve 
	if(ris_sig==-1)
		perror("ERRORE sigaction sim_time");			
}


void handler_sim_time(int sig){
	printf("È STATO RICEVUTO UN SIGALRM \n");	
	if(sim_time>birth_death){	//c'è ancora del tempo per l'esecuzione
		aggiornamento=1;
		tempoScaduto=0;
		alarm(birth_death);
	}else{
		alarm(sim_time);
		sim_time=0;
		tempoScaduto=1;
	}
	
	sim_time=sim_time-birth_death;	
	return;
}






