# FLAGS PER LA COMPILAZIONE

CFLAGS = -Wpedantic -Wall

all: descrittore.bin Gestore.bin

Gestore.bin :  Gestore.o newlife.o errlib.o newsemaphore.o newmetodo.o
	echo Linking Gestore
	gcc $(CFLAGS) -o Gestore Gestore.o newlife.o errlib.o newsemaphore.o newmetodo.o

descrittore.bin : descrittore.o newsemaphore.o newmetodo.o
	echo Linking descrittore
	gcc $(CFLAGS) -o descrittore descrittore.o newsemaphore.o newmetodo.o
	
Gestore.o : Gestore.c
	echo Creazione Gestore.o
	gcc $(CFLAGS) -c Gestore.c

newlife.o : newlife.c
	echo Creazione newlife.o
	gcc $(CFLAGS) -c newlife.c

errlib.o : errlib.c errlib.h 
	echo Creazione errlib.o
	gcc $(CFLAGS) -c errlib.c
	
descrittore.o : descrittore.c
	echo Creazione descrittore.o newsemaphore.h newmetodo.h
	gcc $(CFLAGS) -c descrittore.c
	
newsemaphore.o : newsemaphore.c
	echo Creazione newsemaphore.o
	gcc $(CFLAGS) -c newsemaphore.c

newmetodo.o : newmetodo.c
	echo Creazione newmetodo.o
	gcc $(CFLAGS) -c newmetodo.c

