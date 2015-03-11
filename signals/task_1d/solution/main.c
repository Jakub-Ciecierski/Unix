/******************************************************************************
Marcin Borkowski
marcinbo (at) mini (dot) pw (dot) edu (dot) pl
********************************************************************************/

#define _GNU_SOURCE 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))
#define SIGDELAY 1
#define NSMULT 1000000L


volatile sig_atomic_t last_signal = 0;
volatile sig_atomic_t children = 0 ;

volatile sig_atomic_t counter = 0 ;

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void sig_handler(int sig) {
	last_signal = sig;
	fprintf(stderr,"sig_handler counter: %d \n", counter++);
}

void sigchld_handler(int sig) {
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(pid>0) children--;
		if(0==pid) return;
		if(0>=pid) {
			if(ECHILD==errno) return;
			ERR("waitpid:");
		}
	}
}

void child_work(int i) {
	struct timespec t, tn = {0,0};
	int d;
	tn.tv_nsec=SIGDELAY*NSMULT;
	srand(time(NULL)*getpid());	
	d=rand()%10;
	fprintf(stderr,"CREATED %d(%d) with digit  %d\n",getpid(),i,d);

	for (;d>0;d--) {
		if(kill(getppid(),SIGRTMIN+i)<0) ERR("kill:");
		for(t=tn;nanosleep(&t,&t);)
			if(EINTR!=errno) ERR("nanosleep:");
	}
	exit(EXIT_SUCCESS);
}

void parent_work(sigset_t oldmask, int n) {
	int *sigs;
	int i;
	int c = 0;
	if((sigs=(int*)malloc(n*sizeof(int)))==NULL) ERR("malloc:");
	while(1){ // should be (children)
		last_signal=0;
		sigsuspend(&oldmask);
		fprintf(stderr,"sigsuspend counter: %d \n", c++);
		i=last_signal-SIGRTMIN;
		if(i>=0&&i<n)
			sigs[i]++;
			
		if(children == 0)
		{
			for(i=0;i<n;i++)
				printf("SIGRTMIN+%d -> %d\n",i,sigs[i]);
		}
	}
	for(i=0;i<n;i++)
		printf("SIGRTMIN+%d -> %d\n",i,sigs[i]);
	free(sigs);
}

void create_children(int n) {
	pid_t s;
	for (n--;n>=0;n--) {
		if((s=fork())<0) ERR("Fork:");
		if(!s) child_work(n);
		children++;
	}
}

void usage(char *name){
	fprintf(stderr,"USAGE: %s 0<n<32\n",name);
}

int main(int argc, char** argv) {
	sigset_t mask, oldmask;
	int n,i;
	if(argc<2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	n=atoi(argv[1]);
	if(n<=0||n>=32) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	sigemptyset (&mask);
	if(sethandler(sigchld_handler,SIGCHLD)) ERR("Setting SIGCHLD:");
	sigaddset (&mask, SIGCHLD);
	for(i=0;i<n;i++){
		if(sethandler(sig_handler,SIGRTMIN+i)) ERR("Setting SIGRTMIN+i:");
		sigaddset (&mask, SIGRTMIN+i);
	}	
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	create_children(n);
	parent_work(oldmask,n);
	sigprocmask (SIG_UNBLOCK, &mask, NULL);
	return EXIT_SUCCESS;
}
