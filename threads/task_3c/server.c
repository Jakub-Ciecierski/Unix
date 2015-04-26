/*
Write a tic tac toe server. Server's purpose is to pair players, 
provide synchronization during a game and check winning conditions. 
Server is a multi-threaded application - each game is handled by separate thread.
Server waits for clients to connect on TCP port given as a command line parameter. 
If there are two clients connected to a server the game between them 
begins in a separate thread. There can be maximum number 
MAX_PLAYERS = 10 of players connected to the server at the same time. 
No new connections should be accepted if this number is exceeded.

Game handling thread randomly chooses one of two players to begin the 
game and sends him information about it.
Then in the loop thread waits for player's answer 
which contains move he want to play.
If move is improper (e.g. field is already occupied or
field number is improper), player that made it loses the game 
immediately. Both players are informed about this situation,
game finishes and players wait for another game to begin.
If move causes player to win the game or game becomes a draw, 
both players are informed about this situation, game finishes
and players wait for another game to begin.
Otherwise updated board is send to another player ...

If client disconnects during the game, his opponent wins the game.

On SIGINT (C-c) server application: shouldn't accept any new players, 
should not start any new games, should terminate connection to any 
players waiting for a game. After all games are finished it should terminate. 

*/

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

#define MAX_PLAYERS 10
#define THREAD_NUM 5

volatile sig_atomic_t do_continue = 1;

typedef struct
{
	int* sock_p1;
	int* sock_p2;
	int* players_count;
	int* condition;
	pthread_cond_t* cond;
	pthread_mutex_t* mutex;
} game_arg;

void sig_handler(int sig)
{
	fprintf(stderr,"[Server] Signal \n");
	do_continue = 0;
}

void cleanup(void *arg)
{
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}

void tic_tac_toe(int player1, int player2)
{
	fprintf(stderr,"[Server-Game] Game starting... \n");
}

void* start_game(void* args)
{
	fprintf(stderr,"[Server-Game] Game thread initiated \n");

	game_arg garg;
	memcpy(&garg, args, sizeof(garg));
	
	int sock_p1;
	int sock_p2;
	
	while(do_continue)
	{
		pthread_cleanup_push(cleanup, (void *) garg.mutex);
		if (pthread_mutex_lock(garg.mutex) != 0)
			ERR("pthread_mutex_lock");
		while (!*garg.condition && do_continue)
			if (pthread_cond_wait(garg.cond, garg.mutex) != 0)
				ERR("pthread_cond_wait");
		*garg.condition = 0;
		if (!do_continue)
			pthread_exit(NULL);
		
		sock_p1 = *garg.sock_p1;
		sock_p2 = *garg.sock_p2;
		pthread_cleanup_pop(1);
		
		tic_tac_toe(sock_p1, sock_p2);
		
		// decrease player count - critical section
		pthread_cleanup_push(cleanup, (void *) garg.mutex);
		if (pthread_mutex_lock(garg.mutex) != 0)
			ERR("pthread_mutex_lock");
		*garg.players_count -= 2;
		pthread_cleanup_pop(1);
		
	}
	fprintf(stderr,"[Server-Game] Thread finishing... \n");
	return NULL;
}

void init_thread_pool(pthread_t* threads, game_arg* garg, pthread_cond_t* cond, 
				pthread_mutex_t *mutex, int* players_count, 
				int* sock_p1,int* sock_p2, int* condition)
{
	int i;
	for (i = 0; i < THREAD_NUM; i++)
	{
		garg[i].cond = cond;
		garg[i].mutex = mutex;
		garg[i].players_count = players_count;
		garg[i].sock_p1 = sock_p1;
		garg[i].sock_p2 = sock_p2;
		garg[i].condition = condition;
		if (pthread_create(&threads[i], NULL, start_game, (void *) &garg[i]) != 0)
			ERR("pthread_create");
	}
}

int add_new_client(int sfd){
	int fd;	
	if((fd=TEMP_FAILURE_RETRY(accept(sfd,NULL,NULL)))<0){
		if(EAGAIN==errno||EWOULDBLOCK==errno) return -1;
		ERR("accept");
	}
	return fd;
}

void server_work(int fdT,sigset_t* oldmask, pthread_mutex_t* mutex, 
			pthread_cond_t* cond, game_arg* garg,
			int* condition, int* sock_p1, int* sock_p2, int* players_count)
{	
	int cfd;
	fd_set base_rfds, rfds;
	FD_ZERO(&base_rfds);
	FD_SET(fdT, &base_rfds);
	
	fprintf(stderr,"[Server] starting work \n");
	
	while(do_continue){
		rfds = base_rfds;
		fprintf(stderr,"[Server] starting pselect \n");
		if(pselect(fdT+1,&rfds,NULL,NULL,NULL,oldmask)>0){
			fprintf(stderr,"[Server] new tcp client \n");
			if((cfd = add_new_client(fdT)) < 0) continue;
			
			// start of critical section
			if (pthread_mutex_lock(mutex) != 0) ERR("pthread_mutex_lock");
			(*players_count)++;
			
			// if player count limit is reached, close connection
			if(*players_count == MAX_PLAYERS){
				fprintf(stderr,"[Server] Max players reached, closing connection \n");
				if (TEMP_FAILURE_RETRY(close(cfd)) == -1) ERR("close");
			}else{
				if(*players_count%2 == 1){
					fprintf(stderr,"[Server] Player 1 joined lobby \n");
					*sock_p1 = cfd;
				} else{
					fprintf(stderr,"[Server] Player 2 joined lobby \n");
					*sock_p2 = cfd;
					
					// start game
					*condition = 1;
					if (pthread_cond_signal(cond) != 0)
					ERR("pthread_cond_signal");
				}
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
	fprintf(stderr,"USAGE: %s port\n",name);
}

void pcleanup(pthread_t* threads, pthread_mutex_t *mutex, pthread_cond_t *cond)
{
	int i;
	
	if (pthread_cond_broadcast(cond) != 0)
		ERR("pthread_cond_broadcast");
	
	for (i = 0; i < THREAD_NUM; i++)
		if (pthread_join(threads[i], NULL) != 0)
			ERR("pthread_join");
		
	if (pthread_mutex_destroy(mutex) != 0)
		ERR("pthread_mutex_destroy");
	if (pthread_cond_destroy(cond) != 0)
		ERR("pthread_cond_destroy");
}

int main(int argc, char** argv) {
	int fdT;
	int new_flags;
	int sock_p1, sock_p2, players_count = 0, condition = 0;
	pthread_t threads[THREAD_NUM];
	game_arg garg[THREAD_NUM];
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	
	if(argc!=2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	// set handlers
	if(sethandler(sig_handler, SIGINT)<0) ERR("sethandler");
	if(sethandler(SIG_IGN, SIGPIPE)<0) ERR("sethandler");
	sigset_t mask, oldmask;
	
	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);

	init_thread_pool(threads, garg, &cond, &mutex, &players_count, 
						&sock_p1, &sock_p2, &condition);
	
	// TCP inet socket
	fdT=bind_inet_socket(atoi(argv[1]),SOCK_STREAM, BACKLOG);
	new_flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);

	// do work
	server_work(fdT, &oldmask, &mutex, &cond, garg,
				&condition, &sock_p1, &sock_p2, &players_count);

	sigprocmask (SIG_UNBLOCK, &mask, NULL);
	
	// clean up
	if(TEMP_FAILURE_RETRY(close(fdT))<0)ERR("close");
	pcleanup(threads, &mutex, &cond);
	
	fprintf(stderr,"Server has terminated.\n");
	
	return EXIT_SUCCESS;
}
