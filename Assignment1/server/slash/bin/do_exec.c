//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include<sys/wait.h> 

#include "acl_utils.c"



int do_exec(int argc, char **args){
	printf("do_exec: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	if( validatePath(args[0])==-1) {
		printf("do_exec: validation of file path %s failed\n", args[0]);
		return -1;
	}

	int pid = fork();
	if(pid==0){
		printf("do_exec: successfully forked \n");
		printf("\n");
		//successfully forked: child process
		execvp(args[0],args);
		exit(EXIT_SUCCESS);

	}else if(pid==-1){
		//error forking
		fprintf(stderr,"error forking. child process not created");

	}else{
		//successfully forked: parent process
		wait(0);
	}
	
	printf("\n");
	if (seteuid(getuid())==-1){
		printf("do_exec: error setting euid\n");
	}
	printf("do_exec: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

	return 0;
}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return do_exec(argc-1, argv+1);
}