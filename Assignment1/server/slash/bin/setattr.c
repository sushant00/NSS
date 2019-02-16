#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "acl_utils.c"

#include <sys/types.h>
#include <pwd.h>
#include <sys/xattr.h>

int main(int argc, char **argv){
	// char *inp[2] = {"how", "are"};
	printf("here %s %s %s\n", argv[1], argv[2], argv[3]);
	// struct file_acl_data acl = {inp , 2};
	size_t size = sizeof(argv[2]);
	int a = setxattr(argv[3], argv[1], (void *)argv[2], size, XATTR_CREATE);
	// int a = setxattr("/home/sushant/Desktop/nss/Assignment1/slash/home/my", "user.myacl", "hello world \n", size, XATTR_CREATE);
	printf("%d", a);
	perror("erro");
}