#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"



int setacl(int argc, char **args){
	int delete;
	printf("setacl: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	if (argc >= 3){
		if ( strncmp(args[0], "-m", 2) == 0 ) {
			delete = 0;

		} else if( strncmp(args[0], "-x", 2) == 0 ){
			delete = 1;

		} else{
			printf("setacl: wrong arguments '%s'\n", args[0]);
			return -1;
		}

		if( validateAclEntry(args[1], delete ^ 1)==-1 ) {
			printf("setacl: validation of acl entry %s failed, wrong format\n", args[1]);
			return -1;
		}
		if( validatePath(args[2])==-1) {
			printf("setacl: validation of file path %s failed\n", args[2]);
			return -1;
		}
		if( auth(args[2])==-1) {
			printf("setacl: authentication for file path %s failed\n", args[2]);
			return -1;
		}

		if (seteuid(getuid())==-1){
			printf("setacl: error setting euid\n");
		}
		printf("setacl: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());


		char *key = malloc(strlen(ACL_ATTRIB_NAME) + MAX_ACL_LEN);
		strcpy(key, ACL_ATTRIB_NAME);
		strcpy(strchr(key, 0), ":");
		strcpy(strchr(key, 0), type);
		strcpy(strchr(key, 0), ":");
		// if(strlen(name) == 0) {
		// 	printf("setacl: acl entry for owner %s\n");
		// 	name = ;
		// }
		strcpy(strchr(key, 0), name);

		if(delete == 1) {
			printf("setacl: deleting acl entry\n");
			int status = removexattr(args[2], key);
			if (status == -1){
				perror("setacl: error deleting acl");
				return -1;
			}
		}else {
			//first delete old acl if present
			int status = removexattr(args[2], key);

			status = setxattr(args[2], key, (void *)perms, strlen(perms)+1, XATTR_CREATE);
			if (status == -1){
				perror("setacl: error setting acl");
				return -1;
			}		
		}

	} else {
		printf("setacl: supply arguments, usage: setacl <-m|-x> <acl entry> <filename>\n");
		return -1;
	}
	printf("setacl: success\n");
	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	setacl(argc-1, argv+1);
	// size_t size = 100;
	// int a = setxattr("/home/sushant/Desktop/my", "user.myacl", "hello world \n", size, XATTR_CREATE);
	// printf("%d", a);
	// perror("erro");
}