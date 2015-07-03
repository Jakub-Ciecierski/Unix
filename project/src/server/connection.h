#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "../util/macros.h"
#include "../util/io.h"
#include "../util/sockets.h"
#include "../util/msg_protocol.h"
#include "database.h"

#include <stdio.h>

#define CN_NO_GAME -1

/**********************************/
/******** UTILITY FUNCTIONS *******/
/**********************************/

/**
 * Changes status of client's connection
 * */
void change_status(int status);

void join_game(int id);

void log_in(char* p_name);

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_login_handler(int fd, char* name);
void msg_register_handler(int fd, char* name);

void msg_game_new_handler(int fd);
void msg_game_ext_handler(int fd, int id);

void msg_status_handler(int fd);

void msg_game_board_handler(int fd);
void msg_game_status_handler(int fd);
void msg_game_turn_handler(int fd);
void msg_game_moves_handler(int fd);
void msg_game_chat_handler(int fd);
void msg_game_msg_handler(int fd, char* msg);
void msg_game_move_handler(int fd, char* msg);
void msg_game_quit_handler(int fd);
void msg_game_forfeit_handler(int fd);

void msg_handler(int fd);

/**********************************/
/******** MAIN FUNCTIONS **********/
/**********************************/

void connection_work(int cfd);

void connection_init(int cfd);

#endif
