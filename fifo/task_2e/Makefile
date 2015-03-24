all: server client

server: server.o unix.o
	gcc -Wall -o server server.o unix.o

client: client.o unix.o
	gcc -Wall -o client client.o unix.o

client.o: client.c unix.h
	gcc -Wall -o client.o -c client.c

server.o: server.c unix.h
	gcc -Wall -o server.o -c server.c

unix.o: unix.c unix.h
	gcc -Wall -o unix.o -c unix.c
	
.PHONY: clean

clean:
	rm server client server.o client.o unix.o
