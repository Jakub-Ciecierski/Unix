#ifndef _LOGIC_H_
#define _LOGIC_H_

#include "../util/macros.h"
#include "../util/io.h"
#include "../util/sockets.h"
#include "database.h"
#include "connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>

#define BACKLOG 3

/**
 * Handles the a given signal
 * */
void sig_handler(int sig);

/**
 * Adds client that wanted to connected to the server.
 * Return the socket descriptor of the connection
 * */
int add_new_client(int sfd);

/**
 * Handles incoming tcp connection.
 * Creates new connection process
 * */
void connection_handler(int cfd);

/**
 * Main server 'loop'
 * */
void server_work(int fdT);

/**********************************/
/*********** INITIATORS ***********/
/**********************************/ 
/**
 * Initiates signal handler
 * */
void init_sig_handlers();
/**
 * Initiates tcp connection
 * */
void init_tcp(int* fdT, int* new_flags, int port);
/**
 * Initiates database directory.
 * Does not create if it already exists
 * */
void init_dir();
/**
 * Used to initilize the server and start up
 * */
void init_server(int port);

#endif
