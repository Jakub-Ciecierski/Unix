#include "connection.h"

volatile sig_atomic_t c_do_continue = 1;

int current_status = CMP_S_LOGGED_OUT;
char player_name[DB_FILENAME_SIZE];
int current_game;

void c_sig_handler(int sig)
{
	fprintf(stderr,"[Connection] Signal \n");

	c_do_continue = 0;
}

void change_status(int status)
{
	current_status = status;
}

void join_game(int id)
{
	current_game = id;
}

void log_in(char* p_name)
{
	// change status and save current login name
	change_status(CMP_S_LOGGED_IN);
	if(sprintf(player_name, "%s", p_name) < 0) ERR("sprintf");
	
	fprintf(stderr,"[Connection] Player name: %s\n", player_name);
}

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_login_handler(int fd, char* name)
{
	fprintf(stderr,"[Connection] User wants to register name: %s\n", name);
	
	char buffer[CMP_BUFFER_SIZE];

	if(db_player_exists(name) == 0 ){
		// send deny msg
		fprintf(stderr, "[Connection] Login Successfull\n");
		
		log_in(name);
		
		// send only header
		if(sprintf(buffer, "%s", CMP_LOGIN_ACC) < 0) ERR("sprintf");
	}
	else{
		fprintf(stderr, "[Connection] Login Failed\n");
		

		if(sprintf(buffer, "%s", CMP_LOGIN_REJ) < 0) ERR("sprintf");
	}
	
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
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
		
		log_in(name);
		
		if(sprintf(buffer, "%s", CMP_REGISTER_ACC) < 0) ERR("sprintf");
	}
	
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
}

void msg_game_new_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	int g_id;
	
	fprintf(stderr, "[Connection] Looking for new Game \n");
	
	// run the function that connects the player with new game
	g_id = db_join_or_create_game(player_name);
	
	join_game(g_id);
	change_status(CMP_S_IN_GAME);
	
	if(sprintf(buffer, "%s%d", CMP_REGISTER_REJ, g_id) < 0) ERR("sprintf");
	
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
}

void msg_game_ext_handler(int fd, int id)
{
	char buffer[CMP_BUFFER_SIZE];
	int* games;
	int game_exists;
	int i;

	fprintf(stderr, "[Connection] Trying to connect to Game: %d\n", id);

	games = db_player_get_games_id(player_name);
	game_exists = -1;

	for(i = 0;i < DB_P_GAMES_SIZE; i++){
		if(games[i] == DB_P_EOA) {
			game_exists = 0;
			break;
		}
		fprintf(stderr, "[Connection] Game#%d id: %d\n", i, games[i]);
	}

	if(game_exists == 0){
		// send acc reply
		join_game(g_id);
		change_status(CMP_S_IN_GAME);
		
		if(sprintf(buffer, "%s%d", CMP_JOIN_GAME_ACC, id) < 0) ERR("sprintf");
		
	}else{
		if(sprintf(buffer, "%s%d", CMP_JOIN_GAME_REJ, id) < 0) ERR("sprintf");
	}

	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) ERR("bulk_write");
	
	free(games);
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
		// get the message it self
		char* name = buffer + CMP_HEADER_SIZE;
		msg_login_handler(fd, name);
	}
	/// GAME NEW
	else if (strcmp(header, CMP_GAME_NEW) == 0)
	{
		msg_game_new_handler(fd);
	}
	/// GAME EXISTING
	else if (strcmp(header, CMP_GAME_EXT) == 0)
	{
		// get the message it self
		int id = atoi((buffer + CMP_HEADER_SIZE));
		msg_game_ext_handler(fd, id);
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
	if(sethandler(c_sig_handler, SIGINT)<0) ERR("sethandler");
	
	// init status	
	current_status = CMP_S_LOGGED_OUT;
	join_game(CN_NO_GAME);
	
	connection_work(cfd);
	
	exit(EXIT_SUCCESS);
}
