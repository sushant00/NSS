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

#include "utils.c"

#define MAX_USERS 20
#define MAX_USERNAME_LEN 50
#define MAX_CLIENTS 10

#define EXIT_REQUEST "exit"
#define EMPTY_CLIENT_MARK -1

#define COMMAND_LEN 1024
#define MAX_TOKENS 50		//assume max tokens = 50

#define NUM_COMMANDS 12


//supported commands
int who(unsigned char **args, int clientInd);
int write_all(unsigned char **args, int clientInd);
int exitShell(unsigned char **args, int clientInd);
int create_group(unsigned char **args, int clientInd);
int group_invite(unsigned char **args, int clientInd);
int group_invite_accept(unsigned char **args, int clientInd);
int request_public_key(unsigned char **args, int clientInd);
int send_public_key(unsigned char **args, int clientInd);
int init_group_dhxchg(unsigned char **args, int clientInd);
int write_group(unsigned char **args, int clientInd);
int list_user_files(unsigned char **args, int clientInd);
int request_file(unsigned char **args, int clientInd);


const unsigned char *COMMAND_NAMES[NUM_COMMANDS] = {
	"who", "write_all", "exit", "create_group",
	"group_invite", "group_invite_accept",
	"request_public_key", "send_public_key",
	"init_group_dhxchg", "write_group",
	"list_user_files", "request_file"};

const int(*COMMAND_FUNCS[NUM_COMMANDS])(unsigned char **, int) = {
	who, write_all, exitShell, create_group,
	group_invite, group_invite_accept,
	request_public_key, send_public_key,
	init_group_dhxchg, write_group,
	list_user_files, request_file
};


const unsigned char *delim = " \n\r\t";//space, carriage return , newline, tab


void* handleConnection(void *args);
void* informExit(void *args);
void* listenKDC(void *args);

int initVars(void);
int userConnected(int userID);
int getEmptySpot(void);

unsigned char **parseCommand(unsigned char *command, unsigned char **tokens);
int exec_internal(unsigned char **args, int clientInd);
unsigned char *getUserName(int uid);

//shell vars
size_t bufsize = 1024;		//assume max line length = 100 chars
unsigned char *line;

// Synchronisation vars
int numClients;
int clientSockets[MAX_CLIENTS];
int clientIDs[MAX_CLIENTS];
unsigned char *clientSendBuf[MAX_CLIENTS];
unsigned char *clientRecvBuf[MAX_CLIENTS];
unsigned char *clientKeyBuf[MAX_CLIENTS];

sem_t w_mutex;					//threads acquire lock before modifying clientSockets, numClients;
int readCount = 0;				//read count for clients reading clientSockets


//struct for args to connection threads
struct Args {	
	int connectionSocket;
	// int clientID;
};

struct ListenArgs {
	int socket;
	unsigned char *serverType;
	void (* handler)(void *);
};

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
		fprintf(stderr, "KDC: could not bind KDC\n");
		exit(1);
	}

	printf("KDC: ready\n");

	//IPv4 address SERVER
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(SERVER_PORT);		//host to network byte(big endian) short
	local_addr.sin_addr.s_addr = INADDR_ANY;		//bind to every local interface

	len = sizeof(local_addr);
	ret = bind(serverSocket, (struct sockaddr *)&local_addr, (socklen_t)len);
	if(ret==-1){	//error
		fprintf(stderr, "KDC: could not bind\n");
		exit(1);
	}

	printf("Chat Server: ready\n");

	pthread_t kdc_thread;
	pthread_create(&kdc_thread, NULL, listenKDC, (void *)&kdcSocket);

	ret = listen(serverSocket, MAX_CLIENTS);//ignore if too many clients waiting
	if(ret==-1){	//error
		fprintf(stderr, "Chat Server: could not listen\n");
		exit(1);
	}

	printf("Chat Server: listening...Type %s to close the server anytime\n", EXIT_REQUEST);

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

			size_t sendmsglen = MSG_LEN*sizeof(unsigned char);
			unsigned char *sendmsg = malloc(sendmsglen);
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
	pthread_join(kdc_thread, (void **)(&ret));

	return ret;
}

