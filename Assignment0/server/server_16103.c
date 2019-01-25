//Sushant Kumar Singh
//2016103

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <semaphore.h>

#define MAX_USERS 20
#define MAX_USERNAME_LEN 50
#define MAX_CLIENTS 10
#define SERVER_PORT 12000
#define EXIT_REQUEST "exit"
#define COMMAND_LEN 1024
#define MSG_LEN 2024

#define PATH_PASSWD "etc/passwd"
#define PATH_HOME "slash/home/"

const char *COMMANDS[6] = {"cd", "create_dir", "exit", "fget", "fput", "ls"};
const char *delim = " \n\r\t";//space, carriage return , newline, tab


void* handleConnection(void *args);
void* informExit(void *args);
int setupFileSystem(void);
int getEmptySpot(void);
int authAndConnect(int connectionSocket);
int initVars(void);

char *readCommand();
char **parseCommand(char * command);
int exec_internal(char **args);

//supported internal commands
int cd(char **args);
int create_dir(char **args);
int exitShell(char **args);
int fget(char **args);
int fput(char **args);
int ls(char **args);


//shell vars
size_t bufsize = 1024;		//assume max line length = 100 chars
char *line;
int MAX_TOKENS = 50;		//assume max tokens = 50
char **tokens;
char *pwd;


//user database
char userNames[MAX_USERS+1][MAX_USERNAME_LEN];
char userGroup[MAX_USERS+1][MAX_USERNAME_LEN];

// Synchronisation vars
int numClients;
int clientSockets[MAX_CLIENTS];
int clientIDs[MAX_CLIENTS];
sem_t w_mutex;					//threads acquire lock before modifying clientSockets, numClients;
int readCount = 0;				//read count for clients reading clientSockets


//struct for args to connection threads
struct Args {	
	int connectionSocket;
	// int clientID;
};

// SERVER
int main(void){

	initVars();

	if ( setupFileSystem()==-1 ){
		fprintf(stderr, "[error] could not setup filesystem\n");
		exit(0);
	}else {
		printf("file system setup done\n");
	}

	printf("starting server...\n");

	int serverSocket, len, ret, enable=1;
	struct sockaddr_in local_addr;


	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(serverSocket==-1){	//error
		fprintf(stderr, "could not create server\n");
		exit(1);
	}
	ret = setsockopt(serverSocket, SOL_SOCKET, SO_KEEPALIVE | SO_REUSEADDR, (const void *)&enable, sizeof(enable));
	if(ret==-1){	//error
		fprintf(stderr, "could not set socket options\n");

	}

	//IPv4 address
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(SERVER_PORT);		//host to network byte(big endian) short
	local_addr.sin_addr.s_addr = INADDR_ANY;		//bind to every local interface

	len = sizeof(local_addr);
	ret = bind(serverSocket, (struct sockaddr *)&local_addr, (socklen_t)len);
	if(ret==-1){	//error
		fprintf(stderr, "could not bind\n");
		exit(1);
	}

	printf("server ready\n");

	ret = listen(serverSocket, MAX_CLIENTS);//ignore if too many clients waiting
	if(ret==-1){	//error
		fprintf(stderr, "could not listen\n");
		exit(1);
	}


	printf("listening...Type %s to close the server anytime\n", EXIT_REQUEST);

	//handles the exit request from server
	pthread_t exit_thread;
	pthread_create(&exit_thread, NULL, informExit, (void *)&serverSocket);


	//wait for clients
	while(1){
		struct sockaddr_in connection_addr;
		len = sizeof(connection_addr);
		int connectionSocket = accept(serverSocket, (struct sockaddr *)&connection_addr, (socklen_t *)&len);
		if(connectionSocket==-1){	//error
			fprintf(stderr, "[error] could not accept connection\n");
			exit(1);
		}
		printf("new connection incoming\n");

		if(numClients==MAX_CLIENTS){
			printf("rejecting client as max connections reached\n");

			size_t sendmsglen = MSG_LEN*sizeof(char);
			char *sendmsg = malloc(sendmsglen);
			sprintf(sendmsg, "server: Too many conncetions. Try later.\n");
			int sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
			close(connectionSocket);
		}else{

			// increase number of clients and save the connectionSocket;
			numClients++;
			printf("a client connected\n");

			struct Args con_args = {connectionSocket};

			//handle the client in new thread
			pthread_t connection_thread;
			pthread_create(&connection_thread, NULL, handleConnection, (void *)&con_args);

			// for(int i=0; i<MAX_CLIENTS; i++){
			// 	if(clientIDs[i] == 0){
			// 		clientSockets[i] = connectionSocket;
			// 		clientIDs[i] = i+1;//TODO: assign unique ids

			// 	}
			// }
		}

	}

	return 0;
}



