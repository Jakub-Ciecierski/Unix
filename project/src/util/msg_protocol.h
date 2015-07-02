/**
 * This header defines Communication Message Protocol - CMP
 * 
 * Each Message is decoded with a header built of two chars
 * indicating what kind of message it is
 * */

#ifndef _MSG_PROTOCOL_H_
#define _MSG_PROTOCOL_H_

/**********************************/
/************ BUFFERS *************/
/**********************************/
#define CMP_HEADER_SIZE 2*sizeof(char)
#define CMP_MSG_SIZE 256
#define CMP_BUFFER_SIZE CMP_MSG_SIZE + CMP_HEADER_SIZE

/**********************************/
/******** PROTOCOL HEADERS ********/
/**********************************/
/// CLIENT SIDE
#define CMP_REGISTER "re"
#define CMP_LOGIN "ll"
#define CMP_GAME_NEW "gn"
#define CMP_GAME_EXT "ge"
#define CMP_STATUS "ss"

/// SERVER SIDE
#define CMP_REGISTER_ACC "ra"
#define CMP_REGISTER_REJ "rr"

#define CMP_LOGIN_ACC "la"
#define CMP_LOGIN_REJ "lr"

/**********************************/
/********* CLIENTS STATES *********/
/**********************************/
#define CMP_S_LOGGED_OUT 0
#define CMP_S_LOGGED_IN 1
#define CMP_S_IN_GAME 2

/**
 * Gets the header of this message
 * */
 /*
void cmp_get_header(char* header, char* msg)
{
	strncpy(header, msg, CMP_HEADER_SIZE);
}
*/
#endif

