#include "msgqueue.h"

void open_msgqueue(key_t keyvalue)
{
	int qid;
	
	if((qid = msgget( keyvalue, IPC_CREAT | 0660 )) < 0) ERR("msgget");
	
	return(qid);
}

int send_message(int qid, struct msgqbuf *qbuf)
{
	int result, length;

	/* The length is essentially the size of the structure minus sizeof(mtype) */
	length = sizeof(struct msgqbuf) - sizeof(long);        

	if((result = msgsnd( qid, qbuf, length, 0)) < 0) ERR("msgsnd");
	
	return(result);
}

int recv_message(int qid, long type, struct msgqbuf *qbuf)
{
	int result, length;

	/* The length is essentially the size of the structure minus sizeof(mtype) */
	length = sizeof(struct msgqbuf) - sizeof(long);        

	if((result = msgrcv(qid, qbuf, length, type,  0)) < 0) ERR("msgrcv");
	
	return(result);
}
