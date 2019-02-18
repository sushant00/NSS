//Sushant Kumar Singh
//2016103

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "filesystem_utils.c"

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/xattr.h>

#define ACL_ATTRIB_NAME "user.myacl"

#define MAX_TOKENS 4
#define ACL_TOKENIZER ':'

#define MAX_ACL_LEN 100
#define MAX_TYPE_LEN 1
#define MAX_NAME_LEN 50
#define PERM_LEN 3
#define NUM_TYPES 4

struct file_acl_data
{
	char **acl;
	unsigned int acl_len;
};

unsigned int owner_uid;
unsigned int owner_gid;

char owner_uname[MAX_NAME_LEN];
char owner_gname[MAX_NAME_LEN];

char type[MAX_TYPE_LEN];
char name[MAX_NAME_LEN];
char perms[PERM_LEN];
unsigned int perm;

const char *types[NUM_TYPES] = {"m", "g", "u", "o"};

int validateAclEntry(char * aclentry, int withPerms);
int auth(char *path); 
int getOwnerInfo(char *path);
int inheritAcl(char *path);
int authDACPerm(char *path, unsigned int reqd_perm);
int authPerm(char *path, unsigned int reqd_perm);

int validateAclEntry(char * aclline, int withPerms) {
	perm = 0;
	char *aclentry = malloc(MAX_ACL_LEN);
	strcpy(aclentry, aclline);
	printf("validateAclEntry: %s\n", aclentry);
	//
	printf("validateAclEntry: validate acl type\n");	
	char *tokenEnd;
	tokenEnd = strchr(aclentry, ACL_TOKENIZER);
	if(tokenEnd == NULL) {
		printf("validateAclEntry: wrong format of acl entry\n");
		return -1;
	}
	*tokenEnd = 0;
	int found = 0;
	for( int i = 0; i<NUM_TYPES; i++) {
		if( strcmp(aclentry, types[i])==0 ){
			strcpy(type, aclentry);
			found = 1;
		}
	}
	if(found == 0) {
		printf("validateAclEntry: wrong acl type %s\n", aclentry);
	}



	//
	printf("validateAclEntry: validate username\n");
	aclentry = tokenEnd+1;
	tokenEnd = strchr(aclentry, ACL_TOKENIZER);
	if(tokenEnd == NULL) {
		printf("validateAclEntry: wrong format of acl entry\n");
		return -1;
	}
	*tokenEnd = 0;

	//TODO: Add a check to validate the usernames
	struct passwd spass;
	if(strlen(aclentry)==0 || getpwnam(aclentry) != NULL) {
		strcpy(name, aclentry);
	}else{
		printf("validateAclEntry: user %s does not exist\n", aclentry);
		return -1;
	}



	//
	printf("validateAclEntry: validate permissions\n");
	aclentry = tokenEnd+1;
	if (strlen(aclentry)!=PERM_LEN) {
		printf("validateAclEntry: wrong perm format \n");
		return -1;
	}

	if(withPerms){
		if( aclentry[0] == 'r'){
			perm = perm ^ 4;
		}else{
			if (aclentry[0]!='-') {
				printf("validateAclEntry: wrong perm format\n");
			}
		}
		if( aclentry[1] == 'w'){
			perm = perm ^ 2;
		}else{
			if (aclentry[1]!='-') {
				printf("validateAclEntry: wrong perm format\n");
			}
		}
		if( aclentry[2] == 'x'){
			perm = perm ^ 1;
		}else{
			if (aclentry[2]!='-') {
				printf("validateAclEntry: wrong perm format\n");
			}
		}
		strcpy(perms, aclentry);
	}

	return 0;
}

int auth(char *path) {
	printf("auth: current user id %d\n", getuid());
	//check if root
	if(getuid() == ROOT_PID) {
		owner_uid = 0;
		owner_gid = 0;
		return 0;
	}
	getOwnerInfo(path);
	printf("auth: file %s has owner UID %ld\n", path, (long)owner_uid);

	if(getuid()==owner_uid) {
		printf("auth: authenticated\n");
		return 0;
	}
	return -1;
}

