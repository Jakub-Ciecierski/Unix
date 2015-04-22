all: client server
client: client.c
	gcc -Wall -o client client.c -lpthread
server: server.c	
	gcc -Wall -o server server.c -lpthread
.PHONY: clean
clean:
	-rm client server
