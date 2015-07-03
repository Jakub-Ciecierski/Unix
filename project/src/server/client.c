#include "client.h"

/**
 * Current state of client in the network
 * */
int current_status = CMP_S_LOGGED_OUT;

int current_game;

volatile sig_atomic_t do_continue = 1;

void change_status(int status)
{
	current_status = status;
}

void join_game(int g_id)
{
	
}

void c_sigint_handler(int sig)
{
	fprintf(stderr,"[Client] SIGINT \n");
	
	do_continue = 0;
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
	
	change_status(CMP_S_IN_GAME);
	join_game(g_id);
}

void msg_game_rej_handler(int fd)
{
	fprintf(stdout, " << Joining Game failed\n");
}

void msg_status_response_handler(int fd, char* msg)
{
	int i;
	// TODO
	fprintf(stdout, " << Your games:\n");
	
	for(i = 0; i < CMP_P_GAMES_SIZE; i++){
		int id = *(((int*)(msg)+i));
		if(id == CMP_P_EOA) break;
		
		fprintf(stdout, "ID: %d:\n", id);
	}
}

void msg_board_response_handler(char* msg)
{
	int i, j;
	fprintf(stdout, " << Board \n");
	
	for(i = 0;i < BOARD_ROW_SIZE; i++)
	{
		for(j = 0;j < BOARD_ROW_SIZE; j++)
		{
			fprintf(stdout, "%c, ",msg[i*BOARD_ROW_SIZE + j]);
		}
		fprintf(stdout, "\n");
	}
}
void msg_game_status_response_handler(char* msg)
{
	fprintf(stdout, " << Game Status: %s\n", msg);
}
void msg_turn_response_handler(char* msg)
{
	fprintf(stderr, " << Turn belongs to: %s\n", msg);
	
}
void msg_moves_response_handler(char* msg)
{
	fprintf(stdout, "[Client] Moves response\n");
}
void msg_chat_response_handler(char* msg)
{
	fprintf(stdout, " << Chat: \n%s", msg);
}
void msg_msg_response_handler(char* msg)
{
	fprintf(stdout, "[Client] MSG response\n");
}

void msg_move_response_acc_handler(char* msg)
{
	fprintf(stdout, " << Move Successfull\n");
}
void msg_move_response_rej_handler(char* msg)
{
	fprintf(stdout, " << Move Failed\n");
}
void msg_quit_response_handler(char* msg)
{
	fprintf(stdout, " << You quited the game\n");
	join_game(CN_NO_GAME);
	change_status(CMP_S_LOGGED_IN);
}
void msg_forfeit_response_handler(char* msg)
{
	fprintf(stdout, " << You forfeited the game\n");
	join_game(CN_NO_GAME);
	change_status(CMP_S_LOGGED_IN);
}

void msg_handler(int fd)
{	
	char buffer[CMP_BUFFER_SIZE];
	char header[CMP_HEADER_SIZE];
	char msg[CMP_BUFFER_SIZE];
	int size;
	
	if((size = bulk_read(fd, buffer, CMP_BUFFER_SIZE)) < 0 ) ERR("bulk_read");
	else if( size == 0){
		do_continue = 0;
		return;
	}
	
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

	/// BOARD RESPONSE
	else if (strcmp(header, CMP_BOARD_RESPONSE) == 0) {
		msg_board_response_handler(msg);
	}
	/// GAME STATUS RESPONSE
	else if (strcmp(header, CMP_GAME_STATUS_RESPONSE) == 0) {
		msg_game_status_response_handler(msg);
	}
	/// TURN RESPONSE
	else if (strcmp(header, CMP_TURN_RESPONSE) == 0) {
		msg_turn_response_handler(msg);
	}
	/// MOVES RESPONSE
	else if (strcmp(header, CMP_MOVES_RESPONSE) == 0) {
		msg_moves_response_handler(msg);
	}
	/// CHAT RESPONSE
	else if (strcmp(header, CMP_CHAT_RESPONSE) == 0) {
		msg_chat_response_handler(msg);
	}
	/// MSG RESPONSE
	else if (strcmp(header, CMP_MSG_RESPONSE) == 0) {
		msg_msg_response_handler(msg);
	}
	/// MOVE RESPONSE ACC
	else if (strcmp(header, CMP_MOVE_RESPONSE_ACC) == 0) {
		msg_move_response_acc_handler(msg);
	}
	/// MOVE RESPONSE REJ
	else if (strcmp(header, CMP_MOVE_RESPONSE_REJ) == 0) {
		msg_move_response_rej_handler(msg);
	}
	/// QUIT RESPONSE
	else if (strcmp(header, CMP_QUIT_RESPONSE) == 0) {
		msg_quit_response_handler(msg);
	}
	/// FORFEIT RESPONSE
	else if (strcmp(header, CMP_FORFEIT_RESPONSE) == 0) {
		msg_forfeit_response_handler(msg);
	}
	else if (strcmp(header, CMP_ALIVE) == 0) {
		fprintf(stderr, "[Client] Received ALIVE\n");
	}
	
	else {
		perror("unknown message");
		exit(EXIT_FAILURE);
	}
	
}

