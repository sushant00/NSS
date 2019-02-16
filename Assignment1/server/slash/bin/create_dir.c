//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "acl_utils.c"

int create_dir(int argc, char **args){
	printf("create_dir: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	char *ptr = strrchr(args[0], '/');
	*ptr = 0;

	if( validatePath(args[0])==-1) {
		printf("create_dir: validation of file path %s failed\n", args[0]);
		return -1;
	}

	*ptr = '/';
	if ( access(args[0], F_OK) == 0 ) {
		printf("create_dir: %s file already created\n", args[0]);
		//authenticate write access for file, as file is already created
		return -1;
	} else {
		//authenticate write access to directory as we are about to create a file in it
		*ptr = 0;
		if(authPerm(args[0], 2) == -1){
			printf("create_dir: permission denied %s\n", args[0]);
			return -1;
		}

		*ptr = '/';
	}


	//authenticated write access
	printf("create_dir: authenticated for write access %s\n", args[0]);

	if(mkdir(args[0], 0777) == -1){
		printf("create_dir: %s\n", strerror(errno));
		return -1;
	}

	if ( inheritAcl(args[0])==-1 ){
		printf("fput: error inheriting acls from parent for %s\n", args[0]);
		return -1;
	}

	printf("create_dir: successfuly created %s\n", args[0]);

	if (seteuid(getuid())==-1){
		printf("create_dir: error setting euid\n");
	}
	printf("create_dir: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	
	return 0;
}


int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return create_dir(argc-1, argv+1);
}