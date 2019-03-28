//Sushant Kumar Singh
//2016103
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pwd.h>
#include<sys/wait.h> 

#include <openssl/evp.h>
#include <openssl/hmac.h>

#define MAX_SHADOWFILE_LINE_LEN 1024
#define MAX_FILE_LEN 4096
#define KEY_LEN_BITS 256
#define HMAC_ITER 200
// #define IV_HMAC_ITER 400
#define PASSWD_FILE "/etc/shadow"
#define MAX_PASSWD_LEN 512

// unsigned char *getIVUser(int uid);
int getPassword(int uid, unsigned char *buff);
int getKeyIVUser(int uid, unsigned char *key, unsigned char *iv);
int cipher(unsigned char *input, int len_input, unsigned char *output, int doEnc, int uid);
int calculateHMAC(unsigned char *d, int len_d, unsigned char *md, int uid);

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

	size_t linelen = MAX_SHADOWFILE_LINE_LEN*sizeof(unsigned char);
	char *line = malloc(linelen);
	char *unameEnd;
	char *passwdEnd;
	while(getline(&line, &linelen, fp)!=-1){  //read until end of file
		unameEnd = strchr(line, ':');
		*unameEnd = 0;
		if (strcmp(pw_s->pw_name, line) == 0){
			// printf("getPassword: found user match, uname %s\n", line);
			passwdEnd = strchr(unameEnd+1, ':');
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

	// unsigned char *key = malloc(KEY_LEN_BITS/sizeof(unsigned char));
	// unsigned char *iv = malloc(KEY_LEN_BITS/sizeof(unsigned char));
	// int pass_len = ;

	// printf("getKeyIVUser: called for uid %d\n", uid);
	int ret = PKCS5_PBKDF2_HMAC_SHA1(pass, strlen(pass), NULL, 0, HMAC_ITER, KEY_LEN_BITS, key);
	if(!ret){
		perror("getKeyIVUser: error generating key");
		return -1;
	}

	ret = PKCS5_PBKDF2_HMAC_SHA1(key, KEY_LEN_BITS/sizeof(unsigned char), NULL, 0, HMAC_ITER, KEY_LEN_BITS, iv);
	if(!ret){
		perror("getKeyIVUser: error generating iv");
		return -1;
	}

	// printf("getKeyIVUser: key generated successfully %s\n", key);
	return 0;
}


int cipher(unsigned char *input, int len_input, unsigned char *output, int doEnc, int uid) {
	printf("cipher: called doEnc=%d, len_input=%d, input=%s\n", doEnc, len_input, input);
	unsigned char *key = malloc(KEY_LEN_BITS/sizeof(unsigned char));
	unsigned char *iv = malloc(KEY_LEN_BITS/sizeof(unsigned char));
	getKeyIVUser(uid, key, iv);

	int len_output = 0;
	int len;

	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();	
	int ret = EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv, doEnc);
	if(!ret){
		printf("cipher: error evp cipher init\n");
		return -1;
	}

	ret = EVP_CipherUpdate(ctx, output, &len, input, len_input);
	if(!ret){
		perror("cipher: Cipher update failed");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	len_output+=len;

	ret = EVP_CipherFinal_ex(ctx, output+len, &len);
	if(!ret){		
		printf("cipher: Cipher final failed\n");
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	len_output+=len;

	printf("cipher: %d successful. outputlen=%d, output=%s\n", doEnc, len_output, output);

	EVP_CIPHER_CTX_free(ctx);
	return len_output;
}


int calculateHMAC(unsigned char *d, int len_d, unsigned char *md, int uid) {
	printf("calculateHMAC: called for len=%d,d=%s\n", len_d, d);
	unsigned char *key = malloc(KEY_LEN_BITS/sizeof(unsigned char));
	unsigned char *iv = malloc(KEY_LEN_BITS/sizeof(unsigned char));
	getKeyIVUser(uid, key, iv);

	int len_md = 0;
	if( HMAC(EVP_sha1(), key, KEY_LEN_BITS, d, len_d, md, &len_md) == 0){
		perror("calculateHMAC: error");
		return -1;
	}
	printf("calculateHMAC: success. md_len=%d,md=%s\n", len_md, md);
	return len_md;

}
