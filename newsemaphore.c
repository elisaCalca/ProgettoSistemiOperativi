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

union semun{
	int val;
	struct semid_ds* buf;
	unsigned short* array;
};

//inizializzare un semaforo come disponibile
int initSemAvailable(int sem_id, int semNum){
	union semun arg;
	arg.val=1;
	return semctl(sem_id, semNum, SETVAL, arg);
}

//inizializzare un semaforo come occupato
int initSemInUse(int sem_id, int semNum){
	union semun arg;
	arg.val=0;
	return semctl(sem_id, semNum, SETVAL, arg);
}

//Occupare un semaforo
int reserveSem(int sem_id, int semNum){
	struct sembuf sops;
	sops.sem_num=semNum;
	sops.sem_op= -1;
	sops.sem_flg= 0;
	return semop(sem_id, &sops, 1);
}

//Rilasciare un semaforo
int releaseSem(int sem_id, int semNum){	
	struct sembuf sops;
	sops.sem_num= semNum;
	sops.sem_op= 1;
	sops.sem_flg= 0;
	return semop(sem_id, &sops, 1);
}


