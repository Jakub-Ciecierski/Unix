#include "macros.h"
#include "io.h"
#include "sockets.h"

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

#define MAX_SLEEP_MS 2000
#define MIN_SLEEP_MS 500

#define MILLI 1000
#define MICRO 1000000

volatile sig_atomic_t last_signal=0;

volatile sig_atomic_t do_work=1;

void sigalrm_handler(int sig) {
	last_signal=sig;
	do_work=0;
}

void calculate(int32_t* n)
{
	fprintf(stdout,"[%d] Received N: %d \n",getpid(), *n);
	(*n)++;
}

void communicate(int fd, struct sockaddr_in addr, int32_t* n)
{
	/***** SEND DATA *****/
	*n = htonl(*n);

	fprintf(stderr,"[Client] sending data \n");
	while(sendto(fd,(char *)n, sizeof(int32_t),0,
					(struct sockaddr*)&addr,sizeof(addr))<0)
	{
		if(EINTR!=errno && errno!=EPIPE && errno!=ECONNRESET)ERR("sendto:");
		if(SIGALRM==last_signal) {
			fprintf(stderr,"[Client] timeout reached... \n");
			return;
		}
	}

	/***** RECEIVE DATA *****/
	fprintf(stderr,"[Client] waiting for response \n");
	socklen_t len = sizeof(struct sockaddr_in);

	while(recvfrom(fd,(char *)n,sizeof(int32_t),0,
					(struct sockaddr*)&addr,&len)<0)
	{
		if(EINTR!=errno && errno!=EPIPE && errno!=ECONNRESET)ERR("recvfrom:");
		if(SIGALRM==last_signal) {
			fprintf(stderr,"[Client] timeout reached... \n");
			return;
		}
	}
	
	*n = ntohl(*n);
	calculate(n);
}

void setalarm()
{
	srand(getpid());
	int t = rand()%(MAX_SLEEP_MS - MIN_SLEEP_MS) + MIN_SLEEP_MS;
	fprintf(stderr,"[Client] my lifetime in ms: %d \n", t);

	int sec = t / MILLI;
	int milli_sec = t%MILLI;
	int micro_sec = milli_sec * (MICRO/MILLI);

	fprintf(stderr,"[Client] my lifetime in s: %d us: %d\n", sec, micro_sec);

	struct itimerval ts;

	memset(&ts, 0, sizeof(struct itimerval));
	ts.it_value.tv_sec=sec;
	ts.it_value.tv_usec=micro_sec;

	setitimer(ITIMER_REAL,&ts,NULL);
}

void client_work(int fd, struct sockaddr_in addr)
{
	fprintf(stderr,"Client work \n");

	setalarm();

	int32_t n = 1;

	while(do_work)
	{
		communicate(fd, addr, &n);
	}

	fprintf(stdout,"\n >> Client says a nice goodbye. N=%d \n",n);
}

void usage(char * name){
	fprintf(stderr,"USAGE: %s domain port \n",name);
}

int main(int argc, char** argv) {
	int fd;
	struct sockaddr_in addr;

	if(argc!=3) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	
	if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
	if(sethandler(sigalrm_handler,SIGALRM)) ERR("Seting SIGALRM:");
	
	fd = make_socket(PF_INET, SOCK_DGRAM);
	addr=make_inet_address(argv[1],atoi(argv[2]));

	client_work(fd, addr);

	if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
	return EXIT_SUCCESS;
}