int getOwnerInfo(char *path) {
	struct stat sb;
	if (stat(path, &sb) == -1) {
		perror("auth: stat");
		return -1;
	}

	owner_uid = sb.st_uid;
	owner_gid = sb.st_uid;

	struct group *grp_s;
	struct passwd *pw_s;

	pw_s = getpwuid(owner_uid);
	if(pw_s == NULL) {
		perror("getOwnerInfo: error finding owner");
		return -1;
	}
	grp_s = getgrgid(owner_gid);
	if(grp_s == NULL) {
		perror("getOwnerInfo: error finding owner grp");
		return -1;
	}

	// owner_uname = malloc(MAX_NAME_LEN);
	// owner_gname = malloc(MAX_NAME_LEN);
	
	strcpy(owner_uname, pw_s->pw_name);
	strcpy(owner_gname, grp_s->gr_name);


	printf("getOwnerInfo: owner_uname: %s\n", owner_uname);
	printf("getOwnerInfo: owner_gname: %s\n", owner_gname);

	return 0;
}

int authDACPerm(char *path, unsigned int reqd_perm) {
	struct stat sb;
	if (stat(path, &sb) == -1) {
		perror("authDACPerm: stat");
		return -1;
	}
	unsigned int has_perm = 0;
	if(getuid() == sb.st_uid) {
		if(S_IRUSR & sb.st_mode) {
			has_perm = has_perm ^ 4;
		}
		if(S_IWUSR & sb.st_mode) {
			has_perm = has_perm ^ 2;
		}
		if(S_IXUSR & sb.st_mode) {
			has_perm = has_perm ^ 1;
		}
	}else {
		if(S_IROTH & sb.st_mode) {
			has_perm = has_perm ^ 4;
		}
		if(S_IWOTH & sb.st_mode) {
			has_perm = has_perm ^ 2;
		}
		if(S_IXOTH & sb.st_mode) {
			has_perm = has_perm ^ 1;
		}
	}

	if ((reqd_perm & has_perm) >= reqd_perm) {
		printf("authDACPerm: authenticated\n");
		return 0;
	}
	printf("authDACPerm: could not authenticate. reqd perms %u has perms %u\n", reqd_perm, has_perm);
	return -1;
}


int authPerm(char *path, unsigned int reqd_perm) {
	//check if root
	printf("authPerm: %s perm %u\n", path, reqd_perm);
	if(getuid() == ROOT_PID) {
		owner_uid = 0;
		owner_gid = 0;
		return 0;
	}

	unsigned int has_perm = 0;


	getOwnerInfo(path);

	struct group *grp;
	struct passwd *pw;


	pw = getpwuid(getuid());
	if(pw == NULL) {
		perror("authPerm: error finding owner");
		return -1;
	}
	grp = getgrgid(getgid());
	if(grp == NULL) {
		perror("authPerm: error finding owner grp");
		return -1;
	}
	printf("authPerm: uid %u, current user %s, owner %s\n", getuid(), pw->pw_name, owner_uname);

	//referred to man listxattr(2)
	ssize_t buflen, keylen, vallen;
	char *buf, *key, *val;

	buflen = listxattr(path, NULL, 0);
	if (buflen == -1){
		perror("authPerm: listxattr");
		return -1;
	} else if (buflen == 0) {
		printf("authPerm: no acl entries\n");
		return authDACPerm(path, reqd_perm);
	}

	buf = malloc(buflen);
	buflen = listxattr(path, buf, buflen);
	if (buflen == -1){
		perror("authPerm: error listing acl");
		return -1;
	}

	unsigned int mask_perm = 7;

	key = buf;
	printf("\n");
	while (buflen > 0) {
		printf("authPerm: key %s\n", key);
		if(strncmp(key, ACL_ATTRIB_NAME, strlen(ACL_ATTRIB_NAME)) == 0) {
			vallen = getxattr(path, key, NULL, 0);
			if(vallen == -1) {
				perror("authPerm: error getting acl");
				return -1;
			}

			val = malloc(vallen+1);
			vallen = getxattr(path, key, val, vallen);
			if(vallen == -1) {
				perror("authPerm: error getting acl");
				return -1;
			}
			char *key_type = strchr(key, ':')+1;
			char *aclentry = malloc(vallen + strlen(key));
			strcpy(aclentry, key_type);
			strcpy(strchr(aclentry, 0), ":");
			strcpy(strchr(aclentry, 0), val);

			validateAclEntry(aclentry, 1);

			if( strcmp(type, "u") == 0 ) {
				printf("authPerm: %s %s\n", name, pw->pw_name);
				if( strlen(name) == 0 ) {
					printf("authPerm: owner entry owner: %s current user: %s\n", owner_uname, pw->pw_name);
					if ( strcmp(owner_uname, pw->pw_name)==0 ) {
						has_perm = has_perm | perm;
					}
				}
				if ( strcmp(name, pw->pw_name)==0 ) {
					printf("authPerm: named user entry %s\n", name);
					has_perm = has_perm | perm;
					break;
				}

			} else if ( strcmp(type, "g") == 0 ) {
				if( strlen(name) == 0 ) {
					if ( strcmp(owner_gname, grp->gr_name)==0 ) {
						has_perm = has_perm | perm;
					}
				}
				if ( strcmp(name, grp->gr_name)==0 ) {
					has_perm = has_perm | perm;
					break;
				}

			} else if ( strcmp(type, "o") == 0 ) {
				has_perm = has_perm | perm;
			} else if ( strcmp(type, "m") == 0 ) {
				mask_perm = mask_perm & perm;
			}
			printf("authPerm: has perms %u from %s\n", has_perm, key);
		}
		keylen = strlen(key) +1;
		buflen -= keylen;
		key = strchr(key, 0) + 1;
	}
	if(auth(path)==-1) {
		has_perm = (has_perm & mask_perm);
	}

	if ((reqd_perm & has_perm) >= reqd_perm) {
		printf("authPerm: authenticated\n");
		return 0;
	}
	// printf("%d %d\n", reqd_perm & has_perm, reqd_perm);
	printf("authPerm: could not authenticate. reqd perms %u has perms %u\n", reqd_perm, has_perm);
	return -1;

}



