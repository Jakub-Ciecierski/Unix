#include "client.h"

void prepare_send_local_data(char** argv, int32_t data[])
{
	data[0]=htonl(atoi(argv[2])); 			// op1
	data[1]=htonl(atoi(argv[3])); 			// op2
	data[2]=htonl(0);						// result
	data[3]=htonl((int32_t)(argv[4][0]));	// operation
	data[4]=htonl(1);						// status
}

void prepare_send_inet_data(char** argv, int32_t data[])
{
	data[0]=htonl(atoi(argv[3])); 			// op1
	data[1]=htonl(atoi(argv[4])); 			// op2
	data[2]=htonl(0);						// result
	data[3]=htonl((int32_t)(argv[5][0]));	// operation
	data[4]=htonl(1);						// status
}


void prepare_revc_data(int32_t data[])
{
	data[0]=ntohl(data[0]);
	data[1]=ntohl(data[1]);
	data[2]=ntohl(data[2]);
	data[3]=ntohl(data[3]);
	data[4]=ntohl(data[4]);
}

void analyze_result(int32_t data[])
{
	if(data[4])
		fprintf(stdout,"%d %c %d = %d \n", 
				data[0], (char)data[3], data[1], data[2]);
	else
		fprintf(stdout,"Operation impossible \n");
}

void client_work(int fd, int32_t data[])
{
	fprintf(stderr,"[Client] sending data \n");
	if(bulk_write(fd, (char*)data, sizeof(int32_t[5])) <0 && errno!=EPIPE)
		ERR("bulk_write");
	fprintf(stderr,"[Client] waiting for response \n");
	if(bulk_read(fd, (char*)data, sizeof(int32_t[5])) <0 && errno!=EPIPE)
		ERR("bulk_write");
	fprintf(stderr,"[Client] received response \n");
	
	prepare_revc_data(data);
	analyze_result(data);
	
	sleep(2);
}
