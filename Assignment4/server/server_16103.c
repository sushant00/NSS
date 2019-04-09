//Sushant Kumar Singh
//2016103

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <semaphore.h>

#define MAX_USERS 20
#define MAX_USERNAME_LEN 50
#define MAX_CLIENTS 10

#define KDC_PORT 5555
#define SERVER_PORT 12000
#define MSG_LEN 2024

#define EXIT_REQUEST "exit"
#define EMPTY_CLIENT_MARK -1

#define COMMAND_LEN 1024
#define MAX_TOKENS 50		//assume max tokens = 50


#define NUM_COMMANDS 12

const char *COMMAND_NAMES[NUM_COMMANDS] = {
	"who", "write_all", "exit", "create_group",
	"group_invite", "group_invite_accept",
	"request_public_key", "send_public_key",
	"init_group_dhxchg", "write_group",
	"list_user_files", "request_file"};

const int(*COMMAND_FUNCS[NUM_COMMANDS])(char *, int) = {
	who, write_all, exitShell, create_group,
	group_invite, group_invite_accept,
	request_public_key, send_public_key,
	init_group_dhxchg, write_group,
	list_user_files, request_file
}


const char *delim = " \n\r\t";//space, carriage return , newline, tab


void* handleConnection(void *args);
void* informExit(void *args);

int initVars(void);

int authFile(char *dir_path, int userID, int mode, int isDir);

int isUser(char *username);
int isGrp(char *grp);
int inGrp(char *usergrp, int userID);
int getEmptySpot(void);

char **parseCommand(char *command, char **tokens);
int exec_internal(char **args, int clientInd);

//supported commands
int who(char **args, int clientInd);
int write_all(char **args, int clientInd);
int exitShell(int clientInd);
int create_group(char **args, int clientInd);
int group_invite(char **args, int clientInd);
int group_invite_accept(char **args, int clientInd);


//shell vars
size_t bufsize = 1024;		//assume max line length = 100 chars
char *line;

//user database
char userNames[MAX_USERS+1][MAX_USERNAME_LEN];
char userGroups[MAX_USERS+1][MAX_GROUPS_PER_USER][MAX_USERNAME_LEN];

// Synchronisation vars
int numClients;
int clientSockets[MAX_CLIENTS];
int clientIDs[MAX_CLIENTS];
char *clientSendBuf[MAX_CLIENTS];
char *clientRecvBuf[MAX_CLIENTS];

sem_t w_mutex;					//threads acquire lock before modifying clientSockets, numClients;
int readCount = 0;				//read count for clients reading clientSockets


//struct for args to connection threads
struct Args {	
	int connectionSocket;
	// int clientID;
};

struct ListenArgs {
	int socket;
	char *serverType;
	void (* handler)(void *);
}

// SERVER
int main(void){
	//dont buffer output
	setbuf(stdout, NULL);

	initVars();

	printf("starting server...\n");

	int serverSocket, kdcSocket, len, ret, enable=1;
	struct sockaddr_in local_addr;


	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(serverSocket==-1){	//error
		fprintf(stderr, "could not create server\n");
		exit(1);
	}
	printf("server socket created %d\n", serverSocket);
	ret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const void *)&enable, sizeof(enable));
	if(ret==-1){	//error
		fprintf(stderr, "could not set socket options\n");

	}


	kdcSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(kdcSocket==-1){	//error
		fprintf(stderr, "could not create kdc\n");
		exit(1);
	}
	printf("kdc socket created %d\n", kdcSocket);
	ret = setsockopt(kdcSocket, SOL_SOCKET, SO_REUSEADDR, (const void *)&enable, sizeof(enable));
	if(ret==-1){	//error
		fprintf(stderr, "could not set socket options\n");
	}


	//IPv4 address KDC
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(KDC_PORT);		//host to network byte(big endian) short
	local_addr.sin_addr.s_addr = INADDR_ANY;		//bind to every local interface

	len = sizeof(local_addr);
	ret = bind(kdcSocket, (struct sockaddr *)&local_addr, (socklen_t)len);
	if(ret==-1){	//error
		fprintf(stderr, "could not bind KDC\n");
		exit(1);
	}

	printf("KDC ready\n");

	//IPv4 address SERVER
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

	pthread_t kdc_thread;
	pthread_create(&kdc_thread, NULL, listenKDC, (void *)&kdcSocket);

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
		}

	}

	return 0;
}

void* listenKDC(void *args){
	//get the arguments
	int kdcSocket = *(int *)args;

	ret = listen(kdcSocket, MAX_CLIENTS);//ignore if too many clients waiting
	if(ret==-1){	//error
		fprintf(stderr, "could not listen\n");
		exit(1);
	}
	printf("kdc listening \n");

	return 0;
}