/**********************************/
/******** CONSOLE USAGES **********/
/**********************************/

void cnl_usage_s_loggedin()
{
	fprintf(stdout, " 1. %s - Starts or find an available game\n", CNL_GAME);
	fprintf(stdout, " 2. %s - Current status of your connection\n", CNL_STATUS);
}

void cnl_usage_s_loggedout()
{
	fprintf(stdout, " << Console usage:\n\n");
	
	fprintf(stdout, " 1. %s - Logs in to the network\n", CNL_LOGIN);
	fprintf(stdout, " 2. %s - Registers in to the network\n", CNL_REGISTER);
}

void cnl_usage_s_game()
{
	fprintf(stdout, " << Game options\n\n");
	fprintf(stdout, " 1. %s - display board\n", CNL_BOARD);
	fprintf(stdout, " 2. %s - get game status\n", CNL_STATUS);
	fprintf(stdout, " 3. %s - whose turn is it\n", CNL_TURN);
	fprintf(stdout, " 4. %s - list all moves so far\n", CNL_MOVES);
	fprintf(stdout, " 5. %s - display chat\n", CNL_CHAT);
	fprintf(stdout, " 6. %s - send a message\n", CNL_MSG);
	fprintf(stdout, " 7. %s - make a move\n", CNL_MOVE);
	fprintf(stdout, " 8. %s - leave the game, you can return later\n", CNL_QUIT);
	fprintf(stdout, " 9. %s - forfeits the game\n", CNL_FORFEIT);
}

/**********************************/
/******** CONSOLE HANDLERS ********/
/**********************************/

void cnl_login_handler(int fd)
{
	char msg[CMP_MSG_SIZE];	
	
	// ask client for name
	fprintf(stdout, " << Please type a name to login:\n");
	
	if(fgets(msg, CMP_MSG_SIZE, stdin) < 0) ERR("fgets");
	// we don't want new line character
	msg[strcspn(msg, "\n")] = 0;
	
	cmp_send(fd, CMP_LOGIN, msg);
}

void cnl_register_handler(int fd)
{
	char msg[CMP_MSG_SIZE];
	
	// ask client for name
	fprintf(stdout, " << Please type a name to register:\n");
	
	if(fgets(msg, CMP_MSG_SIZE, stdin) < 0) ERR("fgets");
	// we don't want new line character
	msg[strcspn(msg, "\n")] = 0;

	cmp_send(fd, CMP_REGISTER, msg);
}

void cnl_game_handler(int fd)
{
	fprintf(stdout, " << Type ID of Game you want to connect\n");
	fprintf(stdout, " << Type 'new' to join new Game \n");
	
	char msg[CMP_MSG_SIZE];
	
	if(fgets(msg, CMP_MSG_SIZE, stdin) < 0) ERR("fgets");
	// we don't want new line character
	msg[strcspn(msg, "\n")] = 0;
	
	// new game
	if(strstr(msg, "new") != NULL)
	{
		fprintf(stderr, "[Client] Creating new Game Request \n");
		cmp_send(fd, CMP_GAME_NEW, msg);
	}
	// Join existing game
	else if(atoi(msg) >= 0)
	{
		fprintf(stderr, "[Client] Join existing Game Request \n");
		cmp_send(fd, CMP_GAME_EXT, msg);
	}
	else{
		return;
	}
}

void cnl_status_handler(int fd)
{
	cmp_send(fd, CMP_STATUS, NULL);
}

