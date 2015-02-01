all: serwer komisja raport

serwer: serwer.o err.o
	cc -Wall -pthread -lrt -o serwer serwer.o err.o
	
raport: raport.o err.o
	cc -Wall -o raport raport.o err.o	
	
komisja: komisja.o err.o
	cc -Wall -o komisja komisja.o err.o		
	
serwer.o: serwer.c 
	cc -Wall -c serwer.c
	
komisja.o: komisja.c 
	cc -Wall -c komisja.c
	
raport.o: raport.c 
	cc -Wall -c raport.c	
	
err.o: err.c err.h
	cc -Wall -c err.c
	
clean:
	rm -f *.o serwer komisja raport
