#include "msg_protocol.h"

int cmp_send(int fd, char* header, char* msg)
{
	char buffer[CMP_BUFFER_SIZE];
	
	if(msg == NULL) {
		if(snprintf(buffer, CMP_BUFFER_SIZE, "%s", header) < 0) ERR("sprintf");
	}
	else {
		if(snprintf(buffer, CMP_BUFFER_SIZE, "%s%s", header, msg) < 0) ERR("sprintf");
	}

	if(bulk_write(fd, buffer, CMP_BUFFER_SIZE) < 0 ) {
		if(errno != EPIPE)
			ERR("bulk_write");
		return -1;
	}
	
	fprintf(stderr, "[CMP] Message sent: \n%s\n\n", buffer);
	
	return 0;
}

