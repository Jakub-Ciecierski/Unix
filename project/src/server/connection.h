#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "../util/macros.h"
#include "../util/io.h"
#include "../util/sockets.h"
#include "../util/msg_protocol.h"
#include "database.h"

#include <stdio.h>

/**
 * Changes status of client's connection
 * */
void change_status(int status);

void log_in(char* p_name);
/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_login_handler(int fd, char* name);
void msg_register_handler(int fd, char* name);

void msg_game_new_handler(int fd);
void msg_game_ext_handler(int fd, int id);

void msg_handler(int fd);

void connection_work(int cfd);

void connection_init(int cfd);

#endif
