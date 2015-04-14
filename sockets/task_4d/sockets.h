#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

int make_socket(int domain, int type);

struct sockaddr_un make_local_address(char *name);

struct sockaddr_in make_inet_address(char *address, uint16_t port);

int bind_local_socket(char *name, int type, int backlog);

int bind_inet_socket(uint16_t port,int type, int backlog);

int connect_local_socket(char *name);

int connect_inet_socket(char *name, uint16_t port);

#endif
