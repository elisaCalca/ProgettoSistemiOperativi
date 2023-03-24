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

void newlife(int init_people, int sem_id, int shm_id, int qab_id, int qba_id, int qagest_id, int qbgest_id, int qconf_id, int qnascite_id){
	pid_t child;
	dati_finali_t *datiFinali;
	int ret, sem_stat;	
	char*caratteristiche[13];
	int genes=(1+rand()%201);
	for(int i=0;i<13;i++){
		caratteristiche[i]=malloc(50*sizeof(char));
	}
	
	datiFinali=(dati_finali_t*)shmat(shm_id, NULL, 0);//attacca il segmento di SM alla memoria virtuale del processo chiamante
	char tipo;
	
	for(int i=0;(i<init_people);i++){
		child=fork();
		
		if(child==0){
			//è nato il primo figlio! occupare il semaforo!
			sem_stat=reserveSem(sem_id, 0);
			if(sem_stat==-1){
				if(datiFinali->tempoScaduto!=1)
					perror("ERRORE reserveSem");
				exit(-1);
			}
				
			srand((unsigned)(time(NULL)+getpid()));
			tipo=rand();
			if(tipo%2==0){
				sprintf(caratteristiche[1], "%c", 'A');
			}else{
				sprintf(caratteristiche[1], "%c", 'B');
			}
			sprintf(caratteristiche[2], "%c", (65+(int)(rand()%(90-64))));	//tra 65 e 65+25 estremi compresi
			sprintf(caratteristiche[3], "%d", (2+(int)(rand()%(genes+1))));	//intervallo tra 2 e genes+2 estremi compresi
			sprintf(caratteristiche[4], "%d", (sem_id));
			sprintf(caratteristiche[5], "%d", (shm_id));
			sprintf(caratteristiche[6], "%d", (qab_id));
			sprintf(caratteristiche[7], "%d", (qba_id));
			sprintf(caratteristiche[8], "%d", (qagest_id));
			sprintf(caratteristiche[9], "%d", (qbgest_id));
			sprintf(caratteristiche[10], "%d", (qconf_id));
			sprintf(caratteristiche[11], "%d", (qnascite_id));
			sprintf(caratteristiche[12], "%d", (init_people));
			caratteristiche[13]=NULL;
		
			ret=execve("descrittore", &caratteristiche[0], NULL);
			if(ret==-1)
				errexecve();
			exit(-1);// se qui ci tornano dei processi vuol dire che ha fallito l'execve e quindi vanno chiusi
				
		}
	}
	
	return;
}

void newtwins(int init_people, int sem_id, int shm_id, int qab_id, int qba_id, int qagest_id, int qbgest_id, int qconf_id, char* nomevecchio, int maxcd, int qnascite_id){
	int ret, sem_stat;
	dati_finali_t *datiFinali;
	char*caratteristiche[13];	//array di stringhe
	int genes=(1+rand()%201);
	for(int i=0;i<13;i++){
		caratteristiche[i]=malloc(101*sizeof(char));
	}
	
	datiFinali=(dati_finali_t*)shmat(shm_id, NULL, 0);//attacca il segmento di SM alla memoria virtuale del processo chiamante
	char tipo;
	char nome[101];
	
	//è nato il primo figlio! occupare il semaforo!
	sem_stat=reserveSem(sem_id, 0);
	if(sem_stat==-1){
		if(datiFinali->tempoScaduto!=1)
			perror("ERRORE reserveSem");
		exit(-1);
	}
				
	srand((unsigned)(time(NULL)+getpid()));
	tipo=rand();
	if(tipo%2==0){
		sprintf(caratteristiche[1], "%c", 'A');		//TIPO
	}else{
		sprintf(caratteristiche[1], "%c", 'B');
	}
	int i;
	for(i=0; i<strlen(nomevecchio);i++){
		nome[i]=nomevecchio[i];
	}
	nome[i]=(char)(65+(int)(rand()%(90-64)));
	i++;
	nome[i]='\0';
	sprintf(caratteristiche[2], nome, nome);	//tra 97 e 122 estremi compresi
	sprintf(caratteristiche[3], "%d", (maxcd+(int)(rand()%(genes+maxcd+1))));		//GENOMA
	sprintf(caratteristiche[4], "%d", (sem_id));
	sprintf(caratteristiche[5], "%d", (shm_id));
	sprintf(caratteristiche[6], "%d", (qab_id));
	sprintf(caratteristiche[7], "%d", (qba_id));
	sprintf(caratteristiche[8], "%d", (qagest_id));
	sprintf(caratteristiche[9], "%d", (qbgest_id));
	sprintf(caratteristiche[10], "%d", (qconf_id));
	sprintf(caratteristiche[11], "%d", (qnascite_id));
	sprintf(caratteristiche[12], "%d", (init_people));
	caratteristiche[13]=NULL;
		
	ret=execve("descrittore", &caratteristiche[0], NULL);
	if(ret==-1)
		errexecve();
	exit(-1);// se qui ci tornano dei processi vuol dire che ha fallito l'execve e quindi vanno chiusi
					
	return;
}






