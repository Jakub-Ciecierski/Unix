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

int add_new_client(int sfd){
	int nfd;
	if((nfd=TEMP_FAILURE_RETRY(accept(sfd,NULL,NULL)))<0) {
		if(EAGAIN==errno||EWOULDBLOCK==errno) return -1;
		ERR("accept");
	}
	return nfd;
}

void calculate(int32_t* data)
{
	(*data)++;
}

void communicate(int fd){
	struct sockaddr_in addr;
	int32_t data;
	socklen_t size=sizeof(addr);
	if(TEMP_FAILURE_RETRY(recvfrom(fd,(char *)&data,
				sizeof(int32_t),0,&addr,&size))<0 &&
					errno!=EPIPE && errno!=ECONNRESET) ERR("read:");
	
	data = ntohl(data);
	calculate(&data);
	data = htonl(data);
	
	if(TEMP_FAILURE_RETRY(sendto(fd,(char *)&data,
				sizeof(int32_t),0,&addr,sizeof(addr)))<0 &&
					errno!=EPIPE && errno!=ECONNRESET) ERR("write:");
}

void server_work(int fdU)
{
	int fdmax;
	fd_set base_rfds, rfds ;
	sigset_t mask, oldmask;
	
	FD_ZERO(&base_rfds);
	FD_SET(fdU, &base_rfds);
	
	fdmax=fdU;
	
	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	
	fprintf(stderr,"server before main loop \n");
	
	while(do_continue){
		rfds=base_rfds;
		fprintf(stderr,"[Server] starting pselect \n");
		if(pselect(fdmax+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(fdU,&rfds)){
				fprintf(stderr,"[Server] new udp client \n");
				communicate(fdU);
			}

		}else{
			if(EINTR==errno) continue;
			ERR("pselect");
		}
	}
	sigprocmask (SIG_UNBLOCK, &mask, NULL);
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
