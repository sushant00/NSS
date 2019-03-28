//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"
#include "enc_utils.c"

int putEncryptedContent(unsigned char *path, unsigned char *ciphertext, int cipherLen);
int getOldContent(unsigned char *path, unsigned char *oldContent);
int getNewContent(unsigned char *newContent);

int fput_encrypt(int argc, unsigned char **args){
	printf("fput_encrypt: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	unsigned char *ptr = strrchr(args[0], '/');
	*ptr = 0;

	if( validatePath(args[0])==-1) {
		printf("fput_encrypt: validation of file path %s failed\n", args[0]);
		return -1;
	}

	*ptr = '/';
	if ( access(args[0], F_OK) == 0 ) {
		printf("fput_encrypt: %s file already created. Overwriting it\n", args[0]);
		//authenticate write access for file, as file is already created
		if(authPerm(args[0], 2) == -1){
			printf("fput_encrypt: permission denied for file %s\n", args[0]);
			return -1;
		}

		// //create the file (and overwrite old)
		// FILE* file = fopen(args[0], "w+");
		// fclose(file);
		// chown(args[0], getuid(), getgid());
		// if ( inheritAcl(args[0])==-1 ){
		// 	printf("fput_encrypt: error inheriting acls from parent for %s\n", args[0]);
		// 	return -1;
		// }
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

	//write the content after encrypting

	unsigned char *plaintext = malloc(sizeof(unsigned char)*MAX_FILE_LEN);
	unsigned char *ciphertext = malloc(sizeof(unsigned char)*(MAX_FILE_LEN + strlen(key) ));
	
	int oldLen = 0;
	oldLen = getOldContent(args[0], (unsigned char *)plaintext);

	printf("fput_encrypt: enter the content to put. Type '%s' to finish writing.\n", FILE_END);

	getNewContent(plaintext + oldLen);
	//encrypt the content
	int cipherLen = encryptContent(plaintext, ciphertext, 1);
	if(cipherLen<0){
		printf("fput_encrypt: error decrypting old content\n");
		return -1;
	}
	putEncryptedContent(args[0], ciphertext, cipherLen);

	if (seteuid(getuid())==-1){
		printf("fput_encrypt: error setting euid\n");
	}
	printf("fput_encrypt: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	
	return 0;
}


int putEncryptedContent(unsigned char *path, unsigned char *ciphertext, int cipherLen) {
	// printf("putEncryptedContent: to %s\n", path);
	FILE* filePtr = fopen(path, "a");
	if(filePtr==NULL) {
		printf("putEncryptedContent: some error opening %s for append\n", path);
		return -1;
	}

	for(int i=0; i<cipherLen; i++ ){
		fprintf(filePtr, "%c", ciphertext[i]);
	}

	fclose(filePtr);
	printf("putEncryptedContent: finished putting encrypted content\n");
	return 0;
}

int getOldContent(unsigned char *path, unsigned char *oldContent){
	int encrypted;
	int oldLen = 0;

	FILE *fp;
	int c;
	int index = 0;
	fp = fopen(path, "r");
	c = fget(fp);
	if (c=='E') {
		//this is encrypted content, decrypt it first
		encrypted = 1;
	}else{
		encrypted = 0;
	}

	//read the file content till end of file
	while((c = fgetc(fp) != EOF)) {
		oldContent[index++] = c;
		oldLen+=1;
	}

	// if encrypted, decrypt it first
	if( encrypted ){
		unsigned char *decryptedContent = malloc(sizeof(unsigned char)*(MAX_FILE_LEN + KEY_LEN_BITS ));
		int len_decryptedContent = encryptContent(oldContent, decryptedContent, 0);
		if(len_decryptedContent<0){
			printf("getOldContent: error decrypting old content\n");
			return -1;
		}
		oldLen = len_decryptedContent;
		strncpy(oldContent, decryptedContent, len_decryptedContent);
		
	}
	return oldLen;
}

int getNewContent(unsigned char *newContent) {
	printf("getNewContent: called\n");

	size_t inputlen = MAX_LINE_LEN*sizeof(unsigned char);
	unsigned char *input = malloc(inputlen);
				
	//take line input
	while(1){
		fgets(input, inputlen, stdin);
		unsigned char *newline = strrchr(input, '\n');
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

int main(int argc, unsigned char **argv){
	setbuf(stdout, NULL);
	return fput_encrypt(argc-1, argv+1);
}