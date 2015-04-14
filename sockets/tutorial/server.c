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

void calculate(int32_t data[])
{
	fprintf(stderr,"[Server] calculating \n");
	int32_t op1, op2, result, status = 1;
	op1 = ntohl(data[0]);
	op2 = ntohl(data[1]);
	switch(ntohl(data[3]))
	{
		case '*':
			result = op1 * op2;
			break;
		case '/':
			if(op2 == 0) status = 0;
			else result = op1/op2;
			break;
		case '+':
			result = op1+op2;
			break;
		case '-':
			result = op1-op2;
			break;
	}
	
	data[2] = htonl(result);
	data[4] = htonl(status);
}

void communicateStream(int cfd){
	ssize_t size;
	int32_t data[5];
	if((size=bulk_read(cfd,(char *)data,sizeof(int32_t[5])))<0) 
		ERR("read:");
	if(size==(int)sizeof(int32_t[5])){
		calculate(data);
		if(bulk_write(cfd,(char *)data,sizeof(int32_t[5]))<0&&errno!=EPIPE) 
			ERR("write:");
	}
	if(TEMP_FAILURE_RETRY(close(cfd))<0)ERR("close");
}

void communicateDgram(int fd){
	struct sockaddr_in addr;
	int32_t data[5];
	socklen_t size=sizeof(addr);
	if(TEMP_FAILURE_RETRY(recvfrom(fd,(char *)data,
				sizeof(int32_t[5]),0,&addr,&size))<0) ERR("read:");
				
	calculate(data);

	if(TEMP_FAILURE_RETRY(sendto(fd,(char *)data,
				sizeof(int32_t[5]),0,&addr,sizeof(addr)))<0 &&
					errno!=EPIPE) ERR("write:");
}

void server_work(int fdL, int fdT, int fdU)
{
	int cfd,fdmax;
	fd_set base_rfds, rfds ;
	sigset_t mask, oldmask;
	
	FD_ZERO(&base_rfds);
	FD_SET(fdL, &base_rfds);
	FD_SET(fdT, &base_rfds);
	FD_SET(fdU, &base_rfds);
	
	fdmax=(fdT>fdL?fdT:fdL);
	fdmax=(fdU>fdmax?fdU:fdmax);
	
	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	
	while(do_continue){
		rfds=base_rfds;
		cfd=-1;
		fprintf(stderr,"[Server] starting pselect \n");
		if(pselect(fdmax+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(fdL,&rfds)){
				fprintf(stderr,"[Server] new local client \n");
				cfd=add_new_client(fdL);
				if(cfd>=0)communicateStream(cfd);
			}
			if(FD_ISSET(fdT,&rfds)){
				fprintf(stderr,"[Server] new tcp client \n");
				cfd=add_new_client(fdT);
				if(cfd>=0)communicateStream(cfd);
			}
			if(FD_ISSET(fdU,&rfds)){
				fprintf(stderr,"[Server] new udp client \n");
				communicateDgram(fdU);
			}

		}else{
			if(EINTR==errno) continue;
			ERR("pselect");
		}
	}
	sigprocmask (SIG_UNBLOCK, &mask, NULL);
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s socket port\n",name);
}

int main(int argc, char** argv) {
	int fdL,fdT,fdU;
	int new_flags;

	if(argc!=3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	// set handlers
	if(sethandler(sig_handler, SIGINT)<0) ERR("sethandler");
	if(sethandler(SIG_IGN, SIGPIPE)<0) ERR("sethandler");
	
	// Local socket
	fdL=bind_local_socket(argv[1], SOCK_STREAM, BACKLOG);
	new_flags = fcntl(fdL, F_GETFL) | O_NONBLOCK;
	fcntl(fdL, F_SETFL, new_flags);
	
	// TCP inet socket
	fdT=bind_inet_socket(atoi(argv[2]),SOCK_STREAM, BACKLOG);
	new_flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);
	
	// UDP inet socket
	fdU=bind_inet_socket(atoi(argv[2]),SOCK_DGRAM, BACKLOG);

	// do work
	server_work(fdL, fdT, fdU);
	
	// clean up
	if(TEMP_FAILURE_RETRY(close(fdL))<0)ERR("close");
	if(TEMP_FAILURE_RETRY(close(fdT))<0)ERR("close");
	if(TEMP_FAILURE_RETRY(close(fdU))<0)ERR("close");
	if(unlink(argv[1])<0)ERR("unlink");
	
	fprintf(stderr,"Server has terminated.\n");
	
	return EXIT_SUCCESS;
}
