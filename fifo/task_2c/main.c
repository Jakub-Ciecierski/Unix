#include "unix.h"
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <limits.h>

volatile sig_atomic_t children_count;

volatile sig_atomic_t last_signal;

volatile sig_atomic_t can_start_new_round;

void sigchld_handler(int sig)
{
	last_signal = sig;
	can_start_new_round = 1;
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(0==pid) return;
		if(0>pid) {
			if(ECHILD==errno) return;
			ERR("waitpid:");
		}
		children_count--;
		fprintf(stderr,"Children_count :%d \n", children_count);
	}
}

void sigrt_handler(int sig)
{
	can_start_new_round = 1;
	fprintf(stderr,"Receieved SIG_RT, can continue \n");
}

void child_work(int round_pipe[], int result_pipe[], sigset_t old_mask)
{
	TEMP_FAILURE_RETRY(close(result_pipe[1]));
	TEMP_FAILURE_RETRY(close(round_pipe[0]));
	
	srand(getpid());
	int isWinner = 1;

	while(isWinner)
	{
		int r = rand()%20;
		fprintf(stdout,"[%d] Child, my number: %d \n", getpid(), r);

		// sends its r to parent
		char round_buffer[PIPE_BUF];
		*((pid_t *)round_buffer) = getpid();
		*((int *)(round_buffer + sizeof(pid_t))) = r;
		
		if(bulk_write(round_pipe[1], round_buffer, PIPE_BUF)<0) 
			ERR("bulk_write()");

		// wait for result
		char result_buffer[PIPE_BUF];
		pid_t winner_pid;

		int count = 0;
		fprintf(stderr,"[%d] Reading pipe \n", getpid());
		count=bulk_read(result_pipe[0],result_buffer,PIPE_BUF);
		fprintf(stderr,"[%d] After reading pipe \n", getpid());

		if(count<0) ERR("bulk_read()");
		if(count < PIPE_BUF) memset(result_buffer+count,0,PIPE_BUF-count);
		if(count>0){
			winner_pid = *((pid_t *)result_buffer);
		}

		if(winner_pid == getpid())
		{
			fprintf(stdout,"[%d] I'm the winner. Message from Parent:   %s \n",
					getpid(),
					result_buffer+sizeof(pid_t*));
					
			isWinner = 0;
		}
		else
		{
			while(can_start_new_round == 0)
			{
				sigsuspend(&old_mask);
			}
			can_start_new_round = 0;
		}
		
	}
	
	fprintf(stderr,"[%d] Closing... \n", getpid());
	
	TEMP_FAILURE_RETRY(close(result_pipe[0]));
	TEMP_FAILURE_RETRY(close(round_pipe[1]));
}

void parent_work(int round_pipe[], int result_pipe[], sigset_t old_mask)
{
	TEMP_FAILURE_RETRY(close(result_pipe[0]));
	TEMP_FAILURE_RETRY(close(round_pipe[1]));

	srand(getpid());

	while(children_count)
	{
		can_start_new_round = 0;
		
		pid_t pid;
		for(;;){
			pid=waitpid(0, NULL, WNOHANG);
			if(0==pid) break;
			if(0>pid) {
				if(ECHILD==errno) break;
				ERR("waitpid:");
			}
			children_count--;
		}

		int r = rand()%20;
		fprintf(stdout,"[%d] Parent, my number: %d \n", getpid(), r);

		int* results = (int*)malloc(sizeof(int) * children_count);
		pid_t* results_pid = (pid_t*)malloc(sizeof(pid_t) 
									* children_count);

		int current_index = 0;
		char round_buffer[PIPE_BUF];

		int count = 0;
		fprintf(stderr,"[%d]P Current children_count %d \n", getpid(),children_count);
		do{
			fprintf(stderr,"[%d]P Reading pipe \n", getpid());
			count=bulk_read(round_pipe[0], round_buffer, PIPE_BUF);
			fprintf(stderr,"[%d]P After reading pipe \n", getpid());

			if(count<0) ERR("bulk_read()");
			if(count < PIPE_BUF) memset(round_buffer+count,0,PIPE_BUF-count);
			if(count>0){
				results_pid[current_index] = *((pid_t *)round_buffer);
				results[current_index] = *((int *)(round_buffer 
											+ sizeof(pid_t)));
			}
			current_index++;
			
		}while(current_index < children_count);

		// Find the winner
		int i = 0;
		int min_index = 0;
		int min_diff = results[min_index] - r;
		if(min_diff < 0)
			min_diff *= -1;

		for(i = 0;i < children_count; i++)
		{
			int diff = results[i] - r;
			if(diff < 0){
				diff *= -1;
			}
			if(min_diff > diff){
				min_diff = diff;
				min_index = i;
			}
		}
		fprintf(stdout,"The winner with with dif: %d is: %d \n",
					min_diff, results_pid[min_index]);

		// send the results to children
		char result_buffer[PIPE_BUF];
		*((pid_t*)result_buffer) = results_pid[min_index];

		char* result_message = (result_buffer + sizeof(pid_t*));
		char* msg = "Congratulations to the winner !!!";
		i = 0;
		char c;
		while((c = msg[i]) != '\0'){
			result_message[i++] = c;
		}
		
		int send_index = 0;
		while(send_index++ < children_count)
		{
			fprintf(stderr,"[%d]P Sending results \n", getpid());
			if(bulk_write(result_pipe[1], result_buffer, PIPE_BUF)<0) 
				ERR("bulk_write()");
		}

		fprintf(stdout,"\n *******  ROUND FINISHED %d more to go ! ******* \n\n", 
					children_count-1);
		last_signal = 0;
		while(last_signal != SIGCHLD)
			sigsuspend(&old_mask);
		
		kill(0, SIGRTMIN);
		
		fprintf(stdout," LET'S CONTINUE \n");
	}
	
	fprintf(stdout,"\n ******* GAME FINISHED ******* \n");
	
	TEMP_FAILURE_RETRY(close(result_pipe[1]));
	TEMP_FAILURE_RETRY(close(round_pipe[0]));
}

void create_children(int round_pipe[], int result_pipe[], int n)
{
	sigset_t mask;	
	sigset_t old_mask;	
	sigemptyset(&mask);
	sigaddset(&mask,SIGCHLD);
	sigaddset(&mask,SIGRTMIN);
	sigprocmask(SIG_BLOCK,&mask,&old_mask);

	while(n-- > 0)
	{
		switch(fork())
		{
			case 0:
				child_work(round_pipe, result_pipe, old_mask);
				exit(EXIT_SUCCESS);
			case -1:
				ERR("fork()");
		}
	}
	
	parent_work(round_pipe, result_pipe, old_mask);
}

void usage()
{
	fprintf(stderr,"main n \n");
	fprintf(stderr,"n: number of children \n");
	fprintf(stderr,"redirect stderr for clear output \n");
}

int main(int argc, char** argv)
{
	if(argc != 2){
		usage();
		exit(EXIT_FAILURE);
	}
	
	int MAX_CHILD = 0;
	if((MAX_CHILD = atoi(argv[1])) < 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	children_count = MAX_CHILD;

	sethandler(sigchld_handler, SIGCHLD);
	sethandler(sigrt_handler, SIGRTMIN);
	
	// this pipe is used for children to submit their number
	int round_pipe[2]; 
	// this pipe is used for the parent to send results
	int result_pipe[2];

	if(pipe(round_pipe) < 0 )
		ERR("pipe()");
	if(pipe(result_pipe) < 0 )
		ERR("pipe()");
	
	create_children(round_pipe, result_pipe, MAX_CHILD);
	
	return EXIT_SUCCESS;
}
