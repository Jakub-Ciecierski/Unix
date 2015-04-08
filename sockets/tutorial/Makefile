all: clientLocal clientTCP clientUDP  server
clientTCP: clientTCP.c	
	gcc -Wall -o clientTCP clientTCP.c
clientUDP: clientUDP.c	
	gcc -Wall -o clientUDP clientUDP.c
clientLocal: clientLocal.c	
	gcc -Wall -o clientLocal clientLocal.c
server: server.c	
	gcc -Wall -o server server.c
.PHONY: clean
clean:
	rm clientLocal clientUDP clientTCP server
