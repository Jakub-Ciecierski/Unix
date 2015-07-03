/**
 * This header defines Communication Message Protocol - CMP
 * 
 * Each Message is decoded with a header built of two chars
 * indicating what kind of message it is
 * */

#ifndef _MSG_PROTOCOL_H_
#define _MSG_PROTOCOL_H_

#include "io.h"

/**********************************/
/************ BUFFERS *************/
/**********************************/

#define CMP_HEADER_SIZE 2*sizeof(char)
#define CMP_MSG_SIZE 512
#define CMP_BUFFER_SIZE CMP_MSG_SIZE + CMP_HEADER_SIZE

/// How many games will be read from player profile minus 1
/// to keep the end of array codition - DB_P_EOA
#define CMP_P_GAMES_SIZE 25
/// If found this value, then previous value was the last game id in the array
#define CMP_P_EOA -1

/**********************************/
/******** PROTOCOL HEADERS ********/
/**********************************/

/// CLIENT SIDE
#define CMP_REGISTER "re"
#define CMP_LOGIN "ll"
#define CMP_GAME_NEW "gn"
#define CMP_GAME_EXT "ge"
#define CMP_STATUS "st"

#define CMP_BOARD "bd"
#define CMP_GAME_STATUS "gs"
#define CMP_TURN "tn"
#define CMP_MOVES "ms"
#define CMP_CHAT "ch"
#define CMP_MSG "mg"
#define CMP_MOVE "mv"
#define CMP_QUIT "qt"
#define CMP_FORFEIT "ff"

/// SERVER SIDE
#define CMP_ALIVE "al"

#define CMP_REGISTER_ACC "ra"
#define CMP_REGISTER_REJ "rr"

#define CMP_LOGIN_ACC "la"
#define CMP_LOGIN_REJ "lr"

#define CMP_GAME_JOIN_ACC "ga"
#define CMP_GAME_JOIN_REJ "gr"

#define CMP_STATUS_RESPONSE "sr"

#define CMP_BOARD_RESPONSE "br"
#define CMP_GAME_STATUS_RESPONSE "or"
#define CMP_TURN_RESPONSE "tr"
#define CMP_MOVES_RESPONSE "mr"
#define CMP_CHAT_RESPONSE "cr"
#define CMP_MSG_RESPONSE "pr"
#define CMP_MOVE_RESPONSE_ACC "va"
#define CMP_MOVE_RESPONSE_REJ "vr"
#define CMP_QUIT_RESPONSE "qr"
#define CMP_FORFEIT_RESPONSE "fr"

/**********************************/
/********* CLIENTS STATES *********/
/**********************************/

#define CMP_S_LOGGED_OUT 0
#define CMP_S_LOGGED_IN 1
#define CMP_S_IN_GAME 2


int cmp_send(int fd, char* header, char* msg);

#endif