//handler for a client
void* handleConnection(void *Args){
	//get the arguments
	struct Args *con_args = Args;
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
		sprintf(sendmsg, EXIT_REQUEST);
		sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
		if(sendlen==-1){ //error
			printf("[error] sending to socket %d\n", connectionSocket);
		}
		free(sendmsg);
		free(recvmsg);
		close(connectionSocket);
		numClients--;
		pthread_exit(NULL);
	}

	char **args;
	int executed;
	char **tokens = malloc(sizeof(char *)*MAX_TOKENS);

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
			clientIDs[clientInd] = EMPTY_CLIENT_MARK;
			close(clientSockets[clientInd]);
			numClients--;
			free(sendmsg);
			free(recvmsg);
			free(tokens);
			pthread_exit(NULL);
		}
		else{
			*strchr(recvmsg, '\n') = 0;
			printf("received %zd bytes from client %s: %s\n", recvlen, userNames[clientIDs[clientInd]], recvmsg);
	
			if(recvmsg[0] == 0) {
				printf("blank msg received\n");
			}else{
				args = parseCommand(recvmsg, tokens);
				// printf("parsed the command\n");

				//check and execute if internal command
				if (exec_internal(args, clientInd)==-1) {
					//informing client
					sprintf(sendmsg, "[error] %s is not valid command\n", args[0]);
					sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
					if(sendlen==-1){ //error
						printf("[error] sending to socket %d\n", connectionSocket);
					}
				}
			}
			sprintf(sendmsg, "%s@server:%s$ ", userNames[clientIDs[clientInd]], clientPWDs[clientInd]);
			sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
			if(sendlen==-1){ //error
				printf("[error] sending to socket %d\n", connectionSocket);
			}
		}
	}
	free(sendmsg);
	free(recvmsg);
	free(tokens);
}


//check if the user is connected, if connected return index in clientIDs
int userConnected(int userID) {
	int clientInd = 0;
	while( clientInd < MAX_CLIENTS) {
		if( clientIDs[clientInd] == userID ){
			printf("userConnected: user %d %s already connected\n", userID, userNames[userID]);
			return clientInd;
		}
		clientInd++;
	}
	printf("userConnected: user %d %s not connected\n", userID, userNames[userID]);
	return -1;
}


