#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "../util/macros.h"
#include "../util/io.h"
#include "../util/sockets.h"

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

void handle_input(char* input);

#endif
