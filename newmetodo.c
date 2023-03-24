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

int mcd(int m, int n){
	while(m!=n){
		if(m>n)
			m=m-n;
		else
			n=n-m;
	}
	return m;
}

int maxdiv(int genoma){
	for(int div=2; div<(genoma/2);div++){
		if(genoma%div==0){
			return genoma/div;
		}
	}
	return 1;	//è un numero primo
}









