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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>

/**********************************/
/********* BUFFER SIZES ***********/
/**********************************/
/// player file size
#define BD_P_SIZE 1024
/// filename size
#define DB_FILENAME_SIZE 256

/**********************************/
/****** DATABASE DIRECTORIES ******/
/**********************************/
#define DB_DIR "database"
#define DB_PLAYER_DIR "database/players"
#define DB_GAME_DIR "database/games"

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

/**********************************/
/*********** FUNCTIONS ************/
/**********************************/

/**
 * Initiates the player file according to the standart
 * */
int db_init_player_file(int fd, char* p_name);

/**
 * Creates player entry.
 * Returns 0 on success.
 * If player exists -1 is returned
 * */
int db_create_player(char* name);

/**
 * Checks whether a given file exists in directory.
 * 
 * Returns 0 if exists, -1 otherwise
 * */
int db_file_exists(char* dir, char* filename);

#endif

