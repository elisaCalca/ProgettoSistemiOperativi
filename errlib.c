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
#include <sys/msg.h>
#include <sys/shm.h>

#include "errlib.h"

void errexecve(void){

	switch(errno){

	case EACCES:
		perror("ERRORE EXECVE > il pathname non è eseguibile o sono negati i permessi");
		break;

	case ENOENT:
		perror("ERRORE EXECVE > il pathname indicato è inesistente");
		break;

	case ENOEXEC:
		perror("ERRORE EXECVE > il pathname non è in un formato effettivamente eseguibile");
		break;

	case ETXTBSY:
		perror("ERRORE EXECVE > il pathname è aperto in scrittura da un altro processo");
		break;

	case E2BIG:
		perror("ERRORE EXECVE > lo spazio complessivo richiesto supera la dimensione max consentita");
		break;

	default: //se falliscono i casi precedenti
		perror("ERRORE EXECVE > l'errore non è presente nella lista");
		break;
	}

	return;
}










