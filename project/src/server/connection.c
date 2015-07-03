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

void c_sigpipe_handler(int sig)
{
	fprintf(stderr,"[Connection] Client disconeccted \n");
	
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
	if(snprintf(player_name, BD_P_SIZE, "%s", p_name) < 0) ERR("sprintf");
	
	fprintf(stderr,"[Connection] Player name: %s\n", player_name);
}

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_login_handler(int fd, char* name)
{
	fprintf(stderr,"[Connection] User wants to login in name: %s\n", name);

	if(db_player_exists(name) == 0 ){
		// send deny msg
		fprintf(stderr, "[Connection] Login Successfull\n");
		
		log_in(name);
		
		cmp_send(fd, CMP_LOGIN_ACC, NULL);
	}
	else{
		fprintf(stderr, "[Connection] Login Failed\n");
		
		cmp_send(fd, CMP_LOGIN_REJ, NULL);
	}
}

void msg_register_handler(int fd, char* name)
{
	fprintf(stderr,"[Connection] User wants to register name: %s\n", name);

	if(db_create_player(name) < 0 ){
		// send deny msg
		fprintf(stderr, "[Connection] Name already exists\n");
		
		cmp_send(fd, CMP_REGISTER_REJ, NULL);
	}
	else{
		fprintf(stderr, "[Connection] Registretion Successfull\n");
		
		log_in(name);
		
		cmp_send(fd, CMP_REGISTER_ACC, NULL);
	}
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
	
	g_id = htonl(g_id);
	if(sprintf(buffer, "%s%d", CMP_GAME_JOIN_ACC, g_id) < 0) ERR("sprintf");
	
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) {
		if(errno != EPIPE)
			ERR("bulk_write");
	}
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

	for(i = 0;i < CMP_P_GAMES_SIZE; i++){
		if(games[i] == CMP_P_EOA) {
			break;
		}
		if(games[i] == id) {
			game_exists = 0;
			break;
		}
		fprintf(stderr, "[Connection] Game#%d id: %d\n", i, games[i]);
	}

	if(game_exists == 0) {
		// send acc reply
		join_game(id);
		change_status(CMP_S_IN_GAME);
		
		if(snprintf(buffer, CMP_BUFFER_SIZE, "%s%d", CMP_GAME_JOIN_ACC, htonl(id)) < 0) ERR("sprintf");

	}
	else {
		if(snprintf(buffer, CMP_BUFFER_SIZE, "%s%d", CMP_GAME_JOIN_REJ, htonl(id)) < 0) ERR("sprintf");
	}

	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) {
		if(errno != EPIPE)
			ERR("bulk_write");
	}

	free(games);
}

/**
 * Currently this function is a major work-around.
 * Works but is not a shinning example of how programming should be done
 * */
