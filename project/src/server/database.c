#include "database.h"

/**********************************/
/*********** FUNCTIONS ************/
/**********************************/

int db_file_exists(char* dir, char* filename, char* mutex)
{
	int ret_val;
	int mutex_fd;
	DIR* dirp;
	struct dirent* dp;
	
	// assumes that it does not exist
	ret_val = -1;
	
	// lock access to player dir
	if((mutex_fd = TEMP_FAILURE_RETRY(open(mutex,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");
	db_set_lock(mutex_fd, 0, 0, F_WRLCK);
	
	fprintf(stderr, "[DB] Before opendir\n");
	if(NULL == (dirp = opendir(dir))) ERR("opendir");
	fprintf(stderr, "[DB] After opendir\n");
	
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			fprintf(stderr, "[DB] filename: %s\n", dp->d_name);
			if((strcmp(filename, dp->d_name) == 0)) {
				ret_val = 0;
				break;
			}
		}
	} while(dp != NULL);

	if(errno != 0) ERR("readdir");
	
	TEMP_FAILURE_RETRY(closedir(dirp));
	
	// unlock access to player dir
	db_set_lock(mutex_fd, 0, 0, F_UNLCK);
	if(TEMP_FAILURE_RETRY(close(mutex_fd)) < 0) ERR("close");

	return ret_val;
}

/**
 *	F_RDLCK
 *		This macro is used to specify a read (or shared) lock.
 *	F_WRLCK
 *		This macro is used to specify a write (or exclusive) lock.
 *	F_UNLCK
 * 		This macro is used to specify that the region is unlocked. 
 * 
 * */
void db_set_lock(int fd, int start, int len, int type)
{
	struct flock l;
	l.l_whence = SEEK_SET;
	l.l_start = start;
	l.l_len = len;
	l.l_type = type;
	if(-1 == TEMP_FAILURE_RETRY(fcntl(fd, F_SETLKW,&l))) ERR("fcntl");
}

/**********************************/
/********* GAME FUNCTIONS *********/
/**********************************/

int db_get_next_game_id()
{
	int size, memset_size;
	int id, next_id;
	int mutex_fd;
	char write_buffer[BD_G_SIZE];
	char read_buffer[BD_G_SIZE];

	// lock mutex
	if((mutex_fd = TEMP_FAILURE_RETRY(open(DB_GAME_MUTEX_PATH, O_CREAT|O_RDWR, 0777))) < 0) ERR("open");
	db_set_lock(mutex_fd, 0, 0, F_WRLCK);
	
	// read current global id
	if(bulk_read(mutex_fd, read_buffer, BD_G_SIZE) < 0) ERR("bulk_read");
	
	fprintf(stderr, "[DB] Read g_mutex: \n%s\n", read_buffer);
	
	if((id = atoi(read_buffer)) < 0) {
		fprintf(stderr, "[DB] WRONG ID REAd\n");
		id = DB_INIT_GAME_ID;
	}
	fprintf(stderr, "[DB] id: %d\n", id);
	
	// reset the content of file
	ftruncate(mutex_fd, 0);
	lseek(mutex_fd, 0, SEEK_SET);
	
	// write to buffer
	next_id = id + 1;
	if((size = sprintf(write_buffer, "%d", next_id)) < 0) ERR("sprintf");
	memset_size = BD_G_SIZE - size;
	if(memset(write_buffer+size, 0, memset_size) < 0 ) ERR("memeset");
	fprintf(stderr, "[DB] write_buffer: \n%s\n", write_buffer);
	
	if(bulk_write(mutex_fd, write_buffer, BD_G_SIZE) < 0) ERR("bulk_write");
	
	// unlock access to player dir
	db_set_lock(mutex_fd, 0, 0, F_UNLCK);
	if(TEMP_FAILURE_RETRY(close(mutex_fd)) < 0) ERR("close");	
	
	return id;
}


int db_init_game_dir(char* dir, int id)
{
	int size;
	int fd;
	char filepath[DB_FILENAME_SIZE];
	char buffer[BD_G_SIZE];
	
	/// ID
	if(sprintf(filepath, "%s/%s%s", dir, DB_H_G_F_ID, DB_GAME_EXT) < 0) ERR("sprintf");
	if((fd = TEMP_FAILURE_RETRY(open(filepath, O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");
	
	if((size = sprintf(buffer, "%d", id)) < 0) ERR("sprintf");
	if(memset(buffer+size, 0, BD_G_SIZE-size) < 0 ) ERR("memeset");
	if(bulk_write(fd, buffer, size) < 0) ERR("bulk_write");
	
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");
	
	/// STATUS
	if(sprintf(filepath, "%s/%s%s", dir, DB_H_G_F_STATUS, DB_GAME_EXT) < 0) ERR("sprintf");
	if((fd = TEMP_FAILURE_RETRY(open(filepath,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");
	
	if((size = sprintf(buffer, "%d", DB_H_G_SS_WAITING)) < 0) ERR("sprintf");
	if(memset(buffer+size, 0, BD_G_SIZE-size) < 0 ) ERR("memeset");
	if(bulk_write(fd, buffer, size) < 0) ERR("bulk_write");
	
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");

	/// PLAYERS
	if(sprintf(filepath, "%s/%s%s", dir, DB_H_G_F_PLAYERS, DB_GAME_EXT) < 0) ERR("sprintf");
	if((fd = TEMP_FAILURE_RETRY(open(filepath,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");
	
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");
	
	/// MOVES
	if(sprintf(filepath, "%s/%s%s", dir, DB_H_G_F_MOVES, DB_GAME_EXT) < 0) ERR("sprintf");
	if((fd = TEMP_FAILURE_RETRY(open(filepath,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");
	
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");
	
	/// CHAT
	if(sprintf(filepath, "%s/%s%s", dir, DB_H_G_F_CHAT, DB_GAME_EXT) < 0) ERR("sprintf");
	if((fd = TEMP_FAILURE_RETRY(open(filepath,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");
	
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");
}

int db_get_game_status(int id)
{
	int fd;
	int status;
	char filepath[DB_FILENAME_SIZE];
	char buffer[BD_G_LINE_SIZE];
	
	if(sprintf(filepath, "%s/%d/%s%s", DB_GAME_DIR, id, DB_H_G_F_STATUS, DB_GAME_EXT) < 0) ERR("sprintf");
	
	if((fd = TEMP_FAILURE_RETRY(open(filepath, O_CREAT|O_RDWR, 0777))) < 0) ERR("open");
	
	if( bulk_read(fd, buffer, BD_G_LINE_SIZE) < 0 ) ERR("bulk_read");
	
	status = atoi(buffer);
	
	if(sscanf(buffer, "%d", &status) < 0) ERR("sprintf");
	
	fprintf(stderr, "[DB] Status: %d, buffer: %s\n", status, buffer);
	
	return status;
}

int db_set_game_status(int id, int status)
{
	int fd;
	int size;
	char filepath[DB_FILENAME_SIZE];
	char buffer[BD_G_LINE_SIZE];
	
	if(sprintf(filepath, "%s/%d/%s%s", DB_GAME_DIR, id, DB_H_G_F_STATUS, DB_GAME_EXT) < 0) ERR("sprintf");
	if((fd = TEMP_FAILURE_RETRY(open(filepath, O_RDWR, 0777))) < 0) ERR("open");
	
	// reset the content of file
	ftruncate(fd, 0);
	lseek(fd, 0, SEEK_SET);
	
	if((size = sprintf(buffer, "%d", status)) < 0) ERR("sprintf");
	if(memset(buffer+size, 0, BD_G_SIZE-size) < 0 ) ERR("memeset");
	if(bulk_write(fd, buffer, size) < 0) ERR("bulk_write");
	
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");	
}

int db_create_game()
{
	int id;
	char dir[DB_FILENAME_SIZE];
	
	// get next available id
	id = db_get_next_game_id();
	
	if(sprintf(dir, "%s/%d", DB_GAME_DIR, id) < 0) ERR("sprintf");

	if(mkdir(dir, 0777) < 0)
		if(errno != EEXIST) ERR("mkdir");

	db_init_game_dir(dir, id);
	
	return id;
}

int db_join_game()
{
	int mutex_fd;
	DIR* dirp;
	struct dirent* dp;
	char filepath[DB_FILENAME_SIZE];

	fprintf(stderr, "[DB] Before opendir\n");
	if(NULL == (dirp = opendir(DB_GAME_DIR))) ERR("opendir");
	fprintf(stderr, "[DB] After opendir\n");

	do {
		int id;
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if(dp->d_type == DT_DIR) {
				if((id = atoi(dp->d_name)) < 0 || 
					strcmp(dp->d_name, ".") == 0 || 
					strcmp(dp->d_name, "..") == 0) continue;
				fprintf(stderr, "DIR: %s, id: %d\n", dp->d_name, id);
				if(db_get_game_status(id) == DB_H_G_SS_WAITING){
					// Change the status of Game to ACTIVE
					db_set_game_status(id, DB_H_G_SS_ACTIVE);
					return id;
				}
			}
		}
	} while(dp != NULL);
	
	if(errno != 0) ERR("readdir");

	TEMP_FAILURE_RETRY(closedir(dirp));

	return -1;
}

int db_join_or_create_game(char* p_name)
{
	int id, mutex_fd;
	
	// lock access to player dir
	if((mutex_fd = TEMP_FAILURE_RETRY(open(DB_GAME_MUTEX_PATH,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");
	db_set_lock(mutex_fd, 0, 0, F_WRLCK);

	if((id = db_join_game()) < 0){
		id = db_create_game();
	}

	db_game_add_player(id, p_name);
	db_player_add_game(id, p_name);

	// unlock access to player dir
	db_set_lock(mutex_fd, 0, 0, F_UNLCK);
	if(TEMP_FAILURE_RETRY(close(mutex_fd)) < 0) ERR("close");
	
	return id;
}

int db_game_add_player(int id, char* p_name)
{
	int ret_val;
	int size;
	int fd, mutex_fd;
	FILE* f;
	char filepath[DB_FILENAME_SIZE];
	char buffer[BD_G_LINE_SIZE];
	char line[BD_G_LINE_SIZE];
	char* err_buffer;

	if(sprintf(filepath, "%s/%d/%s%s", DB_GAME_DIR, id, DB_H_G_F_PLAYERS, DB_GAME_EXT) < 0) ERR("sprintf");
	
	if((f = fopen(filepath, "r+")) == NULL) {
		if(errno == ENOENT){
			fprintf(stderr, "[DB] Game with id: %d does not exist\n",id);
			return -1;
		}
		ERR("open");
	}
	fd = fileno(f);
	
	fprintf(stderr, "[DB] After add_player fopen\n");

	if((size = sprintf(buffer, "%s\n", p_name)) < 0) ERR("sprintf");
	if(memset(buffer+size, 0, BD_G_LINE_SIZE-size) < 0 ) ERR("memeset");

	// check first entry
	err_buffer = fgets(line, BD_G_LINE_SIZE, f);
	line[strcspn(line, "\n")] = 0;
	if(err_buffer == NULL || strcmp(line, "\0") == 0) {
		fprintf(stderr, "[DB] Adding first player: %s\n", line);
		if(bulk_write(fd, buffer, size) < 0 ) ERR("bulk_write");
		ret_val = 0;
	}
	// check second entry
	else{
		err_buffer = fgets(line, BD_G_LINE_SIZE, f);
		line[strcspn(line, "\n")] = 0;
		
		if(err_buffer == NULL || strcmp(line, "\0") == 0) {
			if(bulk_write(fd, buffer, size) < 0 ) ERR("bulk_write");
			fprintf(stderr, "[DB] Adding player2: %s\n", line);
			ret_val = 0;
		}else ret_val = -1;
	}

	// unlock access to player dir
	if(TEMP_FAILURE_RETRY(fclose(f)) < 0) ERR("close");	

	return ret_val;
}


/**********************************/
/******** PLAYER FUNCTIONS ********/
/**********************************/

int* db_player_get_games_id(char* p_name)
{
	char filepath[DB_FILENAME_SIZE];
	FILE* f;
	
	if(snprintf(filepath, DB_FILENAME_SIZE, "%s/%s%s", DB_PLAYER_DIR, p_name, DB_PLAYER_EXT) < 0) ERR("sprintf");
	
	if((f = fopen(filepath, "r")) == NULL) ERR("fopen");
	
	// ... todo
}

int db_init_player_file(int fd, char* p_name)
{
	char buffer[BD_P_SIZE];
	
	// create a buffer
	int size;
	if((size = snprintf(buffer, BD_P_SIZE, "%s\n%s\n%s\n%s", DB_H_P_NAME, p_name, DB_H_P_GAMES, DB_H_EOF)) < 0) ERR("sprintf");
	
	if(memset(buffer+size, 0, BD_P_SIZE-size) < 0 ) ERR("memeset");
	
	fprintf(stderr, "[DB] Writing file: \n%s\n", buffer);
	
	// write it to the file
	if(bulk_write(fd, buffer, size) < 0) ERR("bulk_write");
	
	return 0;
}

int db_player_add_game(int id, char* p_name)
{
	char filename[DB_FILENAME_SIZE];
	char filepath[DB_FILENAME_SIZE];
	char buffer[BD_P_SIZE];
	int size;
	int fd;
	FILE* f;
	
	// create filename: name + player extention
	if(snprintf(filename, DB_FILENAME_SIZE, "%s%s", p_name, DB_PLAYER_EXT) < 0) ERR("sprintf");
	if(snprintf(filepath, DB_FILENAME_SIZE, "%s/%s", DB_PLAYER_DIR, filename) < 0) ERR("sprintf");
	
	if((f = fopen(filepath, "a")) == NULL) ERR("open");
	fd = fileno(f);
	
	if((size = snprintf(buffer, BD_P_SIZE, "%d\n", id)) < 0) ERR("sprintf");
	if(memset(buffer+size, 0, BD_P_SIZE-size) < 0 ) ERR("memeset");
	
	// write it to the file
	if(bulk_write(fd, buffer, size) < 0) ERR("bulk_write");
	
	if(TEMP_FAILURE_RETRY(fclose(f)) < 0) ERR("close");
	
	return 0;
}

int db_create_player(char* p_name)
{
	char filename[DB_FILENAME_SIZE];
	int fd;
	
	// create filename: name + player extention
	if(snprintf(filename, DB_FILENAME_SIZE, "%s%s", p_name, DB_PLAYER_EXT) < 0) ERR("sprintf");
	
	// check if player exists
	if(db_file_exists(DB_PLAYER_DIR, filename, DB_PLAYER_MUTEX_PATH) == 0 ) return -1;

	char filepath[DB_FILENAME_SIZE];
	if(snprintf(filepath, DB_FILENAME_SIZE, "%s/%s", DB_PLAYER_DIR, filename) < 0) ERR("sprintf");
	
	// create the file
	if((fd = TEMP_FAILURE_RETRY(open(filepath,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");

	// close the fd
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");

	return 0;
}

int db_player_exists(char* p_name)
{
	char filename[DB_FILENAME_SIZE];

	// create filename: name + player extention
	if(snprintf(filename, DB_FILENAME_SIZE, "%s%s", p_name, DB_PLAYER_EXT) < 0) ERR("sprintf");

	// check if player exists
	int ret = db_file_exists(DB_PLAYER_DIR, filename, DB_PLAYER_MUTEX_PATH);

	return ret;
}