int getEmptySpot(void) {
	int clientInd = 0;
	while( clientInd < MAX_CLIENTS ) {
		if(clientIDs[clientInd] == EMPTY_CLIENT_MARK) {
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
		free(sendmsg);
		free(recvmsg);
		return -1;
	}
	//receive the msg
	printf("waiting for client for username\n");

	recvlen = recv(connectionSocket, (void *)recvmsg, recvmsglen, 0);
	if(recvlen==-1){ //error
		printf("[error] receiving from socket %d\n", connectionSocket);
		free(sendmsg);
		free(recvmsg);
		return -1;
		
	}else if(recvlen==0){
		printf("client socket %d disconnected abruptly\n", connectionSocket);
		free(sendmsg);
		free(recvmsg);
		return -1;

	}else{
		*strchr(recvmsg, '\n') = 0;
		printf("client response received\n");
		int userInd = 0;
		while(userInd < MAX_USERS){
			if(strlen(userNames[userInd])!=0 && strcmp(userNames[userInd], recvmsg)==0) {
				//check if this user is already connected
				if( userConnected(userInd) >= 0 ) {
					printf("user %s already present", recvmsg);	
					sprintf(sendmsg, "server: %s's another session is active. Bye!\n", userNames[userInd]);
					sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
					free(sendmsg);
					free(recvmsg);
					if(sendlen==-1){ //error
						printf("[error] sending to socket %d\n", connectionSocket);
					}
					return -1;
				}else {
					ind = getEmptySpot();
					if (ind==-1){
						printf("no empty spot for connections");
						free(sendmsg);
						free(recvmsg);
						return -1;
					}else{
						printf("user %s connected\n", userNames[userInd]);

						clientIDs[ind] = userInd;
						clientSockets[ind] = connectionSocket;
						strcpy(clientPWDs[ind], PATH_HOME);
						strcpy(clientPWDs[ind]+strlen(PATH_HOME), userNames[userInd]);
						//allocate memory for sending and receiving
						clientSendBuf[ind] = malloc(MSG_LEN*sizeof(char));
						clientRecvBuf[ind] = malloc(MSG_LEN*sizeof(char));
					
						//informing client
						sprintf(sendmsg, "server: authenticated\n%s@server:%s$ ", userNames[userInd], clientPWDs[ind]);
						sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
						if(sendlen==-1){ //error
							printf("[error] sending to socket %d\n", connectionSocket);
							free(sendmsg);
							free(recvmsg);
							return -1;
						}
						free(sendmsg);
						free(recvmsg);
						return ind;
					}					
				}
			}
			userInd++;
		}
	}
	sprintf(sendmsg, "server: authentication failed.\n");
	sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
	free(sendmsg);
	free(recvmsg);
	if(sendlen==-1){ //error
		printf("[error] sending to socket %d\n", connectionSocket);
	}
	return -1;
}


//
int initVars(void) {
	for(int i=0; i<MAX_CLIENTS; i++) {
		clientIDs[i] = EMPTY_CLIENT_MARK;
	}
}


//exitrequest listener
void* informExit(void *args){
	int serverSocket = *(int *)args;
	printf("server socket %d\n", serverSocket);

	while(1){		
		size_t commandlen = COMMAND_LEN*sizeof(char), sendlen;
		char *command = malloc(commandlen);
		scanf("%s", command);

		if(strcmp(EXIT_REQUEST, command)==0){				
			printf("closing server, informing clients...\n");
			
			//tell each client
			for(int i = 0; i<MAX_CLIENTS; i++){
				if(clientIDs[i] != EMPTY_CLIENT_MARK){
					sendlen = send(clientSockets[i], (void *)command, commandlen, MSG_NOSIGNAL);
					if(sendlen==-1){ //error
						printf("error sending to socket %d\n", clientSockets[i]);
						// sem_wait(&w_mutex);		//wait for lock
						// sem_post(&w_mutex);		//release lock
					}else{
						printf("sent %zd bytes to client %d\n", sendlen, clientIDs[i]);
					}
					shutdown(clientSockets[i], SHUT_RDWR);
					close(clientIDs[i]);
				}
			}
			printf("closing server socket %d\n", serverSocket);
			printf("%d\n", close(serverSocket));
			exit(0);	
		}
	}
}




char **parseCommand(char * command, char **tokens){
	int index = 0;
	char *next = strtok(command, delim);

	while(next!=NULL && index<MAX_TOKENS){
		tokens[index++] = next;
		next = strtok(NULL, delim);
	}
	tokens[index] = 0;//end of args
	return tokens;
}



int exec_internal(char **args, int clientInd) {
	char *command = args[0];

	for(int i=0; i<NUM_COMMANDS; i++){
		if(strcmp(command, COMMAND_NAMES[i])==0){
			COMMAND_FUNCS[i](args+1, clientInd);
		}
	}
	printf("command %s not recognized\n", command);
	return -1;				// not a internal command
}



int isUser(char *username) {
	printf("isUser: checking %s\n", username);
	for(int i=0; i<MAX_USERS; i++) {
		if( strlen(userNames[i])!=0 && strcmp(username, userNames[i])==0 ) {
			printf("isUser: %s is user\n", username);
			return 0;
		}
	}
	printf("isUser: %s is not user\n", username);
	return -1;
}

int isGrp(char *grp) {
	printf("isGrp: checking %s\n", grp);
	return isUser(grp);
	// for(int i=0; i<MAX_USERS; i++) {
	// 	if( userGroups[i]!=NULL && strcmp(grp, userGroups[i])==0 ) {
	// 		printf("isGrp: %s is grp\n", grp);
	// 		return 0;
	// 	}
	// }
	// printf("isGrp: %s is not grp\n", grp);
	// return -1;

}


int inGrp(char *usergrp, int userID) {
	printf("inGrp: check %d %s in grp %s\n", userID, userNames[userID], usergrp);
	for(int i=0; i<MAX_GROUPS_PER_USER; i++) {
		// printf("inGrp: %d %s is in grp %s\n", userID, userNames[userID], userGroups[userID][i]);
		if ( strlen(userGroups[userID][i])!=0 && strcmp(userGroups[userID][i], usergrp)==0 ) {
			printf("inGrp: %d %s is in grp %s\n", userID, userNames[userID], usergrp);
			return 0;
		}
	}
	printf("inGrp: %d %s not in grp %s\n", userID, userNames[userID], usergrp);
	return -1;
}




int exitShell(char *args, int clientInd) {
	printf("client %s disconnected\n", userNames[clientIDs[clientInd]]);
	//takeaway client's ID and close socket
	clientIDs[clientInd] = EMPTY_CLIENT_MARK;
	close(clientSockets[clientInd]);
	numClients--;
	pthread_exit(NULL);
	return 0;
}
