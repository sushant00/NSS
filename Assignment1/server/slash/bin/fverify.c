//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"
#include "enc_utils.c"



int fverify(int argc, char **args){
	printf("fverify: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	if( validatePath(args[0])==-1) {
		printf("fverify: validation of file path %s failed\n", args[0]);
		return -1;
	}
	if( authPerm(args[0], 4)==-1 ) {
		printf("fverify: permission denied file path %s failed\n", args[0]);
		return -1;
	}

	//authenticated read access
	printf("fverify: authenticated for read access %s\n", args[0]);

	//print the file content
	FILE* filePtr = fopen(args[0], "r");
	if(filePtr==NULL){
		printf("fverify: some error opening %s\n", args[0]);
		fclose(filePtr);
		return -1;
	}

	size_t linelen = MAX_LINE_LEN*sizeof(char);
	char *line = malloc(linelen);
	while(getline(&line, &linelen, filePtr)!=-1){  //read until end of file
		printf("%s", line);	
	}
	
	printf("\n");
	if (seteuid(getuid())==-1){
		printf("fverify: error setting euid\n");
	}
	printf("fverify: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return fverify(argc-1, argv+1);
}