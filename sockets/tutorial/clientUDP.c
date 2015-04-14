#include "macros.h"
#include "io.h"
#include "sockets.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

volatile sig_atomic_t last_signal=0 ;

void sigalrm_handler(int sig) {
	last_signal=sig;
}

void client_udp_work(int fd, int32_t data[], struct sockaddr_in addr)
{
	fprintf(stderr,"[UDP] sending data \n");
	if(TEMP_FAILURE_RETRY(sendto(fd,(char *)data,
			sizeof(int32_t[5]),0,(struct sockaddr*)&addr,sizeof(addr)))<0 && 
				errno!=EPIPE && errno!=ECONNRESET)
		ERR("sendto:");
		
	alarm(1);
	
	fprintf(stderr,"[UDP] waiting for response \n");
	socklen_t len = sizeof(struct sockaddr_in);
	while(recvfrom(fd,(char *)data,sizeof(int32_t[5]),0,
			(struct sockaddr*)&addr,&len)<0){
		if(EINTR!=errno && errno!=EPIPE && errno!=ECONNRESET)ERR("recv:");
		if(SIGALRM==last_signal) {
			fprintf(stderr,"[UDP] timeout reached... \n");
			break;
		}
	}
	if(last_signal!=SIGALRM) {
		prepare_revc_data(data);
		analyze_result(data);
	}
	
	sleep(2);
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port  operand1 operand2 operation \n",name);
}

int main(int argc, char** argv) {
	int fd;
	struct sockaddr_in addr;
	int32_t data[5];
	
	if(argc!=6) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	if(sethandler(sigalrm_handler,SIGALRM)) ERR("Seting SIGALRM:");
	
	fd = make_socket(PF_INET, SOCK_DGRAM);
	addr=make_inet_address(argv[1],atoi(argv[2]));
	
	prepare_send_inet_data(argv, data);
	client_udp_work(fd, data, addr);

	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}
