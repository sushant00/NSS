//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"
#include "enc_utils.c"

int fsign(int argc, unsigned char **args){
	printf("\n");	printf("\n");

	printf("fsign: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	unsigned char *ptr = strrchr(args[0], '/');
	*ptr = 0;

	if( validatePath(args[0])==-1) {
		printf("fsign: validation of file path %s failed\n", args[0]);
		return -1;
	}

	*ptr = '/';
	if ( access(args[0], F_OK) == 0 ) {
		printf("fsign: %s file exists\n", args[0]);
		//authenticate write access for file, as file is already created
		if(authPerm(args[0], 2) == -1){
			printf("fsign: permission denied for file %s\n", args[0]);
			return -1;
		}
	}else{
		printf("fsign: %s file does not exist. Could not sign\n", args[0]);
		return 0;
	}


	//authenticated write access
	printf("fsign: authenticated for write access %s\n", args[0]);

	unsigned char *content = malloc(sizeof(unsigned char)*(MAX_FILE_LEN + KEY_LEN_BITS ));
	unsigned char *md = malloc(sizeof(unsigned char)*EVP_MAX_MD_SIZE);
	
	getOwnerInfo(args[0]);

	FILE *fp;
	unsigned char c;
	int index = 0;
	fp = fopen(args[0], "r");

	//read the file content till end of file
	while((c = (unsigned char)fgetc(fp)) != (unsigned char)EOF) {
		content[index++] = c;
	}

	int ret = calculateHMAC(content, index, md, owner_uid);
	if(ret<0){
		printf("fsign: error in HMAC calc\n");
	}
	printf("fsign: calculated HMAC len=%d, md=%s\n", ret, md);
	fclose(fp);

	printf("fsign: creating HMAC filename\n");
	//write the hmac to file.sign
	unsigned char *hmacFileName = malloc(MAX_FILENAME_LEN);
	strcpy(hmacFileName, args[0]);
	strcpy(hmacFileName+strlen(args[0]), ".sign");
	printf("fsign: HMAC filename %s\n", hmacFileName);

	fp = fopen(hmacFileName, "w+");

	for(int i=0; i<ret; i++ ){
		fputc(md[i], fp);
	}
	fclose(fp);

	if (seteuid(getuid())==-1){
		printf("fsign: error setting euid\n");
	}
	printf("fsign: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	printf("\n");	printf("\n");
	return 0;
}

int main(int argc, unsigned char **argv){
	setbuf(stdout, NULL);
	return fsign(argc-1, argv+1);
}