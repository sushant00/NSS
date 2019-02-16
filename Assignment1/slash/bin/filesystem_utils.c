//Sushant Kumar Singh
//2016103

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_USERS 20
#define MAX_GROUPS_PER_USER 10
#define MAX_USERNAME_LEN 50

#define FILE_END "end"

#define MAX_PWD_LEN 512
#define MAX_LINE_LEN 1024

#define ROOT_PID 0
#define ROOT "fakeroot"
#define ROOT_DIR "slash"
#define DENTRY "/dentry.dir"
#define PATH_PASSWD "etc/passwd"
#define PATH_HOME "slash/home/"


int authFile(char *dir_path, int userID, int mode, int isDir);
int validatePath(char *path);
int putContent(char *path, int clientInd);

//checks if this path is accessible by the server
int validatePath(char *path) {
	printf("validatePath: %s\n", path);
	char absPath[ strlen(PATH_HOME) + MAX_PWD_LEN ];
	char absPathHome[ strlen(PATH_HOME) + MAX_PWD_LEN ];
	realpath(PATH_HOME, absPathHome);
	realpath(path, absPath);
	printf("validatePath: abs path %s\nabs path home %s\n", absPath, absPathHome);
	size_t lenPre = strlen(absPathHome); 
	if ( strncmp(absPath, absPathHome, lenPre) == 0 ) {
		//check if this file exists
		if ( access(path, F_OK) == 0 ) {
			return 0;
		}else {
			printf("validatePath: %s does not exist\n", path);
		}
	}


	printf("validatePath: could not validate path %s\n", path);
	return -1;
}


// //authenticate if this directory is readable by the current user
// //rwx mode
// int authFile(char *path, int mode) {
// 	char file_path[ strlen(PATH_HOME) + MAX_PWD_LEN ];
// 	strcpy(file_path, path);
// 	printf("authFile: authenticating user %s for %s\n", userNames[userID], file_path);
// 	FILE* dentryPtr = fopen(file_path, "r");
// 	if(dentryPtr==NULL){
// 		printf("authFile: some error opening %s\n", file_path);
// 		fclose(dentryPtr);
// 		return -1;
// 	}

// 	char username[MAX_USERNAME_LEN];
// 	char usergrp[MAX_USERNAME_LEN];
// 	int file_mode = 0;
// 	if ( fscanf(dentryPtr, "%s", username)==1 && fscanf(dentryPtr, " %s", usergrp)==1 ){
// 		if( strcmp(username, userNames[userID])==0 || inGrp(usergrp, userID)==0 ) {
// 			file_mode = file_mode ^ 4;
// 			printf("authFile: has read permission. File mode %d\n", file_mode);
// 		}			
// 		if( strcmp(username, userNames[userID])==0 ) {
// 			file_mode = file_mode ^ 2;
// 			printf("authFile: has write permission. File mode %d\n", file_mode);
// 		}

// 		printf("mode: %d file mode: %d bit_and: %d\n", mode, file_mode, mode&file_mode);
// 		if ((mode & file_mode) > 0) {
// 			printf("authenticated user %s mode %d for dir %s\n", userNames[userID], mode, file_path);
// 			//successfully authenticated for mode access
// 			return 0;
// 		}
// 	}

// 	printf("could not authenticate user %s for dir %s\n", userNames[userID], file_path);
// 	return -1;
// }