//reads user database file - passwd, creates home directories for users
int setupFileSystem(void){
	FILE* userDBptr = fopen(PATH_PASSWD, "r");

	if(userDBptr==NULL){
		fprintf(stderr, "[error] opening user database file\n");
		return -1;
	}
	printf("reading user database\n");

	int numUser = 0;
	char dirName[ strlen(PATH_HOME) + MAX_USERNAME_LEN ];
	strcpy(dirName, PATH_HOME);
	
	char buf[MAX_USERNAME_LEN];
	while (numUser<MAX_USERS && fscanf(userDBptr, "%s", buf)==1 ) {
		strcpy(userNames[numUser], buf);

		//now read the group name
		if ( fscanf(userDBptr, "%s", buf)==1 ){
			strcpy(userGroup[numUser], buf);
		}else{
			fprintf(stderr, "[error] wrong user database format\n");
			return -1;
		}
		
		printf("making directory for %s\n", userNames[numUser]);
		strcpy(dirName + strlen(PATH_HOME), userNames[numUser]);
		printf("directory name %s\n", dirName);
		if(mkdir(dirName, 0775) == -1){
			if (errno != EEXIST){
				fprintf(stderr, "[error] while creating user %s home directory, errno %d\n",userNames[numUser], errno);
				return -1;
			}
		}
		numUser++;
	}
	return 0;

}


//check if the user is connected, if connected return index in clientIDs
int userConnected(int userID) {
	int clientInd = 0;
	while( clientInd < MAX_CLIENTS) {
		if( clientIDs[clientInd] == userID ){
			return clientInd;
		}
		clientInd++;
	}
	return -1;
}


int getEmptySpot(void) {
	int clientInd = 0;
	while( clientInd < MAX_CLIENTS ) {
		if(clientIDs[clientInd] == -1) {
			return clientInd;
		}
		clientInd+=1;
	}
	return -1;
}



int authAndConnect(int connectionSocket) {
	int ind;
	printf("authenticating new client\n");

	size_t recvmsglen = MSG_LEN*sizeof(char);
	char *recvmsg = malloc(recvmsglen);
	size_t recvlen, sendlen;

	size_t sendmsglen = MSG_LEN*sizeof(char);
	char *sendmsg = malloc(sendmsglen);

	sprintf(sendmsg, "server: enter username ");
	sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
	if(sendlen==-1){ //error
		printf("[error] sending to socket %d\n", connectionSocket);
		return -1;
	}
	//receive the msg
	printf("waiting for client for username\n");

	recvlen = recv(connectionSocket, (void *)recvmsg, recvmsglen, 0);
	if(recvlen==-1){ //error
		printf("[error] receiving from socket %d\n", connectionSocket);
		return -1;
		
	}else if(recvlen==0){
		printf("client socket %d disconnected abruptly\n", connectionSocket);
		return -1;

	}else{
		printf("client response received\n");
		int userInd = 0;
		while(userInd < MAX_USERS){
			if(strcmp(userNames[userInd], recvmsg)==0) {
				//check if this user is already connected
				if( userConnected(userInd) > 0 ) {
					printf("user %s already present", recvmsg);
					return -1;
				}else {
					ind = getEmptySpot();
					if (ind==-1){
						printf("no empty spot for connections");
						return -1;
					}else{
						printf("user %s connected\n", userNames[userInd]);

						clientIDs[ind] = userInd;
						clientSockets[ind] = connectionSocket;
					
						//informing client
						sprintf(sendmsg, "server: authenticated");
						sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
						if(sendlen==-1){ //error
							printf("[error] sending to socket %d\n", connectionSocket);
							return -1;
						}
						return ind;
					}					
				}
			}
			userInd++;
		}
	}
	return -1;
}


//
int initVars(void) {
	for(int i=0; i<MAX_CLIENTS; i++) {
		clientIDs[i] = -1;
	}
}

