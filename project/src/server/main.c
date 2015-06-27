#include "server.h"

void usage(char * name){
	fprintf(stderr,"USAGE: %s port workdir \n",name);
}

int main(int argc, char** argv)
{
	int port;
	
	if(argc != 3) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[1]);
	if(port < 0){
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// switch to new working dir
	if (chdir(argv[2]) == -1) ERR("chdir");
	
	init_server(port);

	return EXIT_SUCCESS;
}
