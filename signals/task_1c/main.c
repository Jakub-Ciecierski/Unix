/*
*DONE*
Write a program creating 2 processes - parent and child.
Child process at random intervals of 0.1s, 0.2s, 0.3s, 0.4s or 0.5s 
sends SIGRTMIN to parent process. 

Parent process with equal probability 50% 
send back SIGRTMIN or SIGRTMAX. 

Child must analyse the answer and print '-' for SIRGTMIN or '+' for SIGRTMAX.
This process continues in a loop. 
At each iteration child process can terminate with 1% probability. 

Parent process must immediately terminate when child process dies or 
after n-th iteration. 

Output from child process must 
be printed in one line (e.g. ++-+---++)

Program takes sole positive integer argument n > 0 
*/

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

void sigchld_handler(int sig) {
	kill(0, SIGKILL);
	fprintf(stderr,"Parent terminating \n");
	exit(EXIT_SUCCESS);
}

void parent_handler(int sig)
{
}

void child_handler(int sig)
{
	if(sig == SIGRTMIN)
		fprintf(stdout,"-");
	if(sig == SIGRTMAX)
		fprintf(stdout,"+");
		
	if(TEMP_FAILURE_RETRY(fflush(stdout))<0)ERR("fflush:");
}

void child_work(int n, sigset_t old_mask)
{
	fprintf(stderr,"[%d] Child working \n", getpid());
	if(sethandler(child_handler, SIGRTMIN) < 0)
		ERR("sethandler()");
	if(sethandler(child_handler, SIGRTMAX) < 0)
		ERR("sethandler()");

	srand(getpid());
	int count = 0;
	
	while(n-- > 0)
	{	
		if(kill(getppid(), SIGRTMIN) < 0) ERR("kill()");
		
		count++;
		fprintf(stderr,"[%d] Signals sent: %d \n", getpid(), count);
		
		// nanosleep
		int sleep_time = rand()%5;
		struct timespec req;
		req.tv_sec = 0;
		if(sleep_time == 0)
			req.tv_nsec = 500000000L;
		if(sleep_time == 1)
			req.tv_nsec = 400000000L;
		if(sleep_time == 2)
			req.tv_nsec = 300000000L;
		if(sleep_time == 3)
			req.tv_nsec = 200000000L;
		if(sleep_time == 4)
			req.tv_nsec = 100000000L;
		
		while(nanosleep(&req,&req) < 0)	
			if(errno != EINTR) ERR("nanosleep()");
			
		sigsuspend(&old_mask);
	}
}

void parent_work(pid_t pid,sigset_t old_mask)
{
	fprintf(stderr,"[%d] Parent working \n", getpid());
	if(sethandler(parent_handler, SIGRTMIN) < 0)
		ERR("sethandler()");
	
	srand(getpid());
	int count = 0;
	while(1)
	{
		int which_signal = rand()%2;
		
		sigsuspend(&old_mask);
		count++;
		fprintf(stderr,"[%d] Signals received: %d \n", getpid(), count);

		if(which_signal == 0)
			if(kill(pid, SIGRTMIN) < 0)
				ERR("kill()");
		if(which_signal == 1)
			if(kill(pid, SIGRTMAX) < 0)
				ERR("kill()");
	}
}

void usage()
{
	fprintf(stderr,"main n \n");
	fprintf(stderr,"n: maximum number of iterations \n");
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		usage();
		exit(EXIT_FAILURE);
	}
	
	int n = atoi(argv[1]);
	if(n < 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	if(sethandler(sigchld_handler, SIGCHLD) < 0)
		ERR("sethandler()");

	sigset_t mask;
	sigset_t old_mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGRTMIN);
	sigaddset(&mask,SIGRTMAX);
	sigprocmask(SIG_BLOCK,&mask,&old_mask);
	
	pid_t pid = fork();
	switch(pid)
	{
		case 0:
			child_work(n, old_mask);
			exit(EXIT_SUCCESS);
		case -1:
			ERR("fork()");
	}

	parent_work(pid, old_mask);

	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);

	exit(EXIT_SUCCESS);
}