//handler for a client
void* handleConnection(void *args){
	//get the arguments
	struct Args *con_args = args;
	int connectionSocket = con_args->connectionSocket;
	// int clientID = con_args->clientID;

	size_t recvmsglen = MSG_LEN*sizeof(char);
	char *recvmsg = malloc(recvmsglen);
	size_t recvlen, sendlen;

	size_t sendmsglen = MSG_LEN*sizeof(char);
	char *sendmsg = malloc(sendmsglen);

	//authenticate the user
	int clientInd = authAndConnect(connectionSocket);
	if ( clientInd == -1 ){
		printf("authentication failed. Closing connection.\n");

		//informing client
		sprintf(sendmsg, "server: authentication failed\n");
		sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
		if(sendlen==-1){ //error
			printf("[error] sending to socket %d\n", connectionSocket);
		}
		close(connectionSocket);
		numClients--;
		pthread_exit(NULL);
	}

	char **args;
	int executed;

	while(1){
		//receive the msg
		recvlen = recv(connectionSocket, (void *)recvmsg, recvmsglen, 0);
		if (recvlen <= 0){
			if(recvlen==-1){ //error
				printf("error receiving from %s\n", userNames[clientIDs[clientInd]]);
			}else if(recvlen==0){
				printf("client %s disconnected abruptly\n", userNames[clientIDs[clientInd]]);
			}
			
			//takeaway client's ID and close socket
			clientIDs[clientInd] = 0;
			close(clientSockets[clientInd]);
			numClients--;
			pthread_exit(NULL);
		}
		else{
			printf("received %zd bytes from client %s: %s\n", recvlen, userNames[clientIDs[clientInd]], recvmsg);
			args = parseCommand(recvmsg);
			//check and execute if internal command
			if (exec_internal(args)==-1) {
				//informing client
				sprintf(sendmsg, "[error] %s is not valid command\n", args[0]);
				sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
				if(sendlen==-1){ //error
					printf("[error] sending to socket %d\n", connectionSocket);
				}
			}
			

			if(strcmp(EXIT_REQUEST, recvmsg)==0){				
				printf("client %s disconnected\n", userNames[clientIDs[clientInd]]);
				//takeaway client's ID and close socket
				clientIDs[clientInd] = 0;
				close(clientSockets[clientInd]);
				numClients--;
				pthread_exit(NULL);
			}

			// //create the message to be transmitted
			// sprintf(sendmsg, "received from %d: %s", clientID, recvmsg);
			// printf("%s\n", sendmsg);

			// sendmsglen = strlen(sendmsg)+1;

			// //broadcast the message
			// for(int i = 0; i<MAX_CLIENTS; i++){
			// 	if(clientIDs[i]!=0){
			// 		sendlen = send(clientSockets[i], (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
			// 		if(sendlen==-1){ //error
			// 			printf("error sending to socket %d\n", connectionSocket);
			// 			// sem_wait(&w_mutex);		//wait for lock
			// 			// sem_post(&w_mutex);		//release lock
			// 		}else{
			// 			printf("sent %zd bytes to client %d\n", sendlen, clientIDs[i]);

			// 		}
			// 	}
			// }
		}
	}
}


//exitrequest listener
void* informExit(void *args){
	int serverSocket = *(int *)args;

	while(1){		
		size_t commandlen = COMMAND_LEN*sizeof(char), sendlen;
		char *command = malloc(commandlen);
		scanf("%s", command);

		if(strcmp(EXIT_REQUEST, command)==0){				
			printf("closing server, informing clients...\n");
			
			//tell each client
			for(int i = 0; i<MAX_CLIENTS; i++){
				if(clientIDs[i] != 0){
					sendlen = send(clientSockets[i], (void *)command, commandlen, MSG_NOSIGNAL);
					if(sendlen==-1){ //error
						printf("error sending to socket %d\n", clientSockets[i]);
						// sem_wait(&w_mutex);		//wait for lock
						// sem_post(&w_mutex);		//release lock
					}else{
						printf("sent %zd bytes to client %d\n", sendlen, clientIDs[i]);
					}
				}
			}
			printf("%d\n", close(serverSocket));
			exit(0);	
		}
	}

}




char *readCommand(){
	getline(&line, &bufsize,stdin);
	if(bufsize>100){
		fprintf(stderr, "shell: input line is too long\n");
		bufsize = 100;
	}
	return line;
}


char **parseCommand(char * command){
	int index = 0;
	char *next = strtok(command, delim);

	while(next!=NULL && index<50){
		tokens[index++] = next;
		next = strtok(NULL, delim);
	}
	tokens[index] = 0;//end of args
	return tokens;
}



int exec_internal(char **args, int clientInd) {
	char *command = args[0];

	if(strcmp(command,"cd")==0){		
		cd(args+1);
		return 0;

	}
	else if(strcmp(command,"create_dir")==0){	
		create_dir(args+1);
		return 0;
	}
	else if(strcmp(command, EXIT_REQUEST)==0){
		exitShell(clientInd);
		return 0;
	}
	else if(strcmp(command,"fget")==0){
		fget(args+1);
		return 0;
	}
	else if(strcmp(command,"fput")==0){
		fput(args+1);
		return 0;
	}
	else if(strcmp(command,"ls")==0){
		ls(args+1);
		return 0;
	}
	printf("command %s not recognized\n", command);
	return -1;				// not a internal command
}


int cd(char **args) {
	return 0;
}

int create_dir(char **args) {
	return 0;
}

int exitShell(int clientInd) {
	printf("client %s disconnected\n", userNames[clientIDs[clientInd]]);
	//takeaway client's ID and close socket
	clientIDs[clientInd] = 0;
	close(clientSockets[clientInd]);
	numClients--;
	pthread_exit(NULL);
	return 0;
}

int fget(char **args) {
	return 0;
}

int fput(char **args) {
	return 0;
}

int ls(char **args) {
	return 0;
}