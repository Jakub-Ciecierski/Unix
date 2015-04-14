all: clientLocal clientUDP clientTCP server

clientTCP: clientTCP.c client.o io.o sockets.o macros.h
	gcc -Wall -o clientTCP clientTCP.c io.o sockets.o client.o
	
clientUDP: clientUDP.c client.o io.o sockets.o macros.h
	gcc -Wall -o clientUDP clientUDP.c io.o sockets.o client.o
	
clientLocal: clientLocal.c client.o io.o sockets.o macros.h
	gcc -Wall -o clientLocal clientLocal.c io.o sockets.o client.o
	
server: server.c io.o sockets.o macros.h
	gcc -Wall -o server server.c io.o sockets.o

io.o: io.c io.h macros.h
	gcc -Wall -o io.o -c io.c

sockets.o: sockets.c sockets.h macros.h
	gcc -Wall -o sockets.o -c sockets.c

client.o: client.c client.h macros.h io.o
	gcc -Wall -o client.o -c client.c

.PHONY: clean

clean:
	rm clientLocal clientUDP clientTCP server io.o sockets.o client.o
