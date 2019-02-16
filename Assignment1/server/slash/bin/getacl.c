//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"

int getacl(int argc, char **args){
	printf("getacl: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	if( validatePath(args[0])==-1) {
		printf("getacl: validation of file path %s failed\n", args[0]);
		return -1;
	}
	if( auth(args[0])==-1) {
		printf("getacl: authentication for file path %s failed\n", args[0]);
		return -1;
	}	

	printf("\n");
	printf("# file %s\n", args[0]);
	printf("# owner %s\n", owner_uname);
	printf("# group %s\n", owner_gname);

	//referred to man listxattr(2)
	ssize_t buflen, keylen, vallen;
	char *buf, *key, *val;

	buflen = listxattr(args[0], NULL, 0);
	if (buflen == -1){
		perror("getacl: listxattr");
		return -1;
	} else if (buflen == 0) {
		printf("getacl: no acl entries\n");
		return -1;
	}

	buf = malloc(buflen);
	buflen = listxattr(args[0], buf, buflen);
	if (buflen == -1){
		perror("getacl: error listing acl");
		return -1;
	}

	key = buf;
	printf("\n");
	while (buflen > 0) {
		// printf("getacl: key %s\n", key);
		if(strncmp(key, ACL_ATTRIB_NAME, strlen(ACL_ATTRIB_NAME)) == 0) {
			vallen = getxattr(args[0], key, NULL, 0);
			if(vallen == -1) {
				perror("getacl: error getting acl");
				return -1;
			}

			val = malloc(vallen+1);
			vallen = getxattr(args[0], key, val, vallen);
			if(vallen == -1) {
				perror("getacl: error getting acl");
				return -1;
			}

			printf("%s:%s\n", strchr(key, ':')+1, val);
		}
		keylen = strlen(key) +1;
		buflen -= keylen;
		key = strchr(key, 0) + 1;
	}


	if (seteuid(getuid())==-1){
		printf("getacl: error setting euid\n");
	}
	printf("getacl: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	

	// char *val;
	// size_t 
	// val = malloc(vallen+1); //one extra byte for NULL terminater
	// printf("getacl: attribute size %ld\n", vallen);
	// ssize_t size = getxattr(args[0], ACL_ATTRIB_NAME, val, vallen);
	// if(size==-1) {
	// 	printf("getacl: error errno %d\n", errno);
	// 	perror("getacl: error geting acls");
	// 	return -1;
	// }
	// printf("getacl: getting acl successful\n");

	// struct file_acl_data *sacl = (struct file_acl_data *)val;
	// printf("getacl: acl length %u\n", sacl->acl_len);
	// printf("\n");
	// for(int i = 0; i < sacl->acl_len; i++){
	// 	printf("%s\n", sacl->acl[i]);
	// }
	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return getacl(argc-1, argv+1);
}