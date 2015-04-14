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

void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port  operand1 operand2 operation \n",name);
}

int main(int argc, char** argv) {
	int fd;
	int32_t data[5];
	
	if(argc!=6) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	
	fd=connect_inet_socket(argv[1],atoi(argv[2]));
	
	prepare_send_inet_data(argv, data);	

	client_work(fd, data);
	
	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}
