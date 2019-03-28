//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"
#include "enc_utils.c"



int fverify(int argc, char **args){
	printf("\n");	printf("\n");


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

	//read the hmac from the file.sign
	unsigned char *hmacFileName = malloc(MAX_FILENAME_LEN);
	strcpy(hmacFileName, args[0]);
	strcpy(hmacFileName+strlen(args[0]), ".sign");
	printf("fverify: HMAC filename %s\n", hmacFileName);

	if ( access(hmacFileName, F_OK) == 0 ) {
		printf("fverify: found %s file. verifying hmac\n", hmacFileName);
	}else{
		printf("fverify: hmac file %s not found. Skipping verification\n", hmacFileName);
		return 0;
	}


	unsigned char *content = malloc(sizeof(unsigned char)*(MAX_FILE_LEN + KEY_LEN_BITS ));
	unsigned char *md = malloc(sizeof(unsigned char)*EVP_MAX_MD_SIZE);
	unsigned char *oldmd = malloc(sizeof(unsigned char)*EVP_MAX_MD_SIZE);

	//read hmac file
	FILE *fp;
	unsigned char c;
	int index = 0;
	fp = fopen(hmacFileName, "r");

	//read the file content of hmac file till end of file
	while((c = (unsigned char)fgetc(fp)) != (unsigned char)EOF) {
		oldmd[index++] = c;
	}
	fclose(fp);
	printf("fverify: stored hmac len=%d, md=%s\n", index, oldmd);


	index = 0;
	fp = fopen(args[0], "r");
	//read the file content till end of file
	while((c = (unsigned char)fgetc(fp)) != (unsigned char)EOF) {
		content[index++] = c;
	}

	getOwnerInfo(args[0]);
	int ret = calculateHMAC(content, index, md, owner_uid);
	if(ret<0){
		printf("fverify: error in HMAC calc\n");
	}
	printf("fverify: calculated HMAC len=%d, md=%s\n", ret, md);
	fclose(fp);

	printf("\n\n");
	if(strcmp(md, oldmd)==0){
		printf("HMAC Validation SUCCESS!\n");
	}else{
		printf("HMAC Validation FAILURE!\n");
	}

	printf("\n\n");
	if (seteuid(getuid())==-1){
		printf("fverify: error setting euid\n");
	}
	printf("fverify: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	printf("\n");	printf("\n");

	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return fverify(argc-1, argv+1);
}