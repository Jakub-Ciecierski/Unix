#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // fork()
#include <sys/wait.h> //waitpid()
#include <errno.h> // error constants
#include <string.h> // memset()

void child_work(int l);
void create_children(int n, int l);
void parent_work(int k, int p);
int sethandler( void (*f)(int), int sigNo);
void child_handler(int sig);
void sigchld_handler(int sig);
void usage(void);

// sig_atomic_t:
// An integer type which can be accessed 
// as an atomic entity even in the presence of
// asynchronous interrupts made by signals.
volatile sig_atomic_t last_signal = 0;

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}

void child_handler(int sig) {
	printf("[%d] received signal %d\n", getpid(), sig);
	last_signal = sig;
}

void sigchld_handler(int sig) {
	fprintf(stdout,"Parent received a signal: %d \n", sig);

	pid_t pid;
	for(;;){
		// waitpid(pid, status, options)
		// waits for child's status change
		// a) termination
		// b) stopped by signal
		// c) resumed by signal
		//
		// pid = 0 means that we wait for all children
		// within the same group as the caller.
		//
		// WNOHANG will cause waitpid to 
		// return immediately
		//
		// with WNOHANG, if atleast one child exists
		// but their states have not been changed yet,
		// waitpid will return 0, otherwise -1.
		//
		// In case a child has changed its state,
		// the ID of that child is returned.
		pid=waitpid(0, NULL, WNOHANG);

		if(pid==0) {
			return;
		}
		if(pid<=0) {
			if(errno==ECHILD) // No child processes
				return;
			perror("waitpid:");
			exit(EXIT_FAILURE);
		}
	}
}

void child_work(int l)
{
	int t,tt;

	srand(getpid());
	t = rand()%6+5; 

	while(l-- > 0){
		for(tt=t;tt>0;tt=sleep(tt));
		if (last_signal == SIGUSR1) 
			printf("Success [%d]\n", getpid());
		else 
			printf("Failed [%d]\n", getpid());
	}
	printf("[%d] Terminates \n",getpid());
}

void parent_work(int k, int p)
{
	int t ;
	for(;;) {
		for(t=k;t>0;t=sleep(t));
		if (kill(0, SIGUSR1)<0){
			perror("children_send");
			exit(EXIT_FAILURE);
		}
		for(t=p;t>0;t=sleep(t));
		if (kill(0, SIGUSR2)<0){
			perror("children_send");
			exit(EXIT_FAILURE);
		}
	}
}

void create_children(int n, int l)
{
	while(n-- > 0) 
	{
		switch(fork())
		{
			// child section
			case 0:
				if(sethandler(child_handler,SIGUSR1)) {
					perror("Seting child SIGUSR1:");
					exit(EXIT_FAILURE);
				}
				if(sethandler(child_handler,SIGUSR2)) {
					perror("Seting child SIGUSR2:");
					exit(EXIT_FAILURE);
				}
				child_work(l);
				exit(EXIT_SUCCESS);
			case -1:
				perror("Fork:");
				exit(EXIT_FAILURE);
		}
	}
}

void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"n - number of children\n");
	fprintf(stderr,"k - Interval before SIGUSR1\n");
	fprintf(stderr,"p - Interval before SIGUSR2\n");
	fprintf(stderr,"l - lifetime of child in cycles\n");
}

int main(int argc, char** argv)
{
	int n, k, p, l;
	pid_t pid;
	if(argc != 5) 
	{
		usage();
		return EXIT_FAILURE;
	}

	n = atoi(argv[1]);
	k = atoi(argv[2]);
	p = atoi(argv[3]);
	l = atoi(argv[4]);
	
	if (n<=0 || k<=0 || p<=0 || l<=0)
	{
		usage();
		return EXIT_FAILURE;
	}
	
	create_children(n,l);
	
	if(sethandler(SIG_IGN,SIGUSR1)) {
		perror("Seting parent SIGUSR1:");
		exit(EXIT_FAILURE);
	}
	
	if(sethandler(SIG_IGN,SIGUSR2)) {
		perror("Seting parent SIGUSR2:");
		exit(EXIT_FAILURE);
	}
	
	// SIGCHLD is sent to parent if child terminated
	if(sethandler(sigchld_handler, SIGCHLD)) {
		perror("Settig parent SIGCHILD: ");
		exit(EXIT_FAILURE);
	}

	parent_work(k, p);

	return EXIT_SUCCESS;

}