int inheritAcl(char *path) {	
	
	//inherit acl from parent dir
	printf("inheritAcl: %s\n", path);
	char parentDir[ strlen(PATH_HOME) + MAX_PWD_LEN ];
	strcpy(parentDir, path);
	*strrchr(parentDir, '/') = 0;

	printf("inheritAcl: parent dir %s\n", parentDir);

	int status;
	ssize_t vallen;
	char *key, *val;

	key = malloc(strlen(ACL_ATTRIB_NAME) + MAX_ACL_LEN);
	strcpy(key, ACL_ATTRIB_NAME);
	char *typeStart = strchr(key, 0);

	strcpy(typeStart, ":u:");
	vallen = getxattr(parentDir, key, NULL, 0);
	val = malloc(vallen+1);
	vallen = getxattr(parentDir, key, val, vallen);
	if(vallen == -1) {
		perror("inheritAcl: error getting acl from parent");
		return -1;
	}
	status = setxattr(path, key, (void *)val, vallen, XATTR_CREATE);
	if (status == -1){
		perror("inheritAcl: error setting acl");
		return -1;
	}	

	strcpy(typeStart, ":g:");
	vallen = getxattr(parentDir, key, NULL, 0);
	val = malloc(vallen+1);
	vallen = getxattr(parentDir, key, val, vallen);
	if(vallen == -1) {
		perror("inheritAcl: error getting acl from parent");
		return -1;
	}
	status = setxattr(path, key, (void *)val, vallen, XATTR_CREATE);
	if (status == -1){
		perror("inheritAcl: error setting acl");
		return -1;
	}

	strcpy(typeStart, ":o:");
	vallen = getxattr(parentDir, key, NULL, 0);
	val = malloc(vallen+1);
	vallen = getxattr(parentDir, key, val, vallen);
	if(vallen == -1) {
		perror("inheritAcl: error getting acl from parent");
		return -1;
	}
	status = setxattr(path, key, (void *)val, vallen, XATTR_CREATE);
	if (status == -1){
		perror("inheritAcl: error setting acl");
		return -1;
	}


	strcpy(typeStart, ":m:");
	vallen = getxattr(parentDir, key, NULL, 0);
	val = malloc(vallen+1);
	vallen = getxattr(parentDir, key, val, vallen);
	if(vallen != -1){
		status = setxattr(path, key, (void *)val, vallen, XATTR_CREATE);
		if (status == -1){
			perror("inheritAcl: error setting acl");
			return -1;
		}
	}
	return 0;
}