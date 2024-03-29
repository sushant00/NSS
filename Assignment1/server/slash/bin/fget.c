//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"



int fget(int argc, char **args){
	printf("fget: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	if( validatePath(args[0])==-1) {
		printf("fget: validation of file path %s failed\n", args[0]);
		return -1;
	}
	if( authPerm(args[0], 4)==-1 ) {
		printf("fget: permission denied file path %s failed\n", args[0]);
		return -1;
	}

	//authenticated read access
	printf("fget: authenticated for read access %s\n", args[0]);
	
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
	
	//print the file content
	FILE* filePtr = fopen(args[0], "r");
	if(filePtr==NULL){
		printf("fget: some error opening %s\n", args[0]);
		fclose(filePtr);
		return -1;
	}
	getc(filePtr);
	size_t linelen = MAX_LINE_LEN*sizeof(char);
	char *line = malloc(linelen);
	while(getline(&line, &linelen, filePtr)!=-1){  //read until end of file
		printf("%s", line);	
	}
	
	printf("\n");
	if (seteuid(getuid())==-1){
		printf("fget: error setting euid\n");
	}
	printf("fget: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return fget(argc-1, argv+1);
}