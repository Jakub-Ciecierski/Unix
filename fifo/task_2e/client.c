#include "unix.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define TIMEOUT 10

volatile sig_atomic_t last_signal;

void sig_handler(int sig)
{
	last_signal = sig;
}

void game(int fifo, sigset_t old_mask)
{
	last_signal = 0;
	srand(getpid());
	
	int bet = rand()%(5-1) + 1;
	fprintf(stdout,"[%d] I'm betting: %d \n", getpid(), bet);
	
	char buffer[PIPE_BUF];
	*((pid_t*)buffer) = getpid();
	*((int*)buffer + sizeof(pid_t*)) = bet;
	
	if(bulk_write(fifo,buffer,PIPE_BUF)<0) ERR("write");

	alarm(TIMEOUT);
	
	sigsuspend(&old_mask);
	if(last_signal == SIGUSR1)
		printf("[%d] WIN [%d]\n", getpid(), bet);
	if(last_signal == SIGUSR2)
		printf("[%d] LOOSE [%d]\n", getpid(), bet);
	if(last_signal == SIGALRM)
		printf("[%d] TIMEOUT [%d]\n", getpid(), bet);
}

void usage()
{
	fprintf(stderr,"client fifo \n");
}

int main(int argc, char** argv)
{
	if(argc != 2){
		usage();
		exit(EXIT_FAILURE);
	}
	sigset_t mask;	
	sigset_t old_mask;	
	sigemptyset(&mask);
	sigaddset(&mask,SIGUSR1);
	sigaddset(&mask,SIGUSR2);
	sigaddset(&mask,SIGALRM);
	sigprocmask(SIG_BLOCK,&mask,&old_mask);

	sethandler(sig_handler, SIGALRM);
	sethandler(sig_handler, SIGUSR1);
	sethandler(sig_handler, SIGUSR2);

	int fifo;

	if((fifo=TEMP_FAILURE_RETRY(open(argv[1],O_WRONLY)))<0) ERR("open");

	game(fifo, old_mask);

	if(TEMP_FAILURE_RETRY(close(fifo))<0) ERR("close");
	
	return(EXIT_SUCCESS);
}
