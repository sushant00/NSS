#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/xattr.h>

int main(int argc, char **argv){
	printf("hello world\n");
	for(int i = 0; i<argc; i++) {
		printf("my arg %d %s\n", i, argv[i]);
	}
}