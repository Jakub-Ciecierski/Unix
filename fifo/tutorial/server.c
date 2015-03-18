#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))

void usage(void){
	fprintf(stderr,"USAGE: server fifo_file\n");
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
	return len ;
}

size_t filter_buffer(char* buf, int len) {
	size_t i,j;
	for(i=j=0;i<len;i++)
		if(isalnum(buf[i])) buf[j++]=buf[i];
	return j;
}

void append_to_file (char *filename,char *buf, size_t len){
	int fd;
	if((fd=TEMP_FAILURE_RETRY(open(filename,O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)))<0){
			perror("Open file:");
			exit(EXIT_FAILURE);
	}
	if(bulk_write(fd,buf,len)<0){
		perror("Write:");
		exit(EXIT_FAILURE);
	}
	if(TEMP_FAILURE_RETRY(close(fd))<0){
			perror("Close file:");
			exit(EXIT_FAILURE);
	}
}

void read_from_fifo(int fifo){
	int64_t  count, i;
	char buffer[PIPE_BUF];
	char fname[20];
	do{
		count=bulk_read(fifo,buffer,PIPE_BUF);
		if(count<0){
			perror("Read:");
			exit(EXIT_FAILURE);
		}
		if(count>0){
			// *((pid_t*)buffer)
			// We cast buffer to pid_t and then dereference it.
			// After the casting, the pointer will look at the same memory
			// but will think that it points at pid_t value.
			// Thus if we dereference first element, it will yeild the value of sizeof(pid_t) bytes
			snprintf(fname,20,"%d.txt",*((pid_t*)buffer));
			i=filter_buffer(buffer+sizeof(pid_t),MSG_SIZE);
			append_to_file(fname,buffer+sizeof(pid_t),i);
		}
	}while(count==PIPE_BUF);
}

int main(int argc, char** argv) {
	int fifo;
	if(argc!=2) {
		usage();
		return EXIT_FAILURE;
	}

	/*
	 * mkfifo() makes a FIFO special file with name pathname.  
	 * mode specifies the FIFO's permissions.  
	 * It is modified by the process's umask in the usual way: 
	 * the permissions of the created file are (mode & ~umask).
	 */
	if(mkfifo(argv[1], S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)<0)
		if(errno!=EEXIST){
			perror("Create fifo:");
			exit(EXIT_FAILURE);
		}
	if((fifo=TEMP_FAILURE_RETRY(open(argv[1],O_RDONLY)))<0){
			perror("Open fifo:");
			exit(EXIT_FAILURE);
	}
	read_from_fifo(fifo);	
	if(TEMP_FAILURE_RETRY(close(fifo))<0){
			perror("Close fifo:");
			exit(EXIT_FAILURE);
	}
	if(unlink(argv[1])<0){
			perror("Remove fifo:");
			exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}
