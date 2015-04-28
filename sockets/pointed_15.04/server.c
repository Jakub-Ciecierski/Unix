#include "macros.h"
#include "io.h"
#include "sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>

#define BACKLOG 3

volatile sig_atomic_t do_continue = 1;

void sig_handler(int sig)
{
	fprintf(stderr,"[Server] Signal \n");
	do_continue = 0;
}

void calculate(int32_t* data)
{
	(*data)++;
}

void communicate(int fd){
	fprintf(stderr,"[Server] Before recvfrom \n");
	struct sockaddr_in addr;
	int32_t data;
	socklen_t size=sizeof(addr);

	while(recvfrom(fd,(char *)&data, sizeof(int32_t),0,&addr,&size)<0)
	{
		if(EINTR!=errno && errno!=EPIPE && errno!=ECONNRESET) ERR("recvfrom:");
		if(errno==EPIPE || errno==ECONNRESET) break;
		if(!do_continue) return;
	}

	fprintf(stderr,"[Server] After recvfrom \n");
	
	data = ntohl(data);
	calculate(&data);
	data = htonl(data);
	
	while(sendto(fd,(char *)&data,sizeof(int32_t),0,&addr,sizeof(addr))<0)
	{
		if(EINTR!=errno && errno!=EPIPE && errno!=ECONNRESET) ERR("recvfrom:");
		if(errno==EPIPE || errno==ECONNRESET) break;
		if(!do_continue) return;
	}		
}

void server_work(int fdU)
{	
	while(do_continue){
		communicate(fdU);
	}
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s port\n",name);
}

int main(int argc, char** argv) {
	int fdU;

	if(argc!=2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	// set handlers
	if(sethandler(sig_handler, SIGINT)<0) ERR("sethandler");
	if(sethandler(SIG_IGN, SIGPIPE)<0) ERR("sethandler");
	
	// UDP inet socket
	fdU=bind_inet_socket(atoi(argv[1]),SOCK_DGRAM, BACKLOG);

	fprintf(stderr,"server before work \n");

	// do work
	server_work(fdU);
	
	// clean up
	if(TEMP_FAILURE_RETRY(close(fdU))<0)ERR("close");
	
	fprintf(stderr,"Server has terminated.\n");
	
	return EXIT_SUCCESS;
}
