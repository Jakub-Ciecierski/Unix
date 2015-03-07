#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (sigaction(sigNo, &act, NULL) == -1)
		return -1;
	return 0;
}

void sigint_handler(int sig)
{
	//printf("SIGINT received \n");
}

void child_work()
{
	if(sethandler(sigint_handler, SIGINT) < 0 )
	{
		kill(0,SIGKILL);
	}

	printf("[%d] Don't kill me please \n", getpid());
	for(;;);
}

void child_smart_work()
{
	printf("[%d] You can't kill me \n", getpid());
	for(;;);
}

int main(int argc, char** argv)
{

	pid_t pid = fork();
	if(pid == 0)
		child_work();
		
	pid = fork();

	if(pid == 0)
		child_smart_work();

	if(sethandler(sigint_handler, SIGINT) < 0 )
	{
		kill(0,SIGKILL);
	}

	int wait_return;
	while ((wait_return = wait(NULL)) > 0)
	{
		printf("wait() interrupted\n");
	}

	if(wait_return < 0)
		if(errno = EINTR)
			printf("EINTR \n");

	return EXIT_SUCCESS;
}
