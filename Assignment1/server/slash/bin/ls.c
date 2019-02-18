//Sushant Kumar Singh
//2016103

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "acl_utils.c"



int ls(int argc, char **args){
	printf("ls: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());
	if( validatePath(args[0])==-1) {
		printf("ls: validation of file path %s failed\n", args[0]);
		return -1;
	}
	if( authPerm(args[0], 4)==-1 ) {
		printf("ls: authentication for file path %s failed\n", args[0]);
		return -1;
	}	
	//authenticated read access
	printf("ls: authenticated for read access %s\n\n", args[0]);



	DIR *d;
	struct dirent *dir;
	d = opendir(args[0]);

	char *path = malloc(MAX_PWD_LEN);
	strcpy(path, args[0]);
	strcpy(strchr(path,0), "/");

	if (d) {
		while (( dir = readdir(d)) != NULL ) {
			// printf("ls: file entry %s\n", dir->d_name);
			if ( strcmp(dir->d_name, ".")==0 || strcmp(dir->d_name, "..")==0 ) {
				// printf("ls: skipping %s\n", dir->d_name);
				continue;
			}
			strcpy(strrchr(path, '/')+1, dir->d_name);
			struct stat sb;
			if (stat(path, &sb) == -1) {
				perror("ls: stat");
				return -1;
			}

			struct group *grp;
			struct passwd *pw;

			pw = getpwuid(sb.st_uid);
			if(pw == NULL) {
				perror("ls: error finding owner");
				return -1;
			}
			grp = getgrgid(sb.st_gid);
			if(grp == NULL) {
				perror("ls: error finding owner grp");
				return -1;
			}

			char fileperms[10];
			if ( dir->d_type == DT_DIR ) {
				fileperms[0] = 'd';
			}else{
				fileperms[0] = ' ';
			}
			fileperms[1] = (sb.st_mode & S_IRUSR) ? 'r' : '-';
			fileperms[2] = (sb.st_mode & S_IWUSR) ? 'w' : '-';
			fileperms[3] = (sb.st_mode & S_IXUSR) ? 'x' : '-';
			fileperms[4] = (sb.st_mode & S_IRGRP) ? 'r' : '-';
			fileperms[5] = (sb.st_mode & S_IWGRP) ? 'w' : '-';
			fileperms[6] = (sb.st_mode & S_IXGRP) ? 'x' : '-';
			fileperms[7] = (sb.st_mode & S_IROTH) ? 'r' : '-';
			fileperms[8] = (sb.st_mode & S_IWOTH) ? 'w' : '-';
			fileperms[9] = (sb.st_mode & S_IXOTH) ? 'x' : '-';

			char *modif_time = ctime(&(sb.st_mtime));
			*strchr(modif_time, '\n') = 0;
			printf("%10s %10s %10s %10ld %10s %10s\n", fileperms, pw->pw_name, grp->gr_name, sb.st_size, modif_time, dir->d_name);



		}
		
		printf("\n");
		if (seteuid(getuid())==-1){
			printf("ls: error setting euid\n");
		}
		printf("ls: uid:%u euid:%u gid:%u egid:%u \n", getuid(), geteuid(), getgid(), getegid());

		closedir(d);
	} else{
		perror("ls: could not open the directory");
		return -1;
	}			
	return 0;

}

int main(int argc, char **argv){
	setbuf(stdout, NULL);
	return ls(argc-1, argv+1);
}