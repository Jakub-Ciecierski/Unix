#include "unix.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>

#define MAX_LETTER 122
#define MIN_LETTER 97

#define MAX_SLEEP_MS 2000
#define MIN_SLEEP_MS 100

#define MILLI 1000
#define NANO 1000000000

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
	}
}

void usage()
{
	fprintf(stderr,"main n1, ... , nk\n" );
}

void child_send(char* bufferA, char* bufferB, int pipeA[], int pipeB[])
{
	
	fprintf(stderr,"[%d] writing to pipes \n", getpid());

	if(bulk_write(pipeA[1],bufferA,PIPE_BUF)<0) ERR("bulk_write()");
	if(bulk_write(pipeB[1],bufferB,PIPE_BUF)<0) ERR("bulk_write()");
	
}

void child_work(int id, int n,int pipeA[], int pipeB[])
{
	fprintf(stderr,"[%d] Child work my number : %d \n" , getpid(), n);
	srand(getpid());

	int t = rand()%(MAX_SLEEP_MS - MIN_SLEEP_MS) + MIN_SLEEP_MS;
	int sec = t / MILLI;
	int msec = t%MILLI;
	int nanosec = msec * (NANO/MILLI);

	struct timespec req;
	req.tv_sec = sec;
	req.tv_nsec = nanosec;
	
	fprintf(stderr,"t: [%d], sec: [%d], msec: [%d], nsec: [%d]\n" ,
					t, sec, msec, nanosec);
	
	while(nanosleep(&req,&req) < 0)	
	{
		if(errno != EINTR){
			ERR("nanosleep()");
		}
	}
	
	// create block
	char block[PIPE_BUF];
	int i = 0;
	for(i = 0;i < n; i++)
	{
		char c = rand()%(MAX_LETTER - MIN_LETTER) + MIN_LETTER; // create small latters
		block[i] = c;
	}
	fprintf(stderr,"[%d] block: %s \n", getpid(), block);

	char bufferA[PIPE_BUF];
	*((int*)bufferA) = n;
	*((int*)bufferA + sizeof(int*)) = id;
	
	child_send(bufferA, block, pipeA, pipeB);
}

void parent_parse_message(char* bufferA,char* bufferB)
{
	int id = *((int*)bufferA + sizeof(int*));
	
	char* start_syntax = "<block from subprocess";
	char* end_syntax = "</block>";
	
	fprintf(stdout,"%s %d> %s %s \n",
				start_syntax, id, bufferB, end_syntax);
}

void parent_work(int pipeA[], int pipeB[])
{
	fprintf(stderr,"[%d] Parent work\n" , getpid());
		
	// close write end of pipes
	if(TEMP_FAILURE_RETRY(close(pipeA[1])) <0 ) ERR("close");
	if(TEMP_FAILURE_RETRY(close(pipeB[1])) <0 ) ERR("close");
	
	char bufferA[PIPE_BUF];
	char bufferB[PIPE_BUF];
	int countA = 0;
	int countB = 0;
	do{
		countA=bulk_read(pipeA[0],bufferA,PIPE_BUF);
		countB=bulk_read(pipeB[0],bufferB,PIPE_BUF);

		if(countA<0 || countB<0) ERR("bulk_read()");
		if(countA < PIPE_BUF) memset(bufferA+countA,0,PIPE_BUF-countA);
		if(countB < PIPE_BUF) memset(bufferB+countB,0,PIPE_BUF-countB);

		if(countA>0 && countB>0){
			parent_parse_message(bufferA,bufferB);
		}
	}while(countA!=0 || countB!=0); // while the pipe is open on write ends
	
	// close write end of pipes
	if(TEMP_FAILURE_RETRY(close(pipeA[0])) <0 ) ERR("close");
	if(TEMP_FAILURE_RETRY(close(pipeB[0])) <0 ) ERR("close");
}

void create_children(int n, char** argv)
{	
	int pipeA[2];
	if(pipe(pipeA) < 0 )
		ERR("pipe()");

	int pipeB[2];
	if(pipe(pipeB) < 0 )
		ERR("pipe()");

	int i = 0;
	for(i = 0;i < n;i++)
	{
		pid_t pid = fork();
		if(pid == 0)
		{
			int num; 
			num = atoi(argv[i+1]);
			if( num < 1 || num > 100){
				usage();
				kill(0, SIGKILL);
			}
			if(TEMP_FAILURE_RETRY(close(pipeA[0])) <0 ) ERR("close");
			if(TEMP_FAILURE_RETRY(close(pipeB[0])) <0 ) ERR("close");
			child_work(i+1, num, pipeA, pipeB);
			if(TEMP_FAILURE_RETRY(close(pipeA[1])) <0 ) ERR("close");
			if(TEMP_FAILURE_RETRY(close(pipeB[1])) <0 ) ERR("close");
			exit(EXIT_SUCCESS);
		}
		if(pid < 0)
			ERR("fork");
	}
	parent_work(pipeA, pipeB);
}

int main(int argc, char** argv)
{
	if(argc < 2){
		usage();
		exit(EXIT_FAILURE);
	}
	
	if(sethandler(sigchld_handler, SIGCHLD) < 0) ERR("sethandler");
	if(sethandler(SIG_IGN, SIGPIPE) < 0) ERR("sethandler");

	create_children(argc-1, argv);
	
	while (TEMP_FAILURE_RETRY(wait(NULL)) > 0);
	
	return(EXIT_SUCCESS);
}
