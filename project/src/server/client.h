#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "../util/macros.h"
#include "../util/io.h"
#include "../util/sockets.h"
#include "../util/msg_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#define CN_NO_GAME -1

/**********************************/
/********* CONSOLE INPUT **********/
/**********************************/

#define CNL_GAME "game"
#define CNL_STATUS "status"

#define CNL_LOGIN "login"
#define CNL_REGISTER "register"

#define CNL_BOARD "board"
#define CNL_STATUS "status"
#define CNL_TURN "turn"
#define CNL_MOVES "moves"
#define CNL_CHAT "chat"
#define CNL_MSG "msg"
#define CNL_MOVE "move"
#define CNL_QUIT "quit"
#define CNL_FORFEIT "forfeit"

#define BOARD_ROW_SIZE 8

/**
 * Changes status of client's connection
 * */
void change_status(int status);

void join_game(int g_id);

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_regacc_handler(int fd);
void msg_regrej_handler(int fd);

void msg_logacc_handler(int fd);
void msg_logrej_handler(int fd);

void msg_game_acc_handler(int fd, char* msg);
void msg_game_rej_handler(int fd);

void msg_status_response_handler(int fd, char* msg);

void msg_board_response_handler(char* msg);
void msg_game_status_response_handler(char* msg);
void msg_turn_response_handler(char* msg);
void msg_moves_response_handler(char* msg);
void msg_chat_response_handler(char* msg);
void msg_move_response_acc_handler(char* msg);
void msg_move_response_rej_handler(char* msg);
void msg_msg_response_handler(char* msg);
void msg_quit_response_handler(char* msg);
void msg_forfeit_response_handler(char* msg);

/// Main message handler
void msg_handler(int fd);

/**********************************/
/******** CONSOLE HANDLERS ********/
/**********************************/

/// Usages
void cnl_usage_s_loggedin();
void cnl_usage_s_loggedout();
void cnl_usage_s_game();

/// Specific console handlers
void cnl_login_handler(int fd);
void cnl_register_handler(int fd);
void cnl_game_handler(int fd);
void cnl_status_handler(int fd);

/// Game console
void cnl_game_board_handler(int fd);
void cnl_game_status_handler(int fd);
void cnl_game_turn_handler(int fd);
void cnl_game_moves_handler(int fd);
void cnl_game_chat_handler(int fd);
void cnl_game_msg_handler(int fd);
void cnl_game_move_handler(int fd);
void cnl_game_quit_handler(int fd);
void cnl_game_forfeit_handler(int fd);

/// Individual console components - depending of client status
void cnl_handler_s_loggedin(int fd, char* line);
void cnl_handler_s_loggedout(int fd, char* line);
void cnl_handler_s_game(int fd, char* line);
/// Main console handler
void cnl_handler(int fd);

/**********************************/
/*********** MAIN STUFF ***********/
/**********************************/

void client_work(int fd);

void usage(char * name);
int main(int argc, char** argv);

#endif
