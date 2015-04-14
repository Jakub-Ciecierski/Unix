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
#define MAXMSG 80

volatile sig_atomic_t do_continue = 1;

void sig_handler(int sig)
{
	fprintf(stderr,"[Server] Signal \n");
	do_continue = 0;
}

int add_new_client(int sfd,fd_set* base_rfds, int* fdmax){
	int fd;	
	if((fd=TEMP_FAILURE_RETRY(accept(sfd,NULL,NULL)))<0){
		if(EAGAIN==errno||EWOULDBLOCK==errno) return -1;
		ERR("accept");
	}
	FD_SET(fd, base_rfds);
	*fdmax=(*fdmax<fd?fd:*fdmax);
	return 1;
}

int remove_client()
{
	return 1;
}

void communicate(fd_set rfds, fd_set* base_rfds, int fdmax, int* numbers){
	int i;
	int size;
	char data[MAXMSG];
	
	for(i = 0;i <= fdmax; i++){
		if(FD_ISSET(i,&rfds)){
			if((size=bulk_read(i, data, MAXMSG))<0) 
				ERR("read:");
			if(size == 0){
				fprintf(stderr,"[Server] Removing client \n");
				FD_CLR(i, base_rfds);
				if(TEMP_FAILURE_RETRY(close(i))<0)ERR("close");
			}else{
				fprintf(stderr,"[Server] Received: %s \n",data);
			}
		}
	}
}

void server_work(int fdT)
{
	fprintf(stderr,"[Server] starting work \n");
	int fdmax;
	int numbers[FD_SETSIZE];
	fd_set base_rfds, rfds;
	sigset_t mask, oldmask;
	
	FD_ZERO(&base_rfds);
	FD_SET(fdT, &base_rfds);
	
	fdmax = fdT;
	
	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	
	while(do_continue){
		rfds=base_rfds;
		fprintf(stderr,"[Server] starting pselect \n");
		if(pselect(fdmax+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(fdT,&rfds)){
				fprintf(stderr,"[Server] new tcp client \n");
				add_new_client(fdT, &base_rfds, &fdmax);
				FD_CLR(fdT,&rfds);
			}
			FD_CLR(fdT, &base_rfds);
			communicate(rfds, &base_rfds, fdmax, numbers);
			FD_SET(fdT, &base_rfds);
			
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
	int fdT;
	int new_flags;

	if(argc!=2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	// set handlers
	if(sethandler(sig_handler, SIGINT)<0) ERR("sethandler");
	if(sethandler(SIG_IGN, SIGPIPE)<0) ERR("sethandler");
	
	fprintf(stderr,"[Server] before bouding socket.\n");
	
	// TCP inet socket
	fdT=bind_inet_socket(atoi(argv[1]),SOCK_STREAM, BACKLOG);
	new_flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);
	
	fprintf(stderr,"[Server] after bouding socket.\n");
	
	// do work
	server_work(fdT);
	
	// clean up
	if(TEMP_FAILURE_RETRY(close(fdT))<0)ERR("close");

	fprintf(stderr,"Server has terminated.\n");
	
	return EXIT_SUCCESS;
}
