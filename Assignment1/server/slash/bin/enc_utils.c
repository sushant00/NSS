//Sushant Kumar Singh
//2016103
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <openssl/evp.h>

#define MAX_FILE_LEN 4096
#define KEY_LEN 256
#define KEY_HMAC_ITER 500
#define IV_HMAC_ITER 400

unsigned char *getKeyUser(int uid);

unsigned char *getKeyUser(int uid){
	char *pass = "helloworld";
	unsigned char *key = malloc(KEY_LEN/32);
	int pass_len = strlen(pass);

	printf("getKeyUser: called for uid %d\n", uid);
	int status = PKCS5_PBKDF2_HMAC(pass, pass_len, NULL, 0, KEY_HMAC_ITER, EVP_sha256(), KEY_LEN, key);
	if(!status){
		perror("getKeyUser: error generating key");
	}
	printf("getKeyUser: key generated successfully %s\n", key);
}
