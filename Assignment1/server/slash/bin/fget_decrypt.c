//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"
#include "enc_utils.c"



int fget_decrypt(int argc, char **args){
	printf("fget_decrypt: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	if( validatePath(args[0])==-1) {
		printf("fget_decrypt: validation of file path %s failed\n", args[0]);
		return -1;
	}
	if( authPerm(args[0], 4)==-1 ) {
		printf("fget_decrypt: permission denied file path %s failed\n", args[0]);
		return -1;
	}

	//authenticated read access
	printf("fget_decrypt: authenticated for read access %s\n", args[0]);
	
	//fverify the HMAC
	if(fork()==0){
		char *argsexec[] = {"slash/bin/fverify", args[0], NULL};
		// printf("fput_encrypt: calling execvp\n");
		int ret = execvp(argsexec[0], argsexec);
		if(ret<0){
			printf("fget_decrypt: HMAC validation failed\n");
			return -1;
		}
	}else{
		wait(0);
	}

	getOwnerInfo(args[0]);

	unsigned char *plaintext = malloc(sizeof(unsigned char)*MAX_FILE_LEN);
	unsigned char *ciphertext = malloc(sizeof(unsigned char)*(MAX_FILE_LEN + KEY_LEN_BITS ));
	
	int encrypted;
	FILE *fp;
	unsigned char c;
	int index = 0;
	fp = fopen(args[0], "r");
	c = (unsigned char)fgetc(fp);
	if(c!='E'){
		printf("fget_decrypt: this is not a encrypted file\n");
		return 0;
	}
	index = 0;
	//read the file content till end of file
	while((c = (unsigned char)fgetc(fp)) != (unsigned char)EOF) {
		ciphertext[index++] = c;
	}

	// decrypt it first
	int len_plaintext = cipher(ciphertext, index, plaintext, 0, owner_uid);
	if(len_plaintext<0){
		printf("fget_decrypt: error decrypting content\n");
		return -1;
	}	
	printf("fget_decrypt: success \n\n");

	printf("%s\n\n", plaintext);

	printf("\n");
	if (seteuid(getuid())==-1){
		printf("fget_decrypt: error setting euid\n");
	}
	printf("fget_decrypt: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return fget_decrypt(argc-1, argv+1);
}