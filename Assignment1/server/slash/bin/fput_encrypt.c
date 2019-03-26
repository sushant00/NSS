//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"
#include "enc_utils.c"

int putEncryptedContent(char *path);
int getOldContent(char *path, char *oldContent);
int getNewContent(char *newContent);

int fput_encrypt(int argc, char **args){
	printf("fput_encrypt: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	char *ptr = strrchr(args[0], '/');
	*ptr = 0;

	if( validatePath(args[0])==-1) {
		printf("fput_encrypt: validation of file path %s failed\n", args[0]);
		return -1;
	}

	*ptr = '/';
	if ( access(args[0], F_OK) == 0 ) {
		printf("fput_encrypt: %s file already created\n", args[0]);
		//authenticate write access for file, as file is already created
		if(authPerm(args[0], 2) == -1){
			printf("fput_encrypt: permission denied for file %s\n", args[0]);
			return -1;
		}
	} else {
		//authenticate write access to directory as we are about to create a file in it
		*ptr = 0;
		if(authPerm(args[0], 2) == -1){
			printf("fput_encrypt: permission denied for file %s\n", args[0]);
			return -1;
		}

		*ptr = '/';

		//create the file
		FILE* file = fopen(args[0], "w+");
		fclose(file);
		chown(args[0], getuid(), getgid());
		if ( inheritAcl(args[0])==-1 ){
			printf("fput_encrypt: error inheriting acls from parent for %s\n", args[0]);
			return -1;
		}
	}


	//authenticated write access
	printf("fput_encrypt: authenticated for write access %s\n", args[0]);

	unsigned char *key = getKeyUser(getuid());
	// unsigned char *iv = getIVUser(getuid());

	char *outputContent = malloc(sizeof(char)*MAX_FILE_LEN);
	char *encContent = malloc(sizeof(char)*(MAX_FILE_LEN + strlen(key) ));
	int oldLen = getOldContent(args[0], (char *)outputContent);

	printf("fput_encrypt: enter the content to put. Type '%s' to finish writing.\n", FILE_END);

	getNewContent(outputContent + oldLen);
	putEncryptedContent(args[0]);

	if (seteuid(getuid())==-1){
		printf("fput_encrypt: error setting euid\n");
	}
	printf("fput_encrypt: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	
	return 0;
}


int putEncryptedContent(char *path) {
	// printf("putEncryptedContent: to %s\n", path);
	FILE* filePtr = fopen(path, "a");
	if(filePtr==NULL) {
		printf("putContent: some error opening %s for append\n", path);
		return -1;
	}

	fclose(filePtr);
	printf("putEncryptedContent: finished putting encrypted content\n");
	return 0;
}

int getOldContent(char *path, char *oldContent){
	return 0;
}

int getNewContent(char *newContent) {
	printf("getNewContent: called\n");

	size_t inputlen = MAX_LINE_LEN*sizeof(char);
	char *input = malloc(inputlen);
				
	//take line input
	while(1){
		fgets(input, inputlen, stdin);
		char *newline = strrchr(input, '\n');
		*newline = 0;

		//end of file encountered
		if(strcmp(FILE_END, input) == 0) {
			break;
		}
		int wrote = sprintf(newContent, "%s\n", input);	
		newContent = newContent+wrote;
	}

	printf("getNewContent: finished receiving\n");
	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return fput_encrypt(argc-1, argv+1);
}