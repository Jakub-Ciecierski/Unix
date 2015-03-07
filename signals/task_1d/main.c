/*
Write a program creating n child processes. 
Child processes are numbered i=(0..n-1). 
Child process selects random digit d=(0..9) 
and sends SIGRTMIN+i to parent process exactly d-times with 1ms break 
between signals.

Parent program counts receiving signals (each type separately) 
until any of child processes is alive. Prints results and exits.

Sample output for n = 3:

SIGRTMIN+0 -> 3
SIGRTMIN+1 -> 1
SIGRTMIN+2 -> 7

Program takes sole positive integer argument 32 > n > 0 
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

volatile sig_atomic_t last_signal = 0;

volatile sig_atomic_t child_count = 0;

volatile sig_atomic_t counter = 0;

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}

void sigchld_handler(int sig)
{
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(0==pid) return;
		if(0>pid) {
			if(ECHILD==errno) return;
			ERR("waitpid:");
		}
		child_count--;
	}
}

void sigrt_handler(int sig)
{
	last_signal = sig;
	counter++; // just for test number of received signals
	fprintf(stderr,"[%d] Received %d signals \n",getpid(), counter);
}

void child_work(int i)
{
	srand(getpid());
	int d = rand()%10;
	fprintf(stdout,"[%d] Selected d: %d \n", getpid(), d);
	
	int k = 0;
	for(k = 0; k < d; k++)
	{		
		if(kill(getppid(), SIGRTMIN+i) < 0) ERR("kill()");

		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 1000000;
		
		while(nanosleep(&req,&req) < 0)	
			if(errno != EINTR) ERR("nanosleep()");
	}
}

void parent_work(int* counters, int n, sigset_t old_mask)
{
	child_count = n;
	int c = 0;
	while(1)
	{
		last_signal = 0;
		sigsuspend(&old_mask);
		
		// sigsuspend doesn't always unpause 
		c++;
		fprintf(stderr,"[%d] unpaused: %d \n", getpid(), c);

		if(last_signal >= SIGRTMIN && last_signal <= SIGRTMAX) {
			fprintf(stderr,"[%d] Signal: %d \n", getpid(), last_signal);
			counters[last_signal - SIGRTMIN]++;
		}
		
		
		if(child_count == 0){
			break;
			/*
			int i = 0;			
			for(i = 0; i < n;i++){
				fprintf(stdout,"SIGRTMIN+%d -> %d \n", i, counters[i]);
			}
			*/
		}
	}
	
	fprintf(stdout,"\n");
	int i = 0;
	for(i = 0; i < n;i++)
		fprintf(stdout,"SIGRTMIN+%d -> %d \n", i, counters[i]);
	fprintf(stdout,"\nTerminating \n");
}

void usage()
{
	fprintf(stderr,"main n \n");
	fprintf(stderr,"n: number of children \n");
}

int main(int argc, char** argv)
{
	if(argc < 2){
		usage();
		exit(EXIT_FAILURE);
	}

	int n = atoi(argv[1]);
	if(n < 0 || n > 32) {
		usage();
		exit(EXIT_FAILURE);
	}

	if(sethandler(sigchld_handler, SIGCHLD) < 0)
		ERR("sethandler()");

	int counters[n];

	sigset_t mask;
	sigset_t old_mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGCHLD);
	sigprocmask(SIG_BLOCK,&mask,&old_mask);

	int i = 0;
	for(i = 0; i < n; i++)
	{
		// clear memory
		counters[i] = 0;

		if(sethandler(sigrt_handler, SIGRTMIN+i)) ERR("sethandler()");

		sigaddset(&mask,SIGRTMIN+i);
		sigprocmask(SIG_BLOCK,&mask,NULL);

		pid_t pid = fork();
		switch(pid)
		{
			case 0:
				child_work(i);
				exit(EXIT_SUCCESS);
			case -1:
				ERR("fork()");
		}
	}

	parent_work(counters, n, old_mask);
	
	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);

	exit(EXIT_SUCCESS);
}
