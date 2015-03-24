#include "unix.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

void sigpipe_handler(int sig)
{
	fprintf(stderr,"SIG_PIPE \n");
}

void game(char* fifoname)
{
	int fifo;
	int MAX_BETS = 3;
	srand(getpid());

	int* bets = (int*)malloc(MAX_BETS*sizeof(int));
	pid_t* bets_pid = (pid_t*)malloc(MAX_BETS*sizeof(pid_t));

	while(1)
	{
		int bet_index = 0;
		int count = 0;
		fprintf(stdout,"[%d] Starting collecting bets \n", getpid());
		while(bet_index < MAX_BETS)
		{
			if((fifo=TEMP_FAILURE_RETRY(open(fifoname,O_RDONLY)))<0) 
				ERR("open");

			char buffer[PIPE_BUF];

			count=bulk_read(fifo,buffer,PIPE_BUF);
			fprintf(stderr,"[%d] After read \n", getpid());

			if(count<0) ERR("read");
			if(count>0){
				bets_pid[bet_index] = *((pid_t*)buffer);
				bets[bet_index] = *((int*)buffer + sizeof(pid_t*));
				
				fprintf(stdout,"Mr.[%d]. Bet: [%d] \n", 
							bets_pid[bet_index], bets[bet_index]);
				fprintf(stderr,"[%d] After collecting single bet \n", 
							getpid());
			}

			if(TEMP_FAILURE_RETRY(close(fifo))<0) ERR("close");
			bet_index++;
		}
		int result = rand()%(5-1) + 1;
		
		fprintf(stdout,"The result is: %d \n", result);

		int i = 0;
		for(i = 0;i<MAX_BETS;i++)
		{
			if(bets[i] == result) {
				kill(bets_pid[i], SIGUSR1);
			}
			else
				kill(bets_pid[i], SIGUSR2);
		}
	}
}

void usage()
{
	fprintf(stderr,"server fifo \n");
}

int main(int argc, char** argv)
{
	if(argc != 2){
		usage();
		exit(EXIT_FAILURE);
	}
	sethandler(sigpipe_handler, SIGPIPE);

	if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)<0)
		if(errno!=EEXIST) ERR("mkfifo");

	game(argv[1]);

	if(unlink(argv[1])<0) ERR("unlink");
	
	return(EXIT_SUCCESS);
}
