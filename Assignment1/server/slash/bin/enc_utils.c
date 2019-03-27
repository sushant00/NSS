//Sushant Kumar Singh
//2016103
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <openssl/evp.h>

#define MAX_FILE_LEN 4096
#define KEY_LEN 256
#define HMAC_ITER 200
// #define IV_HMAC_ITER 400
#define PASSWD_FILE "/etc/shadow"
#define MAX_PASSWD_LEN 512

// unsigned char *getIVUser(int uid);
int getPassword(int uid, unsigned char *buff);
int getKeyIVUser(int uid, unsigned char *key, unsigned char *iv);

int getPassword(int uid, unsigned char *buff) {
	FILE *fp;	
	fp = fopen(PASSWD_FILE, "r");
	if(fp==NULL){
		printf("getPassword: some error opening %s\n", PASSWD_FILE);
		fclose(fp);
		return -1;
	}

	struct passwd *pw_s;
	pw_s = getpwuid(uid);
	if(pw_s == NULL) {
		perror("getPassword: error finding owner");
		return -1;
	}

	size_t linelen = MAX_LINE_LEN*sizeof(char);
	char *line = malloc(linelen);
	char * unameEnd, passwdEnd;
	while(getline(&line, &linelen, filePtr)!=-1){  //read until end of file
		unameEnd = strchr(line, ":");
		*unameEnd = 0;
		if (strcmp(pw_s->pw_name, line) == 0){
			printf("getPassword: found user match, uname %s\n", line);
			passwdEnd = strchr(unameEnd+1, ":");
			*passwdEnd = 0;
			strcpy(buff, unameEnd+1);
			return 0;
		}
	}
	return -1;
}

int getKeyIVUser(int uid, unsigned char *key, unsigned char *iv){

	unsigned char *pass = malloc(MAX_PASSWD_LEN);
	if(getPassword(uid, pass)!=0){
		printf("getKeyIVUser: error getting passwd\n");
		return -1;
	}

	// unsigned char *key = malloc(KEY_LEN/32);
	// unsigned char *iv = malloc(KEY_LEN/32);
	// int pass_len = ;

	printf("getKeyIVUser: called for uid %d\n", uid);
	int status = PKCS5_PBKDF2_HMAC_SHA1(pass, strlen(pass), NULL, 0, HMAC_ITER, KEY_LEN, key);
	if(!status){
		perror("getKeyIVUser: error generating key");
		return -1;
	}

	int status = PKCS5_PBKDF2_HMAC_SHA1(key, strlen(key), NULL, 0, HMAC_ITER, KEY_LEN, iv);
	if(!status){
		perror("getKeyIVUser: error generating iv");
		return -1;
	}

	printf("getKeyIVUser: key generated successfully %s\n", key);
	return 0;
}