void cnl_game_board_handler(int fd)
{
	cmp_send(fd, CMP_BOARD, NULL);
}
void cnl_game_status_handler(int fd)
{
	cmp_send(fd, CMP_GAME_STATUS, NULL);
}
void cnl_game_turn_handler(int fd)
{
	cmp_send(fd, CMP_TURN, NULL);
}
void cnl_game_moves_handler(int fd)
{
	cmp_send(fd, CMP_MOVES, NULL);
}
void cnl_game_chat_handler(int fd)
{
	cmp_send(fd, CMP_CHAT, NULL);
}
void cnl_game_msg_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	fprintf(stdout, " << Type a message for your opponent\n");
	
	if(fgets(buffer, CMP_BUFFER_SIZE, stdin) < 0) ERR("fgets");

	cmp_send(fd, CMP_MSG, buffer);
}
void cnl_game_move_handler(int fd)
{
	int x1, y1, x2, y2;
	char buffer[CMP_BUFFER_SIZE];
	char write_buffer[CMP_BUFFER_SIZE];
	
	fprintf(stdout, " << Type x1:\n");
	if(fgets(buffer, CMP_BUFFER_SIZE, stdin) < 0) ERR("fgets");
	x1 = atoi(buffer);
	
	fprintf(stdout, " << Type y1:\n");
	if(fgets(buffer, CMP_BUFFER_SIZE, stdin) < 0) ERR("fgets");
	y1 = atoi(buffer);
	
	fprintf(stdout, " << Type x2:\n");
	if(fgets(buffer, CMP_BUFFER_SIZE, stdin) < 0) ERR("fgets");
	x2 = atoi(buffer);
	
	fprintf(stdout, " << Type y2:\n");
	
	if(fgets(buffer, CMP_BUFFER_SIZE, stdin) < 0) ERR("fgets");
	y2 = atoi(buffer);
	
	fprintf(stdout, " << x1: %d, y1: %d -> x2; %d, y2: %d\n", x1, y1, x2 ,y2);
	
	if(snprintf(write_buffer, CMP_BUFFER_SIZE, 
				"%d,%d,%d,%d", x1, y1, x2 ,y2) < 0) ERR("sprintf");

	cmp_send(fd, CMP_MOVE, write_buffer);
}
void cnl_game_quit_handler(int fd)
{	
	cmp_send(fd, CMP_QUIT, NULL);
}
void cnl_game_forfeit_handler(int fd)
{
	cmp_send(fd, CMP_FORFEIT, NULL);
}

/**********************************/
/**** CONSOLE STATUS HANDLERS *****/
/**********************************/

void cnl_handler_s_loggedin(int fd, char* line)
{
	if(strstr(line, CNL_GAME) != NULL) {
		cnl_game_handler(fd);
	} 
	else if(strstr(line, CNL_STATUS) != NULL) {
		cnl_status_handler(fd);
	}
	// print usage
	else {
		cnl_usage_s_loggedin();
	}
}

void cnl_handler_s_loggedout(int fd, char* line)
{
	if(strstr(line, CNL_LOGIN) != NULL) {
		cnl_login_handler(fd);
	} 
	else if(strstr(line, CNL_REGISTER) != NULL) {
		cnl_register_handler(fd);
	}
	// print usage
	else {
		cnl_usage_s_loggedout();
	}
}

void cnl_handler_s_game(int fd, char* line)
{
	if(strstr(line, CNL_BOARD) != NULL) {
		cnl_game_board_handler(fd);
	} 
	else if(strstr(line, CNL_STATUS) != NULL) {
		cnl_game_status_handler(fd);
	}
	else if(strstr(line, CNL_TURN) != NULL) {
		cnl_game_turn_handler(fd);
	}
	else if(strstr(line, CNL_MOVES) != NULL) {
		cnl_game_moves_handler(fd);
	}
	else if(strstr(line, CNL_CHAT) != NULL) {
		cnl_game_chat_handler(fd);
	}
	else if(strstr(line, CNL_MSG) != NULL) {
		cnl_game_msg_handler(fd);
	}
	else if(strstr(line, CNL_MOVE) != NULL) {
		cnl_game_move_handler(fd);
	}
	else if(strstr(line, CNL_QUIT) != NULL) {
		cnl_game_quit_handler(fd);
	}
	else if(strstr(line, CNL_FORFEIT) != NULL) {
		cnl_game_forfeit_handler(fd);
	}
	// print usage
	else {
		cnl_usage_s_game();
	}
}

void cnl_handler(int fd)
{
	char buffer[CMP_BUFFER_SIZE];
	
	if(fgets(buffer, CMP_BUFFER_SIZE, stdin) < 0) ERR("fgets");
	fprintf(stderr, "[Client] input:\n%s", buffer);
	
	if(current_status == CMP_S_LOGGED_IN) {
		cnl_handler_s_loggedin(fd, buffer);
	}
	else if(current_status == CMP_S_LOGGED_OUT) {
		cnl_handler_s_loggedout(fd, buffer);
	}
	else if(current_status == CMP_S_IN_GAME) {
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
	
	fprintf(stderr,"[Client] Closing FD \n");
	if(TEMP_FAILURE_RETRY(close(fd)) < 0 ) ERR("close");
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
	if(sethandler(c_sigint_handler,SIGINT)) ERR("Seting SIGPIPE:");
	
	fd=connect_inet_socket(argv[1],atoi(argv[2]));

	// init the client status;
	change_status(CMP_S_LOGGED_OUT);
	join_game(CN_NO_GAME);

	// print console usage
	cnl_usage_s_loggedout();

	client_work(fd);

	return EXIT_SUCCESS;
}
