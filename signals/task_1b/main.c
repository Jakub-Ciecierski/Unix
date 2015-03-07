/*
*DONE*
Write a program creating n "child" processes, 
each subprocess starts with specified integer parameter k. 

*DONE*
Child process sends it's "k" parameter with SIGRTMIN signal to 
parent process and sleeps for approximately k sec. 
Subprocess repeats this action in a loop. 

*DONE*
Parent process should print received integers immediately upon 
signal arrival. In the same time parent process in a loop prints "*" 
exactly every 1 second.
Program can be terminated at any moment with SIGINT (C-c).

*DONE*
Program takes n integer "k" parameters where k > 0
(non number parameters can be treated as 1), 
one parameter for one child.
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

int setaction( void (*f)(int, siginfo_t *, void *), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = f;
	act.sa_flags = SA_SIGINFO;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}

void sigrtmin_handler(int sig, siginfo_t *si, void *c)
{
	fprintf(stdout,"Received k: %d from[%d]\n",si->si_int, si->si_pid);
	
}

void child_work(int k)
{
	fprintf(stdout,"[%d] k: %d \n",getpid(), k);
	while(1)
	{
		union sigval sig_msg;
		sig_msg.sival_int = k;
		
		if(sigqueue(getppid(), SIGRTMIN, sig_msg) < 0)
		{
			ERR("kill()");
		}
		sleep(k);
	}
}

void parent_work()
{
	while(1)
	{
		fprintf(stdout,"* \n");

		struct timespec req;
		req.tv_sec = 1;
		while(nanosleep(&req,&req) < 0)	
		{
			if(errno != EINTR){
				ERR("nanosleep()");
			}
		}
	}
	
}

void usage()
{
	fprintf(stderr,"main k1 k2 ... k3 \n");
	fprintf(stderr,"k: child's private integer \n");
}

int main(int argc, char** argv)
{
	if(argc == 1)
	{
		usage();
		exit(EXIT_FAILURE);
	}

	sigset_t mask;		
	sigemptyset(&mask);
	sigaddset(&mask,SIGRTMIN);
	sigprocmask(SIG_BLOCK,&mask,NULL);

	// set handler
	if(setaction(sigrtmin_handler, SIGRTMIN) < 0)
		ERR("setaction");
	
	int i = 1;
	for(i = 1; i < argc;i++)
	{
		// parse k
		int k = 0;
		if(sscanf(argv[i],"%d",&k) != 1) {
			k = 1;
		}
		if(k <= 0){
			usage();
			kill(0,SIGKILL);
			exit(EXIT_FAILURE);
		}

		// create children
		pid_t pid = fork();
		switch(pid)
		{
			case 0:
				child_work(k);
				exit(EXIT_SUCCESS);
			case -1:
				ERR("fork()");
		}
	}
	
	sigprocmask(SIG_UNBLOCK,&mask,NULL);

	parent_work();

	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);
	
	exit(EXIT_SUCCESS);
}
