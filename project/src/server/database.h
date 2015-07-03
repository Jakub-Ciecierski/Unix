/**
 * This file contains all database (DB) logic.
 * 
 * Defines database directories and file extentions.
 * 
 * Defines all functions for creating entries for DB
 * e.i. players and games.
 * 
 * Defines all functions for searching the DB.
 * */

#ifndef _DATABASE_H_
#define _DATABASE_H_

#include "../util/macros.h"
#include "../util/io.h"
#include "../util/msg_protocol.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>

#define DB_INIT_GAME_ID 0

/**********************************/
/********* BUFFER SIZES ***********/
/**********************************/
/// player file size
#define BD_P_SIZE 1024
/// game file size
#define BD_G_SIZE 1024
#define BD_G_LINE_SIZE 1024
/// filename size
#define DB_FILENAME_SIZE 256

/**********************************/
/*********** BOARD ***************/
/**********************************/

#define DB_BOARD_ROW_SIZE 8
#define DB_BOARD_SIZE DB_BOARD_ROW_SIZE * DB_BOARD_ROW_SIZE

#define DB_BOARD_EMPTY '0'
#define DB_BOARD_PLAYER1 '1'
#define DB_BOARD_PLAYER2 '2'

/**********************************/
/****** DATABASE DIRECTORIES ******/
/**********************************/
#define DB_DIR "database"
#define DB_PLAYER_DIR "database/players"
#define DB_GAME_DIR "database/games"

/**********************************/
/******* DATABASE MUTEX NAME ******/
/**********************************/
#define DB_PLAYER_MUTEX "p_mutex.mx"
#define DB_GAME_MUTEX "g_mutex.mx"

#define DB_PLAYER_MUTEX_PATH "database/players/p_mutex.mx"
#define DB_GAME_MUTEX_PATH "database/games/g_mutex.mx"

/**********************************/
/******* DATABASE EXTANTIONS ******/
/**********************************/
#define DB_PLAYER_EXT ".p"
#define DB_GAME_EXT ".gm"

/**********************************/
/********** FILE HEADERS **********/
/**********************************/
/// GENERAL HEADERS
#define DB_H_EOF "EOF"

/// PLAYER HEADERS
#define DB_H_P_NAME "NAME"
#define DB_H_P_GAMES "GAMES"

/// GAME HEADERS
#define DB_H_G_ID "ID"
#define DB_H_G_STATUS "STATUS"
#define DB_H_G_PLAYERS "PLAYERS"
#define DB_H_G_PL_NA "N/A"
#define DB_H_G_MOVES "MOVES"
#define DB_H_G_CHAT "CHAT"

/// GAME FILE HEADERS
#define DB_H_G_F_ID "id"
#define DB_H_G_F_STATUS "status"
#define DB_H_G_F_PLAYERS "players"
#define DB_H_G_F_MOVES "moves"
#define DB_H_G_F_CHAT "chat"
#define DB_H_G_F_TURN "turn"
#define DB_H_G_F_BOARD "board"

/// GAME STATUSES
#define DB_H_G_SS_ACTIVE 0
#define DB_H_G_SS_WAITING 1
#define DB_H_G_SS_RESOLVED 2

/**********************************/
/*********** FUNCTIONS ************/
/**********************************/

/**
 * Sets F_SETLKW on given file descriptor
 * */
void db_set_lock(int fd, int start, int len, int type);

/**
 * Checks whether a given file exists in directory.
 * 
 * Returns 0 if exists, -1 otherwise
 * */
int db_file_exists(char* dir, char* filename, char* mutex);

/**********************************/
/********* GAME FUNCTIONS *********/
/**********************************/

int db_get_opponent(int id, char* p_name, char* opp);

void db_get_chat(int id, char* buffer);
int db_add_chat_entry(int id, char* p_name, char* msg);

void db_get_board(int id, char* buffer);
int db_board_move(int id, int x1, int y1, int x2, int y2);

int db_get_moves(int id, char* buffer);
int db_add_move(int id, char* move);

/**
 * Reads g_mutex file and gets next available id 
 * which is stored in that file.
 * */
int db_get_next_game_id();

int db_create_game();

int db_join_game();

int db_join_or_create_game(char* p_name);

int db_init_board(int fd);

/**
 * Initiates the game directory according to the standard
 * */
int db_init_game_dir(char* dir, int id);

int db_get_game_status(int id);
int db_set_game_status(int id, int status);

/**
 * DESCRIPTION:
 * 		Adds a player to a Game by ID, locks DB_H_G_F_PLAYERS file
 * 
 * RETURN VALUE:
 * 		0 on success or less than 0 when game has already two players
 * */
int db_game_add_player(int id, char* p_name);

int db_set_player_turn(int id, char* p_name);
char* db_get_player_turn(int id);

/**********************************/
/******** PLAYER FUNCTIONS ********/
/**********************************/

/**
 * Return an array of game ids of size DB_P_GAMES_SIZE, to which the player
 * belongs.
 * */
int* db_player_get_games_id(char* p_name);

/**
 * Adds the player to the game
 * */
int db_player_add_game(int id, char* p_name);

/**
 * Initiates the player file according to the standard
 * */
int db_init_player_file(int fd, char* p_name);

/**
 * Creates player entry.
 * Returns 0 on success.
 * If player exists -1 is returned
 * */
int db_create_player(char* name);

/**
 * Uses db_file_exists to check if player with
 * such name already exists.
 * */
int db_player_exists(char* name);

#endif
