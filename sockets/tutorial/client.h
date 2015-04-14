#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "macros.h"
#include "io.h"
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

void prepare_send_local_data(char** argv, int32_t data[]);
void prepare_send_inet_data(char** argv, int32_t data[]);
void prepare_revc_data(int32_t data[]);

void analyze_result(int32_t data[]);

void client_work(int fd, int32_t data[]);

#endif
