/*
Write an application that creates a process tree
of height h with height being defined as distance
from node to any leaf.
Each non-leaf node should have n children.

The root node sends SIGUSR1 and 
SIGUSR2 signals interchangeably, every one second, to
its children (only children, not all descendants).
 
Nodes forward any delivered signals to their children 
respectively.

Should a leaf node receive a SIGUSR1
signal and in case its PID is odd, it has 50% chance 
of dying. 

Same rule applies to leaf nodes regarding 
SIGUSR2 and even PIDs. 

Each process should track the 
number of its children. 

Any process with all its
children dead should end its life as well.

All non-leaf processes display their current number of children 
before or after each iteration.

C-c does not prevent 
the application from running.

0 < n < 6, 0 < h < 6 are command line parameters.
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

volatile sig_atomic_t last_signal = 0;

volatile sig_atomic_t children_count = 0;


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
	last_signal = sig;
	while(waitpid(0, NULL, WNOHANG) > 0)
		children_count--;
}

void sigusr_handler(int sig)
{
	last_signal = sig;
}

void root_node_work(int* children_pid, int n)
{
	fprintf(stdout,"R[%d] I'm root node \n",getpid());

	int current_signal = 0;

	while(1)
	{
		fprintf(stdout,"R[%d] Children count: %d \n",getpid(), children_count);
		if(children_count == 0)
		{
			fprintf(stdout,"R[%d] Time to end the app \n",getpid());
			exit(EXIT_SUCCESS);
		}
		int i = 0;
		for(i = 0; i < n; i++)
		{
			if(current_signal == 0) {
				if(kill(children_pid[i], SIGUSR1) < 0)
					if(errno == ESRCH) // allow ESRCH
						fprintf(stderr,"kill() error: %d \n",children_pid[i]);
				fprintf(stdout,"R[%d] Sending SIGUSR1 \n",getpid());
			}
			if(current_signal == 1) {
				if(kill(children_pid[i], SIGUSR2) < 0)
					if(errno == ESRCH) // allow ESRCH
						fprintf(stderr,"kill() error: %d \n",children_pid[i]);
				fprintf(stdout,"R[%d] Sending SIGUSR2 \n",getpid());
			}
		}
		if(current_signal == 0)
			current_signal = 1;
		else
			current_signal = 0;

		sleep(3);
	}
}

void internal_node_work(int* children_pid, int n)
{
	fprintf(stdout,"[%d] I'm internal node \n",getpid());

	last_signal = 0;
	
	sigset_t mask;	
	sigset_t old_mask;	
	sigemptyset(&mask);
	sigaddset(&mask,SIGUSR1);
	sigaddset(&mask,SIGUSR2);
	sigaddset(&mask,SIGCHLD);
	sigprocmask(SIG_BLOCK,&mask,&old_mask);

	while(1)
	{
		sigsuspend(&old_mask);

		if(last_signal == SIGCHLD)
		{
			fprintf(stdout,"I[%d] Children count: %d \n",getpid(), children_count);
			if(children_count == 0)
			{
				fprintf(stdout,"I[%d] It's time to die\n",getpid());
				exit(EXIT_SUCCESS);
			}
		}
		else 
		{
			int i = 0;
			for(i = 0; i < n; i++)
			{
				kill(children_pid[i], last_signal);
			}
		}
	}
}

void leaf_node_work()
{
	srand(getpid());
	int die = rand()%2;

	fprintf(stdout,"[%d] I'm leaf node\n",getpid());

	last_signal = 0;

	sigset_t mask;	
	sigset_t old_mask;	
	sigemptyset(&mask);
	sigaddset(&mask,SIGUSR1);
	sigaddset(&mask,SIGUSR2);
	sigaddset(&mask,SIGCHLD);
	sigprocmask(SIG_BLOCK,&mask,&old_mask);

	while(1)
	{
		sigsuspend(&old_mask);

		die = rand()%2;
		if(last_signal == SIGUSR1 && getpid()%2 != 0)
		{
			if(die == 0)
			{
				fprintf(stderr,"L[%d] It's time to die... \n",getpid());
				exit(EXIT_SUCCESS);
			}
		}
		if(last_signal == SIGUSR2 && getpid()%2 == 0)
		{
			if(die == 0)
			{
				fprintf(stderr,"L[%d] It's time to die... \n",getpid());
				exit(EXIT_SUCCESS);
			}
		}
	}
}

void create_children(int n,int h, int level, int* children_pid)
{
	int i = 0;

	if(level < h) 
	{
		for(i = 0;i < n; i++)
		{
			int pid = fork(); 
			switch (pid)
			{
				case 0:
					create_children(n, h, level + 1, children_pid);
					return;
				case -1:
					error("fork()\n");
					kill(0, SIGKILL);
			}
			children_pid[i] = pid;
		}

		if(level == 0)
			root_node_work(children_pid, n);
		else
			internal_node_work(children_pid, n);
	}
	
	else
	{
		leaf_node_work();
	}
}

void usage()
{
	fprintf(stderr,"tree n k \n");
	fprintf(stderr,"n: number of children of each internal node \n");
	fprintf(stderr,"h: height of the process tree \n");
}

int main(int argc, char** argv)
{
	if(argc != 3){
		usage();
		return EXIT_FAILURE;
	}

	int n = 0;
	int h = 0;

	if(sscanf(argv[1],"%d",&n) != 1) {
		usage();
		return EXIT_FAILURE;
	}
	if(sscanf(argv[2],"%d",&h) != 1) {
		usage();
		return EXIT_FAILURE;
	}

	int children_pid[n];
	
	fprintf(stdout,"n: %d \n",n);
	fprintf(stdout,"h: %d \n",h);

	children_count = n;
	
	if(sethandler(sigusr_handler, SIGUSR1) < 0)
	{
		kill(0, SIGKILL);
	}
	if(sethandler(sigusr_handler, SIGUSR2) < 0)
	{
		kill(0, SIGKILL);
	}

	if(sethandler(sigchld_handler, SIGCHLD) < 0 )
	{
		kill(0,SIGKILL);
	}

	if(sethandler(SIG_IGN, SIGINT) < 0)
	{
		kill(0,SIGKILL);
	}
	
	int level = 0;
	create_children(n, h, level, children_pid);

	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);
	
	return EXIT_SUCCESS;
}
