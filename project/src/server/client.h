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

/**
 * Changes status of client's connection
 * */
void change_status(int status);

/**********************************/
/******** MESSAGE HANDLERS ********/
/**********************************/

void msg_regacc_handler(int fd);
void msg_regrej_handler(int fd);

void msg_logacc_handler(int fd);
void msg_logrej_handler(int fd);

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
