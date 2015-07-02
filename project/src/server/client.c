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

void c_sigpipe_handler(int sig)
{
	fprintf(stderr,"[Client] Client disconeccted \n");
	
	do_continue = 0;
}

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_logacc_handler(int fd)
{
	fprintf(stdout, " << Login successfull, welcome to the Server!\n");
	change_status(CMP_S_LOGGED_IN);
}

void msg_logrej_handler(int fd)
{
	fprintf(stdout, " << Login failed, please try different name\n");
}

void msg_regacc_handler(int fd)
{
	fprintf(stdout, " << Registretion successfull, welcome to the Server!\n");
	change_status(CMP_S_LOGGED_IN);
}
void msg_regrej_handler(int fd)
{
	fprintf(stdout, " << Registretion failed, please try different name\n");
}

void msg_game_acc_handler(int fd, char* msg)
{
	int g_id = atoi(msg);
	fprintf(stdout, " << Joined Game: %d\n", g_id);
}

void msg_game_rej_handler(int fd)
{
	fprintf(stdout, " << Joining Game failed\n");
}

void msg_status_response_handler(int fd, char* msg)
{
	int i;
	int* games;
	
	games = (int*)msg;
	fprintf(stdout, " << Your games:\n");
	for(i = 0; i < CMP_P_GAMES_SIZE; i++){
		if(games[i] == CMP_P_EOA) break;
		
		fprintf(stdout, "ID: %d:\n", games[i]);
	}
}

void msg_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	char header[CMP_HEADER_SIZE];
	char msg[CMP_BUFFER_SIZE];
	
	if(bulk_read(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_read");
	
	fprintf(stderr, "[Client] after bulk_read: \n%s\n", buffer);
	
	// check the header
	strncpy(header, buffer, CMP_HEADER_SIZE);
	// some trash might be left out
	if(memset(header+CMP_HEADER_SIZE, 0, CMP_HEADER_SIZE) < 0 ) ERR("memeset");
	
	// check the message itself
	strncpy(msg, buffer + CMP_HEADER_SIZE, CMP_BUFFER_SIZE);
	
	/// REGISTER ACC
	if (strcmp(header, CMP_REGISTER_ACC) == 0) {
		msg_regacc_handler(fd);
	} 
	/// REGISTER REJ
	else if (strcmp(header, CMP_REGISTER_REJ) == 0) {
		msg_regrej_handler(fd);
	}
	
	/// LOGIN ACC
	else if (strcmp(header, CMP_LOGIN_ACC) == 0) {
		msg_logacc_handler(fd);
	} 
	/// LOGIN REJ
	else if (strcmp(header, CMP_LOGIN_REJ) == 0) {
		msg_logrej_handler(fd);
	}
	
	/// JOIN GAME ACC
	else if (strcmp(header, CMP_GAME_JOIN_ACC) == 0) {
		msg_game_acc_handler(fd, msg);
	}
	/// JOIN GAME REJ
	else if (strcmp(header, CMP_GAME_JOIN_REJ) == 0) {
		msg_game_rej_handler(fd);
	}
	
	/// STATUS RESPONSE
	else if (strcmp(header, CMP_STATUS_RESPONSE) == 0) {
		msg_status_response_handler(fd, msg);
	}

	else{
		perror("unknown message");
		exit(EXIT_FAILURE);
	}
	
}

/**********************************/
/******** CONSOLE USAGES **********/
/**********************************/

void cnl_usage_s_loggedin()
{
	fprintf(stdout, " 1. game - Starts or find an available game\n");
	fprintf(stdout, " 2. status - Current status of your connection\n");
}

void cnl_usage_s_loggedout()
{
	fprintf(stdout, " << Console usage:\n\n");
	
	fprintf(stdout, " 1. login - Logs in to the network\n");
	fprintf(stdout, " 2. register - Registers in to the network\n");
}

void cnl_usage_s_game()
{
	fprintf(stdout, " << TODO\n\n");
}

/**********************************/
/******** CONSOLE HANDLERS ********/
/**********************************/

void cnl_login_handler(int fd)
{
	char msg[CMP_MSG_SIZE];
	char buffer[CMP_BUFFER_SIZE];	
	
	// ask client for name
	fprintf(stdout, " << Please type a name to login:\n");
	
	if(fgets(msg, CMP_MSG_SIZE, stdin) < 0) ERR("fgets");
	// we don't want new line character
	msg[strcspn(msg, "\n")] = 0;
	
	if(sprintf(buffer, "%s%s", CMP_LOGIN, msg) < 0) ERR("sprintf");
	
	// send message to server
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
	
	fprintf(stderr, "[Client] Message sent: \n%s\n\n", buffer);
}

void cnl_register_handler(int fd)
{
	char msg[CMP_MSG_SIZE];
	char buffer[CMP_BUFFER_SIZE];
	
	// ask client for name
	fprintf(stdout, " << Please type a name to register:\n");
	
	if(fgets(msg, CMP_MSG_SIZE, stdin) < 0) ERR("fgets");
	// we don't want new line character
	msg[strcspn(msg, "\n")] = 0;
	
	if(sprintf(buffer, "%s%s", CMP_REGISTER, msg) < 0) ERR("sprintf");
	
	// send message to server
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
	
	fprintf(stderr, "[Client] Message sent: \n%s\n\n", buffer);
}

void cnl_game_handler(int fd)
{
	fprintf(stdout, " << Type ID of Game you want to connect\n");
	fprintf(stdout, " << Type 'new' to join new Game \n");
	
	char msg[CMP_MSG_SIZE];
	char buffer[CMP_BUFFER_SIZE];
	
	if(fgets(msg, CMP_MSG_SIZE, stdin) < 0) ERR("fgets");
	// we don't want new line character
	msg[strcspn(msg, "\n")] = 0;
	
	// new game
	if(strstr(msg, "new") != NULL)
	{
		fprintf(stderr, "[Client] Creating new Game Request \n");
		if(sprintf(buffer, "%s%s", CMP_GAME_NEW, msg) < 0) ERR("sprintf");
	}
	// Join existing game
	else if(atoi(msg) >= 0)
	{
		fprintf(stderr, "[Client] Join existing Game Request \n");
		if(sprintf(buffer, "%s%s", CMP_GAME_EXT, msg) < 0) ERR("sprintf");
	}
	else{
		return;
	}

	// send message to server
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
}

void cnl_status_handler(int fd)
{
	char msg[CMP_BUFFER_SIZE];
	
	if(snprintf(msg, CMP_BUFFER_SIZE, "%s", CMP_STATUS) < 0) ERR("sprintf");
	
	// send message to server
	if(bulk_write(fd, msg, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
	
	fprintf(stderr, "[Client] Message sent: \n%s\n\n", msg);
}

/**********************************/
/**** CONSOLE STATUS HANDLERS *****/
/**********************************/

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
	// print usage
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
	// print usage
	else
	{
		cnl_usage_s_loggedout();
	}
}

void cnl_handler_s_game(int fd, char* line)
{
	// print usage
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
	
	if(sethandler(c_sigpipe_handler,SIGPIPE)) ERR("Seting SIGPIPE:");
	
	fd=connect_inet_socket(argv[1],atoi(argv[2]));

	// init the client status;
	change_status(CMP_S_LOGGED_OUT);

	// print console usage
	cnl_usage_s_loggedout();

	client_work(fd);
	
	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}