void* listenKDC(void *args){
	//get the arguments
	int kdcSocket = *(int *)args;

	int ret = listen(kdcSocket, MAX_CLIENTS);//ignore if too many clients waiting
	if(ret==-1){	//error
		fprintf(stderr, "KDC: could not listen\n");
		exit(1);
	}
	printf("KDC: listening \n");

	size_t recvmsglen = MSG_LEN*sizeof(unsigned char);
	unsigned char *recvmsg = malloc(recvmsglen);
	size_t recvlen;

	size_t sendmsglen = MSG_LEN*sizeof(unsigned char);
	unsigned char *sendmsg = malloc(sendmsglen);
	size_t sendlen;
	
	int uidClient;
	int nonceClient;

	int cipherLen;
	unsigned char *ciphertext;

	unsigned char *key = malloc(KEY_LEN_BITS/(sizeof(unsigned char)*8));
	unsigned char *iv = malloc(KEY_LEN_BITS/(sizeof(unsigned char)*8));

	//wait for clients
	while(1){
		struct sockaddr_in connection_addr;
		int len = sizeof(connection_addr);
		int connectionSocket = accept(kdcSocket, (struct sockaddr *)&connection_addr, (socklen_t *)&len);
		if(connectionSocket==-1){	//error
			fprintf(stderr, "KDC: [error] could not accept connection\n");
			exit(1);
		}
		printf("KDC: new connection incoming \n");
		//receive the msg
		recvlen = recv(connectionSocket, (void *)recvmsg, recvmsglen, 0);

		if (recvlen <= 0){
			if(recvlen==-1){ //error
				printf("KDC: error receiving \n");
			}else if(recvlen==0){
				printf("KDC: client disconnected abruptly\n");
			}
			close(connectionSocket);
		}
		else{
			unsigned char *lineEnd = strchr(recvmsg, '\n');
			if(lineEnd==0){
				printf("KDC: no line end in recv msg\n");
			}else{

			*lineEnd = 0;
			}
			printf("KDC: received %zd bytes: %s\n", recvlen, recvmsg);
			unsigned char tmp = recvmsg[UID_LEN];
			recvmsg[UID_LEN] = 0;
			uidClient = atoi(recvmsg);
			recvmsg[UID_LEN] = tmp;

			nonceClient = atoi(recvmsg + UID_LEN + UID_LEN);
		}
		printf("KDC: uid %d, nonce %d\n", uidClient, nonceClient);
		sendmsglen = 0;
		getKeyIVUser(atoi(UID_CHAT_SERVER), key, iv);
		sendmsglen += KEY_LEN_BITS/(sizeof(unsigned char)*8);
		strncpy(sendmsg, key, sendmsglen);
		sendmsglen += UID_LEN;
		strncpy(sendmsg + KEY_LEN_BITS/(sizeof(unsigned char)*8), recvmsg, UID_LEN);

		ciphertext = malloc(sizeof(unsigned char)*sendmsglen + KEY_LEN_BITS );
		cipherLen = cipher(sendmsg, (int)sendmsglen, ciphertext, 1, atoi(UID_CHAT_SERVER));

		sprintf(sendmsg, "%d", nonceClient);
		sendmsglen = NONCE_LEN;
		strncpy(sendmsg + sendmsglen, key, KEY_LEN_BITS/(sizeof(unsigned char)*8));
		sendmsglen += KEY_LEN_BITS/(sizeof(unsigned char)*8);
		strcpy(sendmsg + sendmsglen, UID_CHAT_SERVER);
		sendmsglen += UID_LEN;
		strncpy(sendmsg + sendmsglen, ciphertext, cipherLen);
		sendmsglen += cipherLen;

		ciphertext = malloc(sizeof(unsigned char)*sendmsglen + KEY_LEN_BITS );
		cipherLen = cipher(sendmsg, (int)sendmsglen, ciphertext, 1, uidClient);

		printf("KDC: sending to client %s\n", ciphertext);

		sendlen = send(connectionSocket, (void *)ciphertext, cipherLen, MSG_NOSIGNAL);
		if(sendlen==-1){ //error
			printf("KDC: [error] sending to socket %d\n", connectionSocket);
			close(connectionSocket);
		}else{
			printf("KDC: sent %ld bytes, cipher len %d\n", sendlen, cipherLen);
		}
	}
	return 0;
}


