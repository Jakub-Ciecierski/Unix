#include "client.h"

/**
 * Current state of client in the network
 * */
int current_status = CMP_S_LOGGED_OUT;

volatile sig_atomic_t do_continue = 1;

void change_status(int status)
{
	current_status = status;
}

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_regacc_handler(int fd)
{
	fprintf(stdout, " << Registretion successfull, welcome to the Server!\n");
	change_status(CMP_S_LOGGED_IN);
}
void msg_regrej_handler(int fd)
{
	fprintf(stdout, " << Registretion failed, please try different name\n");
}

void msg_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	if(bulk_read(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_read");
	
	fprintf(stderr, "[Client] after bulk_read: \n%s\n", buffer);
	
	// check the header
	char header[CMP_HEADER_SIZE];
	strncpy(header, buffer, CMP_HEADER_SIZE);
	// some trash might be left out
	if(memset(header+CMP_HEADER_SIZE, 0, CMP_HEADER_SIZE) < 0 ) ERR("memeset");
	
	/// REGISTER ACC
	if (strcmp(header, CMP_REGISTER_ACC) == 0)
	{
		msg_regacc_handler(fd);
	} 
	/// REGISTER REJ
	else if (strcmp(header, CMP_REGISTER_REJ) == 0)
	{
		msg_regrej_handler(fd);
	}
	else{
		perror("unknown message");
		exit(EXIT_FAILURE);
	}
	
}

/**********************************/
/******** CONSOLE HANDLERS ********/
/**********************************/

void cnl_usage_s_loggedin()
{
	fprintf(stdout, " << [g]ame\n");
	fprintf(stdout, "    Starts or find an available game\n\n");
	
	fprintf(stdout, " << [s]tatus\n");
	fprintf(stdout, "    Current status of your connection\n\n");
}
void cnl_usage_s_loggedout()
{
	fprintf(stdout, " << Console usage:\n\n");
	
	fprintf(stdout, " << [l]ogin\n");
	fprintf(stdout, "    Logs in to the network\n\n");
	
	fprintf(stdout, " << [r]egister\n");
	fprintf(stdout, "    Registers in to the network\n\n");
}
void cnl_usage_s_game()
{
	fprintf(stdout, " << TODO\n\n");
}

void cnl_usage(){
	fprintf(stdout, " << Console usage:\n\n");
	
	fprintf(stdout, " << [l]ogin\n");
	fprintf(stdout, "    Logs in to the network\n\n");
	
	fprintf(stdout, " << [r]egister\n");
	fprintf(stdout, "    Registers in to the network\n\n");
	
	fprintf(stdout, " << [g]ame\n");
	fprintf(stdout, "    Starts or find an available game\n\n");
	
	fprintf(stdout, " << [s]tatus\n");
	fprintf(stdout, "    Current status of your connection\n\n");
}

void cnl_login_handler(int fd)
{
	fprintf(stdout, " << Please type your name TODO\n");
}

void cnl_register_handler(int fd)
{
	char msg[CMP_MSG_SIZE];
	
	// ask client for name
	fprintf(stdout, " << Please type a name to register:\n");
	
	if(fgets(msg, CMP_MSG_SIZE, stdin) < 0) ERR("fgets");
	// we don't want new line character
	msg[strcspn(msg, "\n")] = 0;
	
	char buffer[CMP_BUFFER_SIZE];
	
	if(sprintf(buffer, "%s%s", CMP_REGISTER, msg) < 0) ERR("sprintf");
	
	// send message to server
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
	
	// read status msg
	fprintf(stderr, "[Client] Message sent: \n%s\n\n", buffer);
	//if(bulk_write(fd, msg, BUFFER_SIZE+sizeof(REGISTER_MSG)) < 0 ) ERR("bulk_write");
}

void cnl_game_handler(int fd)
{
	fprintf(stdout, " << Game TODO\n");
}

void cnl_status_handler(int fd)
{
	fprintf(stdout, " << Status TODO\n");
}

void cnl_handler_s_loggedin(int fd, char* line)
{
	if(strstr(line, "game") != NULL)
	{
		cnl_game_handler(fd);
	} 
	else if(strstr(line, "status") != NULL)
	{
		cnl_status_handler(fd);
	}
	else
	{
		cnl_usage_s_loggedin();
	}
}

void cnl_handler_s_loggedout(int fd, char* line)
{
	if(strstr(line, "login") != NULL)
	{
		cnl_login_handler(fd);
	} 
	else if(strstr(line, "register") != NULL)
	{
		cnl_register_handler(fd);
	}
	else
	{
		cnl_usage_s_loggedout();
	}
}

void cnl_handler_s_game(int fd, char* line)
{
	cnl_usage_s_game();
}

void cnl_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	
	if(fgets(buffer, CMP_BUFFER_SIZE, stdin) < 0) ERR("fgets");
	fprintf(stderr, "[Client] input:\n%s", buffer);
	
	if(current_status == CMP_S_LOGGED_IN)
	{
		cnl_handler_s_loggedin(fd, buffer);
	}
	else if(current_status == CMP_S_LOGGED_OUT)
	{
		cnl_handler_s_loggedout(fd, buffer);
	}
	else if(current_status == CMP_S_IN_GAME)
	{
		cnl_handler_s_game(fd, buffer);
	}
	
	/*
	if(strstr(buffer, "login") != NULL)
	{
		cnl_login_handler(fd);
	} 
	else if(strstr(buffer, "register") != NULL)
	{
		cnl_register_handler(fd);
	}
	else if(strstr(buffer, "game") != NULL)
	{
		cnl_game_handler(fd);
	}
	else if(strstr(buffer, "status") != NULL)
	{
		cnl_status_handler(fd);
	}
	else
	{
		cnl_usage();
	}*/
}

void get_current_status(int fd)
{
	
}

void client_work(int fd)
{
	fprintf(stderr,"[Client] starting work \n");
	fd_set base_rfds, rfds ;
	sigset_t mask, oldmask;
	
	FD_ZERO(&base_rfds);
	
	FD_SET(fd, &base_rfds);
	FD_SET(STDIN_FILENO, &base_rfds);
	
	sigemptyset (&mask);
	sigaddset (&mask, SIGINT);
	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	
	while(do_continue){
		rfds=base_rfds;

		fprintf(stderr,"[Client] starting pselect \n");
		if(pselect(fd+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(fd,&rfds)){
				fprintf(stderr,"[Client] received socket data \n");
				msg_handler(fd);
			}
			else if(FD_ISSET(STDIN_FILENO, &rfds)){
				fprintf(stderr,"[Client] received input data \n");
				cnl_handler(fd);
			}

		}
		// pselect error
		else{
			if(EINTR==errno) continue;
			ERR("pselect");
		}
	}
	sigprocmask (SIG_UNBLOCK, &mask, NULL);
}


void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port  \n",name);
}

int main(int argc, char** argv) {
	int fd;
	
	if(argc!=3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	
	fd=connect_inet_socket(argv[1],atoi(argv[2]));

	// init the client status;
	change_status(CMP_S_LOGGED_OUT);

	client_work(fd);
	
	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}
