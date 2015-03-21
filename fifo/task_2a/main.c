#include "unix.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>

/*
Child process works in cycles as long as there is a chance
to read data from the pipe. First it sleeps random time <0-9>s. 
then it reads one part of data from the pipe. 
*/

volatile sig_atomic_t last_signal = 0;

void sig_handler(int sig)
{
	last_signal = sig;
	fprintf(stderr, " Received signal: %d \n", sig);
}

void parent_work(char* filepath, int pipefd[])
{
	last_signal = 0;
	// close read end of pipe
	TEMP_FAILURE_RETRY(close(pipefd[0]));
	
	int fd = TEMP_FAILURE_RETRY(open(filepath, O_RDONLY));
	char buffer[PIPE_BUF];
	int count = 0;
	do{
		count=bulk_read(fd,buffer,PIPE_BUF);
		if(count<0) ERR("bulk_read()");
		if(count < PIPE_BUF) memset(buffer+count,0,PIPE_BUF-count);
		if(count>0){
			if(bulk_write(pipefd[1],buffer,PIPE_BUF)<0) ERR("bulk_write()");
		}
	}while(count==PIPE_BUF 
				|| last_signal == SIGPIPE 
				|| last_signal == SIGINT);
	fprintf(stderr,"[%d] Parent finished working \n", getpid());
	
	TEMP_FAILURE_RETRY(close(fd));
	TEMP_FAILURE_RETRY(close(pipefd[1]));
}

void child_work(int pipefd[])
{
	srand(getpid());
	int t = rand()%10;
	
	fprintf(stderr,"[%d] Child working \n", getpid());

	char buffer[PIPE_BUF];
	int count = 0;
	do{
		sleep(t);
		count=bulk_read(pipefd[0],buffer,PIPE_BUF);
		if(count<0)ERR("bulk_read()");
		if(count < PIPE_BUF) memset(buffer+count,0,PIPE_BUF-count);
		if(count>0){
			fprintf(stdout,"[%d]************* Message: \n %s \n",getpid(), buffer);
		}
	}while(count!=0); // read will return 0 if descriptor is closed
	
	fprintf(stderr,"[%d] Child finished working\n", getpid());
}

void create_children(int n, int pipefd[])
{
	sethandler(sig_handler, SIGPIPE);
	sethandler(sig_handler, SIGINT);

	int i = 0;
	for(i = 0;i < n;i++)
	{
		pid_t pid = fork();
		if(pid == 0)
		{
			sethandler(SIG_IGN, SIGINT);
			TEMP_FAILURE_RETRY(close(pipefd[1]));
			child_work(pipefd);
			
			exit(EXIT_SUCCESS);
		}
	}
}

void usage()
{
	fprintf(stderr,"main: n filename \n");
}

int main(int argc, char** argv)
{
	if(argc != 3)
	{
		usage();
		return EXIT_FAILURE;
	}
	
	int n=atoi(argv[1]);
	if(n<=0) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	int pipefd[2];
	
	if(pipe(pipefd) < 0 )
		ERR("pipe()");

	create_children(n, pipefd);
	
	parent_work(argv[2], pipefd);
	
	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);
	
	return EXIT_SUCCESS;
}
