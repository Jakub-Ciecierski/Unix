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

volatile sig_atomic_t terr_count = 0;

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
	fprintf(stderr,"child died \n");
	pid_t pid;
	for(;;){
		
		pid=waitpid(0, NULL, WNOHANG);
		if(0==pid) return;
		if(0>pid) {
			if(ECHILD==errno) return;
			ERR("waitpid:");
		}
		if(pid > 0){
			terr_count--;
			fprintf(stderr,"child count: %d \n", terr_count);	
		}
	}
}

void sigrt_handler(int sig)
{
	last_signal = sig;
	fprintf(stderr,"SIGNAL RECEIVEC %d \n", sig);
}

void terrorist_work(int s)
{
	fprintf(stderr,"[%d] T: my signal s: %d \n", getpid(), s);

	last_signal = 0;

	srand(getpid());
	int t = rand()%11 + 10;

	int tt = 0;
	for(tt = t; tt > 0; tt =sleep(tt))
	{
		if(last_signal == s) {
			fprintf(stderr,"[%d] DISARMED\n", getpid());
			exit(EXIT_SUCCESS);
		}
			
	}
	fprintf(stdout,"[%d] KABOOM\n", getpid());
}

void sapper_work()
{
	srand(getpid());
	
	while(1)
	{
		int s =  rand()%(SIGRTMAX - SIGRTMIN +1) + (SIGRTMIN);	
		
		fprintf(stderr,"[%d] S. Sending sig: %d to all \n", getpid(), s);
		
		if(kill(0, s) < 0)
			ERR("kill()");
		
		// wait half second
		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 500000000;
		while(nanosleep(&req,&req) < 0)	
			if(errno != EINTR) ERR("nanosleep()");
	}
	
	fprintf(stderr,"[%d] S. My parent: %d \n", getpid(), getppid());
}

void main_work()
{
	// unblock just SIGCHLD
	sigset_t child_mask;
	sigemptyset(&child_mask);
	sigaddset(&child_mask,SIGCHLD);
	sigprocmask(SIG_UNBLOCK,&child_mask,NULL);
	
	// Main
	while(1)
	{	
		pause();
		if(terr_count == 0) {
			fprintf(stderr,"KILLING \n");
			kill(0, SIGKILL);
			exit(EXIT_SUCCESS);
		}
	}
}

void create_terrorist(int n)
{
	// Creating Terrorists
	int i = 0;
	for(i = 0; i < n; i++)
	{
		pid_t pid = fork();
		if(pid == 0) {
			srand(getpid());
			int s =  rand()%(SIGRTMAX - SIGRTMIN +1) + (SIGRTMIN);

			if(sethandler(sigrt_handler, s) < 0)
				ERR("sethandler()");

			sigset_t mask;
			sigemptyset(&mask);
			sigaddset(&mask,s);
			sigprocmask(SIG_UNBLOCK,&mask,NULL);

			terrorist_work(s);
			exit(EXIT_SUCCESS);
		}
		if(pid < 0 )
			ERR("fork())");		
	}
}

void create_sapper()
{
	// Creating Sapper
	switch(fork())
	{
		case 0:
			sapper_work();
			exit(EXIT_SUCCESS);
		case -1:
			ERR("fork())");
	}
}

void usage()
{
	fprintf(stderr,"main n \n");
	fprintf(stderr,"n: number of terorists\n");
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		usage();
		exit(EXIT_FAILURE);
	}

	int n=atoi(argv[1]);
	if(n<=0) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if(sethandler(sigchld_handler, SIGCHLD) < 0)
		ERR("sethandler()");

	terr_count = n;

	sigset_t mask;
	sigset_t old_mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGCHLD);
	
	int i = 0;
	for(i = SIGRTMIN; i <= SIGRTMAX; i++) {
		sigaddset(&mask,i);
	}
	sigprocmask(SIG_BLOCK,&mask,&old_mask);
	
	create_terrorist(n);
	
	create_sapper();

	main_work();

	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);
	
	exit(EXIT_SUCCESS);
}
