all: server
	
server: server.c io.o sockets.o macros.h
	gcc -Wall -o server server.c io.o sockets.o

io.o: io.c io.h macros.h
	gcc -Wall -o io.o -c io.c

sockets.o: sockets.c sockets.h macros.h
	gcc -Wall -o sockets.o -c sockets.c

.PHONY: clean

clean:
	rm server io.o sockets.o
