# TODO variables

all: main client

main: main.c server.o database.o connection.o io.o sockets.o ../util/macros.h 
	gcc -Wall -o server main.c server.o io.o sockets.o database.o connection.o

client: client.c sockets.o io.o ../util/macros.h ../util/msg_protocol.h
	gcc -Wall -o client client.c sockets.o io.o

connection.o: connection.c io.o sockets.o database.o ../util/msg_protocol.h ../util/io.h ../util/sockets.h
	gcc -Wall -o connection.o -c connection.c io.o sockets.o database.o

server.o: server.c io.o sockets.o connection.o ../util/macros.h ../util/io.h ../util/sockets.h
	gcc -Wall -o server.o -c server.c io.o sockets.o connection.o

io.o: ../util/io.c ../util/io.h ../util/macros.h
	gcc -Wall -o io.o -c ../util/io.c

sockets.o: ../util/sockets.c ../util/sockets.h ../util/macros.h
	gcc -Wall -o sockets.o -c ../util/sockets.c 

database.o: database.c io.o database.h ../util/macros.h
	gcc -Wall -o database.o -c database.c io.o

.PHONY: clean

clean:
	rm server connection.o server.o io.o sockets.o client database.o
