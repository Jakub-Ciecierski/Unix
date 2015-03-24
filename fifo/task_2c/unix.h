#ifndef _UNIX_H_
#define _UNIX_H_

#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

int64_t  bulk_read(int fd, char *buf, size_t count);

int64_t  bulk_write(int fd, char *buf, size_t count);

int sethandler( void (*f)(int), int sigNo);

#endif
