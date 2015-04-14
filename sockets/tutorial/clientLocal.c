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
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>

void usage(char * name){
	fprintf(stderr,"USAGE: %s socket operand1 operand2 operation \n",name);
}

int main(int argc, char** argv) {
	int fd;
	if(argc!=5) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if(sethandler(SIG_IGN, SIGPIPE)<0) ERR("sethandler");

	fd=connect_local_socket(argv[1]);

	int32_t data[5];
	prepare_send_local_data(argv, data);	

	client_work(fd, data);
	
	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	fprintf(stderr,"Client has terminated.\n");
	return EXIT_SUCCESS;
}