//handler for a client
void* handleConnection(void *Args){
	//get the arguments
	struct Args *con_args = Args;
	int connectionSocket = con_args->connectionSocket;

	size_t recvmsglen = MSG_LEN*sizeof(unsigned char);
	unsigned char *recvmsg = malloc(recvmsglen);
	size_t recvlen;

	size_t sendmsglen = MSG_LEN*sizeof(unsigned char);
	unsigned char *sendmsg = malloc(sendmsglen);
	size_t sendlen;

	unsigned char *key = malloc(KEY_LEN_BITS/(sizeof(unsigned char)*8));
	unsigned char *plaintext = malloc(sizeof(unsigned char)*sendmsglen + KEY_LEN_BITS );
	int plainLen;
	int uidClient;

	// receive the ticket
	recvlen = recv(connectionSocket, (void *)recvmsg, recvmsglen, 0);

	if (recvlen <= 0){
		if(recvlen==-1){ //error
			printf("Chat Server: error receiving \n");
		}else if(recvlen==0){
			printf("Chat Server: client disconnected abruptly\n");
		}
		close(connectionSocket);
		numClients--;

		free(sendmsg);
		free(recvmsg);
		free(key);
		free(plaintext);
		pthread_exit(NULL);
	}
	else{
		unsigned char *lineEnd = strchr(recvmsg, '\n');
		if(lineEnd==0){
			printf("Chat Server: no line end in recv msg\n");
		}else{
			*lineEnd = 0;
		}
		printf("Chat Server: received %zd bytes: %s\n", recvlen, recvmsg);
		plainLen = cipher(recvmsg, (int)recvlen, plaintext, 0, uidClient);
		strncpy(key, plaintext, KEY_LEN_BITS/(sizeof(unsigned char)*8));
		uidClient = atoi(plaintext + KEY_LEN_BITS/(sizeof(unsigned char)*8));

		printf("Chat Server: uidClient %d, shared key %s\n", uidClient, key);
	}

	free(sendmsg);
	free(recvmsg);
	free(plaintext);

	//authenticate the user
	int clientInd;
	if( !(userConnected(uidClient) >= 0) && (clientInd = getEmptySpot() >=0 ) ){
		clientIDs[clientInd] = uidClient;
		clientSockets[clientInd] = connectionSocket;
		//allocate memory for sending and receiving
		clientSendBuf[clientInd] = malloc(MSG_LEN*sizeof(char));
		clientRecvBuf[clientInd] = malloc(MSG_LEN*sizeof(char));
		clientKeyBuf[clientInd] = key;
					
	}else{
		if(clientInd<0){
			printf("Chat Server: no empty spot\n");
		}else{
			printf("Chat Server: user %d already connected\n", uidClient);
		}		
		close(connectionSocket);
		numClients--;
		free(key);
		pthread_exit(NULL);
	}

	printf("Chat Server: authenticated and connected user\n");

	unsigned char **args;
	int executed;
	unsigned char **tokens = malloc(sizeof(unsigned char *)*MAX_TOKENS);

	while(1){
		//receive the msg
		recvlen = recv(connectionSocket, (void *)recvmsg, recvmsglen, 0);

		if (recvlen <= 0){
			if(recvlen==-1){ //error
				printf("error receiving from %s\n", getUserName(clientIDs[clientInd]));
			}else if(recvlen==0){
				printf("client %s disconnected abruptly\n", getUserName(clientIDs[clientInd]));
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
			printf("received %zd bytes from client %s: %s\n", recvlen, getUserName(clientIDs[clientInd]), recvmsg);
	
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
			sprintf(sendmsg, "%s@server$ ", getUserName(clientIDs[clientInd]));
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
			printf("userConnected: user %d %s already connected\n", userID, getUserName(userID));
			return clientInd;
		}
		clientInd++;
	}
	printf("userConnected: user %d %s not connected\n", userID, getUserName(userID));
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
		size_t commandlen = COMMAND_LEN*sizeof(unsigned char), sendlen;
		unsigned char *command = malloc(commandlen);
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




unsigned char **parseCommand(unsigned char * command, unsigned char **tokens){
	int index = 0;
	unsigned char *next = strtok(command, delim);

	while(next!=NULL && index<MAX_TOKENS){
		tokens[index++] = next;
		next = strtok(NULL, delim);
	}
	tokens[index] = 0;//end of args
	return tokens;
}



int exec_internal(unsigned char **args, int clientInd) {
	unsigned char *command = args[0];

	for(int i=0; i<NUM_COMMANDS; i++){
		if(strcmp(command, COMMAND_NAMES[i])==0){
			COMMAND_FUNCS[i](args+1, clientInd);
		}
	}
	printf("command %s not recognized\n", command);
	return -1;				// not a internal command
}

unsigned char *getUserName(int uid){

	struct passwd *pw_s;
	pw_s = getpwuid(uid);
	if(pw_s == NULL) {
		perror("getUserName: error finding uid ");
		return NULL;
	}
	return pw_s->pw_name;
}


int exitShell(unsigned char **args, int clientInd) {
	printf("client %s disconnected\n", getUserName(clientIDs[clientInd]));
	//takeaway client's ID and close socket
	clientIDs[clientInd] = EMPTY_CLIENT_MARK;
	close(clientSockets[clientInd]);
	numClients--;
	pthread_exit(NULL);
	return 0;
}

int who(unsigned char **args, int clientInd){
	return 0;
}

int write_all(unsigned char **args, int clientInd){
	return 0;
}

int create_group(unsigned char **args, int clientInd){
	return 0;
}
int group_invite(unsigned char **args, int clientInd){
	return 0;
}
int group_invite_accept(unsigned char **args, int clientInd){
	return 0;
}
int request_public_key(unsigned char **args, int clientInd){
	return 0;
}
int send_public_key(unsigned char **args, int clientInd){
	return 0;
}
int init_group_dhxchg(unsigned char **args, int clientInd){
	return 0;
}
int write_group(unsigned char **args, int clientInd){
	return 0;
}
int list_user_files(unsigned char **args, int clientInd){
	return 0;
}
int request_file(unsigned char **args, int clientInd){
	return 0;
}