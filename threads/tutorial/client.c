#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

#define CHUNKSIZE 500
#define NMMAX 30

typedef struct
{
	struct sockaddr_in *addr;
	pthread_mutex_t *mutex;
	char file[NMMAX + 1];
} thread_arg;

volatile sig_atomic_t work = 1;

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s domain port\n", name);
	exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0x00, sizeof(struct sigaction));
	act.sa_handler = f;

	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void siginthandler(int sig)
{
	work = 0;
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;

	do
	{
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0)
			return c;
		if (c == 0)
			return len;
		buf += c;
		len += c;
		count -= c;
	}
	while (count > 0);

	return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;

	do
	{
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if(c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	}
	while (count > 0);

	return len;
}

int make_socket(void)
{
	int sock;
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		ERR("socket");

	return sock;
}

struct sockaddr_in make_address(char *address, uint16_t port)
{
	struct sockaddr_in addr;
	struct hostent *hostinfo;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	hostinfo = gethostbyname(address);
	if (hostinfo == NULL)
		HERR("gethostbyname");

	addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;

	return addr;
}

int connect_socket(struct sockaddr_in *addr)
{
	int socketfd;
	socketfd = make_socket();
	if (connect(socketfd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) < 0)
	{
		if (errno != EINTR)
			ERR("connect");
		else
		{ 
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0)
				ERR("select");
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
				ERR("getsockopt");
			if (status != 0)
				ERR("connect");
		}
	}

	return socketfd;
}

void *threadfunc(void *arg)
{
	int fd;
	char buf[CHUNKSIZE + 1];
	thread_arg *targ = (thread_arg *) arg;
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);

	if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
		ERR("pthread_mask");

	fd = connect_socket(targ->addr);

	if (bulk_write(fd, (void *) targ->file, NMMAX + 1) < 0)
		ERR("write");
	if (bulk_read(fd, (void *) buf, CHUNKSIZE) < 0)
		ERR("read");
	if (TEMP_FAILURE_RETRY(close(fd)) == -1)
		ERR("close");

	if (pthread_mutex_lock(targ->mutex) != 0)
		ERR("pthread_mutex_lock");
	printf("Content of %s:\n", targ->file);
	printf("%s\n", buf);
	if (pthread_mutex_unlock(targ->mutex) != 0)
		ERR("pthread_mutex_unlock");
	if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0)
		ERR("pthread_sigmask");

	free(targ);
	return NULL;
}

void dowork(struct sockaddr_in *addr, pthread_mutex_t *mutex)
{
	thread_arg *targ;
	pthread_t thread;

	while (work)
	{
		if ((targ = (thread_arg *) calloc (1, sizeof(thread_arg))) == NULL)
			ERR("calloc");
		targ->mutex = mutex;
		targ->addr = addr;
		fgets(targ->file, NMMAX + 1, stdin);
		if (targ->file[strlen(targ->file) - 1] == '\n')
			targ->file[strlen(targ->file) - 1] = '\0';
		if (pthread_create(&thread, NULL, threadfunc, (void *) targ) != 0)
			ERR("pthread_create");
		if (pthread_detach(thread) != 0)
			ERR("pthread_detach");
	}
}

int main(int argc, char **argv)
{
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	struct sockaddr_in addr;

	if (argc!=3)
		usage(argv[0]);

	sethandler(SIG_IGN, SIGPIPE);
	sethandler(siginthandler, SIGINT);
	addr = make_address(argv[1], atoi(argv[2]));
	dowork(&addr, &mutex);
	
	return EXIT_SUCCESS;
}