void msg_status_handler(int fd)
{
	int i;
	int* games;
	//char* msg = (char*)malloc(CMP_BUFFER_SIZE*sizeof(char));
	char buffer[CMP_BUFFER_SIZE];
	
	fprintf(stderr, "[Connection] Status handler\n");
	
	games = db_player_get_games_id(player_name);
	buffer[0] = 's';
	buffer[1] = 'r';

	for(i = 0; i < CMP_P_GAMES_SIZE; i++){
		*(((int*)(buffer + CMP_HEADER_SIZE)+i)) = htonl(games[i]);
		if(games[i] == CMP_P_EOA) {
			break;
		}
	}

	for(i = 0; i < CMP_P_GAMES_SIZE; i++){
		int id = *(((int*)(buffer + CMP_HEADER_SIZE)+i));
		if(games[i] == CMP_P_EOA) {
			break;
		}
		fprintf(stderr, "[Connection] ID: %d:\n", id);
	}

	fprintf(stderr, "[Connection] buffer: %s:\n", buffer);
				
	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) {
		if(errno != EPIPE)
			ERR("bulk_write");
	}
	
	free(games);
}
/// DONE
void msg_game_board_handler(int fd)
{
	char buffer[DB_BOARD_SIZE];
	
	fprintf(stderr, "[Connection] board handler\n");
	
	db_get_board(current_game, buffer);
	
	cmp_send(fd, CMP_BOARD_RESPONSE, buffer);
}
/// DONE
void msg_game_status_handler(int fd)
{
	int status;
	char msg[CMP_MSG_SIZE];
	fprintf(stderr, "[Connection] game status handler\n");
	status = db_get_game_status(current_game);
	
	if(snprintf(msg, CMP_MSG_SIZE, "%d", status) < 0) ERR("sprintf");
	
	cmp_send(fd, CMP_GAME_STATUS_RESPONSE, msg);
}
/// DONE
void msg_game_turn_handler(int fd)
{
	char* buff;
	
	fprintf(stderr, "[Connection] turn handler\n");
	
	buff = db_get_player_turn(current_game);
	
	fprintf(stderr, "[Connection] turn: %s\n", buff);
	
	if(strcmp(player_name, buff) == 0)
		fprintf(stderr, " << It's me \n");
	else
		fprintf(stderr, " << It's oponent \n");
	
	cmp_send(fd, CMP_TURN_RESPONSE, buff);
	
	free(buff);
}
/// DONE
void msg_game_moves_handler(int fd)
{
	char buffer[BD_G_SIZE];
	db_get_moves(current_game, buffer);
	
	fprintf(stderr, "[Connection] moves handler\n");
	
	cmp_send(fd, CMP_MOVES_RESPONSE, buffer);
}
/// DONE
void msg_game_chat_handler(int fd)
{
	char buffer[BD_G_SIZE];
	fprintf(stderr, "[Connection] chat handler\n");
	
	db_get_chat(current_game, buffer);
	
	cmp_send(fd, CMP_CHAT_RESPONSE, buffer);
}
/// DONE
void msg_game_msg_handler(int fd, char* msg)
{
	fprintf(stderr, "[Connection] msg handler\n");
	
	db_add_chat_entry(current_game, player_name, msg);
	
	cmp_send(fd, CMP_MSG_RESPONSE, NULL);
}
/// DONE
void msg_game_move_handler(int fd, char* msg)
{
	char opponent[BD_G_SIZE];
	char* p_turn;
	
	fprintf(stderr, "[Connection] move handler\n");
	
	db_get_opponent(current_game, player_name, opponent);
	p_turn = db_get_player_turn(current_game);
	
	if(db_get_game_status(current_game) != DB_H_G_SS_RESOLVED && opponent != NULL && p_turn != NULL && strcmp(player_name, p_turn) == 0) {
		int x1, y1, x2, y2;
		fprintf(stderr, "[Connection] msg: %s\n", msg);	
		if(TEMP_FAILURE_RETRY(sscanf(msg, "%d,%d,%d,%d", 
					&x1, &y1, &x2, &y2)) < 0) ERR("close");
		
		fprintf(stderr, "[Connection] x1: %d, y1: %d -> x2; %d, y2: %d\n", x1, y1, x2 ,y2);
		
		// move on the board
		db_board_move(current_game, x1, y1, x2, y2);
		
		fprintf(stderr, "[Connection] Switching turn to: %s\n", opponent);
		
		// change turn
		db_set_player_turn(current_game, opponent);
		
		// add move to archive
		db_add_move(current_game, msg);
		
		// send msg
		cmp_send(fd, CMP_MOVE_RESPONSE_ACC, NULL);
	}
	else
		cmp_send(fd, CMP_MOVE_RESPONSE_REJ, NULL);
}
/// DONE
void msg_game_quit_handler(int fd)
{
	fprintf(stderr, "[Connection] quit handler\n");
	
	join_game(CN_NO_GAME);
	change_status(CMP_S_LOGGED_IN);
	
	cmp_send(fd, CMP_QUIT_RESPONSE, NULL);
}
/// DONE
void msg_game_forfeit_handler(int fd)
{
	fprintf(stderr, "[Connection] Forfeit handler: g_id %d\n", current_game);
	
	db_set_game_status(current_game, DB_H_G_SS_RESOLVED);
	
	join_game(CN_NO_GAME);
	change_status(CMP_S_LOGGED_IN);
	
	cmp_send(fd, CMP_FORFEIT_RESPONSE, NULL);
}

