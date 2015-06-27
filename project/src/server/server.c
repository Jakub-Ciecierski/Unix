#include "server.h"

volatile sig_atomic_t do_continue = 1;

void sig_handler(int sig)
{
	fprintf(stderr,"[Server] Signal \n");
	do_continue = 0;
}

int add_new_client(int sfd){
	int nfd;
	if((nfd=TEMP_FAILURE_RETRY(accept(sfd,NULL,NULL)))<0) {
		if(EAGAIN==errno||EWOULDBLOCK==errno) return -1;
		ERR("accept");
	}
	return nfd;
}

void connection_handler(int cfd)
{
	// start new process
	pid_t pid = fork();
	if(pid < 0) ERR("fork()");
	if(pid == 0)
	{
		// make it global / input parameter ??
		//char* path = "~/Public/programming/unix/project/src/server/connection";
		
		connection_init(cfd);
		
		// TODO wait for all children
		/*
		int const len = 10;
		char argv1[len];
		if(snprintf(argv1, len, "%d", cfd) < 0 ) ERR("snprintf");
		
		char* path = "./connection";
		if(execl(path, argv1) < 0) ERR("exec");
		*/
	}
	fprintf(stderr,"[Server] Server started new connection \n");
}

void server_work(int fdT)
{
	fprintf(stderr,"[Server] starting work \n");
	int cfd;
	fd_set base_rfds, rfds ;
	sigset_t mask, oldmask;
	
	FD_ZERO(&base_rfds);
	
	FD_SET(fdT, &base_rfds);
	
	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	
	while(do_continue){
		rfds=base_rfds;
		cfd=-1;
		fprintf(stderr,"[Server] starting pselect \n");
		if(pselect(fdT+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(fdT,&rfds)){
				fprintf(stderr,"[Server] new tcp client \n");
				cfd=add_new_client(fdT);
				if(cfd>=0)connection_handler(cfd);
			}

		}else{
			if(EINTR==errno) continue;
			ERR("pselect");
		}
	}
	sigprocmask (SIG_UNBLOCK, &mask, NULL);
}

/*************************************************/
/***************** INIT SECTION ******************/
/*************************************************/

/**
 * Initiates signal handler
 * */
void init_sig_handlers()
{
	// set handlers
	if(sethandler(sig_handler, SIGINT)<0) ERR("sethandler");
	if(sethandler(SIG_IGN, SIGPIPE)<0) ERR("sethandler");
}

/**
 * Initiates tcp connection
 * */
void init_tcp(int* fdT, int* new_flags, int port)
{
	// TCP inet socket
	*fdT=bind_inet_socket(port, SOCK_STREAM, BACKLOG);
	*new_flags = fcntl(*fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);
}

/**
 * Initiates database directory.
 * Does not create if it already exists
 * */
void init_dir()
{	
	if(mkdir(DB_DIR, 0777) < 0) 
		if(errno != EEXIST) ERR("mkdir");
	
	if(mkdir(DB_PLAYER_DIR, 0777) < 0) 
		if(errno != EEXIST) ERR("mkdir");
		
	if(mkdir(DB_GAME_DIR, 0777) < 0) 
		if(errno != EEXIST) ERR("mkdir");
	
	// switch to main database dir
	//if (chdir(DB_DIR) == -1) ERR("chdir");
}

/**
 * Used to initilize the server and all its components
 * */
void init_server(int port)
{
	int fdT;
	int new_flags;
	
	init_sig_handlers();

	init_tcp(&fdT, &new_flags, port);

	init_dir();

	// start server work
	server_work(fdT);
	
	// clean up
	if(TEMP_FAILURE_RETRY(close(fdT))<0)ERR("close");

	fprintf(stderr,"Server has terminated.\n");
}

