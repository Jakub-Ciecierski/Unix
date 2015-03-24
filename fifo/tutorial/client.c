#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))

void usage(void){
	fprintf(stderr,"USAGE: client fifo_file file \n");
}

int sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void pipe_handler(int sig) {
	printf("[%d] received SIGPIPE and must terminate.\n", getpid());
}

int64_t  bulk_read(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(read(fd,buf,count));
		if(c<0) return c;
		if(c==0) return len; //EOF
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}

int64_t  bulk_write(int fd, char *buf, size_t count){
	int c;
	size_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(write(fd,buf,count));
		if(c<0) return c;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len;
}

void write_to_fifo(int fifo, int file){
	int64_t  count;
	char buffer[PIPE_BUF];
	char *buf;
	*((pid_t *)buffer)=getpid();

	buf=buffer+sizeof(pid_t); // Why not buffer+=sizeof(pid_t)
	
	do{
		count=bulk_read(file,buf,MSG_SIZE);
		if(count<0){
			perror("Read:");
			exit(EXIT_FAILURE);
		}
		if(count < MSG_SIZE) memset(buf+count,0,MSG_SIZE-count);
		if(count>0){
			// bulk_write is actually not needed
			// becouse writing to fifo with less or equal to
			// PIPE_BUF is atomix
			if(bulk_write(fifo,buffer,PIPE_BUF)<0){
			perror("Write:");
			exit(EXIT_FAILURE);
			}
		}
	}while(count==MSG_SIZE);
}

int main(int argc, char** argv) {
	int fifo,file;
	if(argc!=3) {
		usage();
		return EXIT_FAILURE;
	}

	if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)<0)
		if(errno!=EEXIST){
			perror("Create fifo:");
			exit(EXIT_FAILURE);
		}
	if((fifo=TEMP_FAILURE_RETRY(open(argv[1],O_WRONLY)))<0){
			perror("Open fifo:");
			exit(EXIT_FAILURE);
	}
	if((file=TEMP_FAILURE_RETRY(open(argv[2],O_RDONLY)))<0){
			perror("Open file:");
			exit(EXIT_FAILURE);
	}
	if(sethandler(pipe_handler,SIGPIPE)) {
		perror("Seting SIGPIPE:");
		exit(EXIT_FAILURE);
	}
	write_to_fifo(fifo,file);	
	if(TEMP_FAILURE_RETRY(close(file))<0){
			perror("Close file:");
			exit(EXIT_FAILURE);
	}
	if(TEMP_FAILURE_RETRY(close(fifo))<0){
			perror("Close fifo:");
			exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
