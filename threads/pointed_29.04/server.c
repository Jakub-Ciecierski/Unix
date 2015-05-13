#include "macros.h"
#include "io.h"
#include "sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>

#define BACKLOG 3
#define MSG_SIZE 100

volatile sig_atomic_t do_continue = 1;

typedef struct
{
	int n;
	int socket;
	pthread_cond_t* cond;
	pthread_mutex_t* mutex;
} thread_arg;

void sig_handler(int sig)
{
	fprintf(stderr,"[Server] Signal \n");
	do_continue = 0;
}

void cleanup(void *arg)
{
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}

void* thread_func(void* args)
{
	fprintf(stderr,"[Server-Thread] Thread initiated \n");

	if(pthread_detach(pthread_self()) < 0 ) ERR("pthread_detach");

	thread_arg targ;
	memcpy(&targ, args, sizeof(targ));
	
	char msg[MSG_SIZE];
	snprintf(msg, MSG_SIZE,"Limit of %d connections reached, disconnecting", targ.n);
	printf("%s \n",msg);

	pthread_cleanup_push(cleanup, (void *) targ.mutex);
	if (pthread_mutex_lock(targ.mutex) != 0)
		ERR("pthread_mutex_lock");

	if (pthread_cond_wait(targ.cond, targ.mutex) != 0)
		ERR("pthread_cond_wait");
	
	pthread_cleanup_pop(1);
	
	if(bulk_write(targ.socket,msg,MSG_SIZE)<0) ERR("bulk_write()");
	
	if(TEMP_FAILURE_RETRY(close(targ.socket))<0)ERR("close");
	
	fprintf(stderr,"[Server-Thread] Thread finishing... \n");
	return NULL;
}

int add_new_client(int sfd){
	int fd;	
	if((fd=TEMP_FAILURE_RETRY(accept(sfd,NULL,NULL)))<0){
		if(EAGAIN==errno||EWOULDBLOCK==errno) return -1;
		ERR("accept");
	}
	return fd;
}

void create_thread(thread_arg targ, pthread_cond_t* cond, 
				pthread_mutex_t *mutex, int socket, int n)
{
	targ.n = n;
	targ.socket = socket;
	targ.cond = cond;
	targ.mutex = mutex;

	pthread_t thread;
	if (pthread_create(&thread, NULL, thread_func, (void *) &targ) != 0)
		ERR("pthread_create");
}

void server_work(int fdT, pthread_mutex_t* mutex, pthread_cond_t* cond, int n)
{	
	int cfd;
	fd_set base_rfds, rfds;
	FD_ZERO(&base_rfds);
	FD_SET(fdT, &base_rfds);
	
	int curr_thread_count = 0;

	thread_arg targ[n];
	
	while(do_continue){
		rfds = base_rfds;
		fprintf(stderr,"[Server] starting pselect \n");
		if(pselect(fdT+1,&rfds,NULL,NULL,NULL,NULL)>0){
			fprintf(stderr,"[Server] new tcp client \n");
			if((cfd = add_new_client(fdT)) < 0) continue;
			
			// start of critical section
			if (pthread_mutex_lock(mutex) != 0) ERR("pthread_mutex_lock");
			
			if((++curr_thread_count) == n){
				if (pthread_cond_broadcast(cond) != 0)
					ERR("pthread_cond_broadcast");
				do_continue = 0;
			}
			else{
				create_thread(targ[curr_thread_count], cond, mutex, cfd, n);
			}
			
			// end of critical section
			if (pthread_mutex_unlock(mutex) != 0)
					ERR("pthread_mutex_unlock");
			
		}else{
			if(EINTR==errno) continue;
			ERR("pselect");
		}
	}
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s number port\n",name);
}

void pcleanup(pthread_mutex_t *mutex, pthread_cond_t *cond)
{
	if (pthread_mutex_destroy(mutex) != 0)
		ERR("pthread_mutex_destroy");
	if (pthread_cond_destroy(cond) != 0)
		ERR("pthread_cond_destroy");
}

int main(int argc, char** argv) {
	int fdT;
	int new_flags;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	if(argc!=3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	int n=atoi(argv[1]);
	if(n<=0) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// set handlers
	if(sethandler(SIG_IGN, SIGPIPE)<0) ERR("sethandler");

	// TCP inet socket
	fdT=bind_inet_socket(atoi(argv[2]),SOCK_STREAM, BACKLOG);
	new_flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);

	// do work
	server_work(fdT, &mutex, &cond, n);
	
	// clean up
	if(TEMP_FAILURE_RETRY(close(fdT))<0)ERR("close");
	pcleanup(&mutex, &cond);
	
	fprintf(stderr,"Server has terminated.\n");
	
	return EXIT_SUCCESS;
}
