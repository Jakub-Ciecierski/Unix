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
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#define MSG_SIZE 100

void client_work(int fd)
{
	char* data = NULL;
	fprintf(stderr,"[Client] waiting for response \n");
	if(bulk_read(fd, (char*)data, MSG_SIZE) <0 && errno!=EPIPE)
		ERR("bulk_write");
	fprintf(stderr,"[Client] received response \n");
	fprintf(stdout,"%s \n",data);
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port  \n",name);
}

int main(int argc, char** argv) {
	int fd;
	
	if(argc!=3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	
	fd=connect_inet_socket(argv[1],atoi(argv[2]));

	client_work(fd);
	
	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}
