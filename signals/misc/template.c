#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}

int setaction( void (*f)(int, siginfo_t *, void *), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = f;
	act.sa_flags = SA_SIGINFO;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}


void usage()
{
	fprintf(stderr,"main \n");
	fprintf(stderr,"... \n");
}

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		usage();
		exit(EXIT_FAILURE);
	}

	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);
	
	exit(EXIT_SUCCESS);
}