/**
 * Handles incomming messages
 * */
void msg_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	char* msg;
	int size;
	
	if((size = bulk_read(fd, buffer, CMP_BUFFER_SIZE)) < 0 ) ERR("bulk_read");
	else if( size == 0){
		c_do_continue = 0;
		return;
	}

	fprintf(stderr, "[Connection] after bulk_read size %d: \n%s\n", size, buffer);
	
	// check the header
	char header[CMP_HEADER_SIZE];
	strncpy(header, buffer, CMP_HEADER_SIZE);
	// some trash might be left out
	if(memset(header+CMP_HEADER_SIZE, 0, CMP_HEADER_SIZE) < 0 ) ERR("memeset");

	msg = buffer + CMP_HEADER_SIZE;

	fprintf(stderr, "[Connection] after cmp_get_header\n");
	
	/// REGISTER
	if (strcmp(header, CMP_REGISTER) == 0) {
		char* name = buffer + CMP_HEADER_SIZE;
		msg_register_handler(fd, name);
	} 
	/// LOGIN
	else if (strcmp(header, CMP_LOGIN) == 0) {
		// get the message it self
		char* name = buffer + CMP_HEADER_SIZE;
		msg_login_handler(fd, name);
	}
	/// GAME NEW
	else if (strcmp(header, CMP_GAME_NEW) == 0) {
		msg_game_new_handler(fd);
	}
	/// GAME EXISTING
	else if (strcmp(header, CMP_GAME_EXT) == 0) {
		int id = atoi((buffer + CMP_HEADER_SIZE));
		msg_game_ext_handler(fd, id);
	}
	/// STATUS
	else if (strcmp(header, CMP_STATUS) == 0) {
		msg_status_handler(fd);
	}
	/// BOARD
	else if (strcmp(header, CMP_BOARD) == 0) {
		msg_game_board_handler(fd);
	}
	/// GAME STATUS
	else if (strcmp(header, CMP_GAME_STATUS) == 0) {
		msg_game_status_handler(fd);
	}
	/// TURN
	else if (strcmp(header, CMP_TURN) == 0) {
		msg_game_turn_handler(fd);
	}
	/// MOVES
	else if (strcmp(header, CMP_MOVES) == 0) {
		msg_game_moves_handler(fd);
	}
	/// CHAT
	else if (strcmp(header, CMP_CHAT) == 0) {
		msg_game_chat_handler(fd);
	}
	/// MSG
	else if (strcmp(header, CMP_MSG) == 0) {
		msg_game_msg_handler(fd, msg);
	}
	/// MOVE
	else if (strcmp(header, CMP_MOVE) == 0) {
		msg_game_move_handler(fd, msg);
	}
	/// QUIT
	else if (strcmp(header, CMP_QUIT) == 0) {
		msg_game_quit_handler(fd);
	}
	/// FORFEIT
	else if (strcmp(header, CMP_FORFEIT) == 0) {
		msg_game_forfeit_handler(fd);
	}
	else {
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
		//msg_handler(cfd);
		
		rfds=base_rfds;

		fprintf(stderr,"[Connection] starting pselect \n");
		if(pselect(cfd+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
			if(FD_ISSET(cfd,&rfds)) {
				fprintf(stderr,"[Connection] received socket data \n");
				msg_handler(cfd);
			}
			// TODO admin console input
		}
		// pselect error
		else {
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
	if(sethandler(c_sigpipe_handler, SIGPIPE)<0) ERR("sethandler");
	
	// init status	
	current_status = CMP_S_LOGGED_OUT;
	join_game(CN_NO_GAME);
	
	connection_work(cfd);
	
	exit(EXIT_SUCCESS);
}
