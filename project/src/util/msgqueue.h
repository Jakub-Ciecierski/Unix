#ifndef _MSGQUEUE_H_
#define _MSGQUEUE_H_

#include "macros.h"
#include <linux/ipc.h>
#include <linux/msg.h>

/**
 * Buffer used in message queue communication
 * */
struct msgqbuf {
	/* Message type */
	long    mtype;
	/* Request identifier */
	long    request_id;
	/* data information */
	char* data;   				
};

/**
 * Opens message queue and returns its id
 * */
void open_msgqueue(key_t keyvalue);

/**
 * Sends message
 * */
int send_message(int qid, struct msgqbuf *qbuf);

/**
 * Receives message
 * */
int recv_message(int qid, long type, struct msgqbuf *qbuf);

#endif
