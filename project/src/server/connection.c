#include "connection.h"

volatile sig_atomic_t c_do_continue = 1;

int current_status = CMP_S_LOGGED_OUT;
char* login_name = "";

void change_status(int status)
{
	current_status = status;
}

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_login_handler(int fd, char* name)
{
	
}

void msg_register_handler(int fd, char* name)
{
	fprintf(stderr,"[Connection] User wants to register name: %s\n", name);
	
	char buffer[CMP_BUFFER_SIZE];

	if(db_create_player(name) < 0 ){
		// send deny msg
		fprintf(stderr, "[Connection] Name already exists\n");
		
		// send only header
		if(sprintf(buffer, "%s", CMP_REGISTER_REJ) < 0) ERR("sprintf");
	}
	else{
		fprintf(stderr, "[Connection] Registretion Successfull\n");
		
		change_status(CMP_S_LOGGED_IN);
		
		if(sprintf(buffer, "%s", CMP_REGISTER_ACC) < 0) ERR("sprintf");
	}
	
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
}

/**
 * Handles incomming messages
 * */
void msg_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	if(bulk_read(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_read");
	
	fprintf(stderr, "[Connection] after bulk_read: \n%s\n", buffer);
	
	// check the header
	char header[CMP_HEADER_SIZE];
	strncpy(header, buffer, CMP_HEADER_SIZE);
	// some trash might be left out
	if(memset(header+CMP_HEADER_SIZE, 0, CMP_HEADER_SIZE) < 0 ) ERR("memeset");

	fprintf(stderr, "[Connection] after cmp_get_header\n");
	
	/// REGISTER
	if (strcmp(header, CMP_REGISTER) == 0)
	{
		// get the message it self
		char* name = buffer + CMP_HEADER_SIZE;
		msg_register_handler(fd, name);
	} 
	/// LOGIN
	else if (strcmp(header, CMP_LOGIN) == 0)
	{

	}
	/// GAME
	else if (strcmp(header, CMP_GAME) == 0)
	{
	 
	}
	/// STATUS
	else if (strcmp(header, CMP_STATUS) == 0)
	{
	 
	}
	else
	{
		perror("unknown message");
		exit(EXIT_FAILURE);
	}
}

/**
 * Main work of connection
 * */
void connection_work(int cfd)
{
	fprintf(stderr, "[Connection] New connection has been established\n");
	
	fprintf(stderr,"[Connection] starting work \n");
	fd_set base_rfds, rfds ;
	sigset_t mask, oldmask;
	
	FD_ZERO(&base_rfds);
	
	FD_SET(cfd, &base_rfds);

	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	
	while(c_do_continue){
		rfds=base_rfds;

		fprintf(stderr,"[Connection] starting pselect \n");
		if(pselect(cfd+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(cfd,&rfds)){
				fprintf(stderr,"[Connection] received socket data \n");
				msg_handler(cfd);
			}
			// TODO admin console input
		}
		// pselect error
		else{
			if(EINTR==errno) continue;
			ERR("pselect");
		}
	}
	if(TEMP_FAILURE_RETRY(close(cfd)) < 0 ) ERR("close");
	
	sigprocmask (SIG_UNBLOCK, &mask, NULL);
}

void connection_init(int cfd)
{
	// init status	
	current_status = CMP_S_LOGGED_OUT;
	login_name = "";

	connection_work(cfd);
}
