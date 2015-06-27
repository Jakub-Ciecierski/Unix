#include "database.h"

int db_init_player_file(int fd, char* p_name)
{
	char buffer[BD_P_SIZE];
	
	// create a buffer
	int size;
	if((size = sprintf(buffer, "%s\n%s\n%s\n%s", DB_H_P_NAME, p_name, DB_H_P_GAMES, DB_H_EOF)) < 0) ERR("sprintf");
	
	if(memset(buffer+size, 0, BD_P_SIZE-size) < 0 ) ERR("memeset");
	
	fprintf(stderr, "[DB] Writing file: \n%s\n", buffer);
	
	// write it to the file
	if(bulk_write(fd, buffer, BD_P_SIZE) < 0) ERR("bulk_read");
	
	return 0;
}

int db_create_player(char* p_name)
{
	char filename[DB_FILENAME_SIZE];
	int fd;
	
	// TODO should lock dir here ?

	// create filename: name + player extention
	if(sprintf(filename, "%s%s", p_name, DB_PLAYER_EXT) < 0) ERR("sprintf");
	
	fprintf(stderr, "[DB] Adding filename: %s\n", filename);

	// check if player exists
	if(db_file_exists(DB_PLAYER_DIR, filename) < 0 )
		return -1;

	char* filepath[DB_FILENAME_SIZE];
	if(sprintf(filepath, "%s/%s", DB_PLAYER_DIR, filename) < 0) ERR("sprintf");
	
	// create the file
	if((fd = TEMP_FAILURE_RETRY(open(filepath,  O_CREAT|O_WRONLY, 0777))) < 0) ERR("open");

	// init the file with bytes
	db_init_player_file(fd, p_name);

	// close the fd
	if(TEMP_FAILURE_RETRY(close(fd)) < 0) ERR("close");

	return 0;
}

/**
 * TODO Lock dir ?
 * */
int db_file_exists(char* dir, char* filename)
{
	DIR* dirp;
	struct dirent* dp;
	
	fprintf(stderr, "[DB] Before opendir\n");
	if(NULL == (dirp = opendir(dir))) ERR("opendir");
	fprintf(stderr, "[DB] After opendir\n");
	
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			fprintf(stderr, "[DB] filename: %s\n", dp->d_name);
			if((strcmp(filename, dp->d_name) == 0)) {
				TEMP_FAILURE_RETRY(closedir(dirp));
				return -1;
			}
		}
	} while(dp != NULL);

	if(errno != 0) ERR("readdir");
	TEMP_FAILURE_RETRY(closedir(dirp));

	return 0;
}
