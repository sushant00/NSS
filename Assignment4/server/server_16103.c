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
#define MAX_GROUPS 4
#define MAX_USER_PER_GROUP 5

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
int send_msg(int clientInd, size_t msg_len);
int recv_msg(int clientInd, size_t msg_len, int decrypt);
int isUser(int uid);
int getClientInd(int uid);
int inGrp(int uid, int gid);
int emptySpace(int gid, int arr[][MAX_USER_PER_GROUP]);
int isInvited(int uid, int gid);

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
int groupInvites[MAX_GROUPS][MAX_USER_PER_GROUP];
int groupUsers[MAX_GROUPS][MAX_USER_PER_GROUP];
unsigned char *clientSendBuf[MAX_CLIENTS];
unsigned char *clientRecvBuf[MAX_CLIENTS];
unsigned char *clientKeyBuf[MAX_CLIENTS];
unsigned char *clientCipherBuf[MAX_CLIENTS];
unsigned char *clientPlainBuf[MAX_CLIENTS];

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
			int len = sprintf(sendmsg, "server: Too many conncetions. Try later.\n");
			sendmsg[len] = 0;
			int sendlen = send(connectionSocket, (void *)sendmsg, sendmsglen, MSG_NOSIGNAL);
			close(connectionSocket);
			free(sendmsg);
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

	unsigned char *key = malloc(KEY_LEN_BYTES+1);
	unsigned char *iv = malloc(KEY_LEN_BYTES+1);

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
				// printf("KDC: no line end in recv msg\n");
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
		getSharedKeyIV(atoi(UID_CHAT_SERVER), uidClient, key, iv);
		sendmsglen += KEY_LEN_BYTES;
		strncpy(sendmsg, key, sendmsglen);
		sendmsglen += UID_LEN;
		strncpy(sendmsg + KEY_LEN_BYTES, recvmsg, UID_LEN);

		// A is client, B is chat server, S is KDC. Encrypt Kab, A with Kbs
		ciphertext = malloc(sizeof(unsigned char)*sendmsglen + KEY_LEN_BITS );
		cipherLen = cipher(sendmsg, (int)sendmsglen, ciphertext, 1, atoi(UID_CHAT_SERVER), NULL);

		sprintf(sendmsg, "%d", nonceClient);
		sendmsglen = NONCE_LEN;
		strncpy(sendmsg + sendmsglen, key, KEY_LEN_BYTES);
		sendmsglen += KEY_LEN_BYTES;
		strcpy(sendmsg + sendmsglen, UID_CHAT_SERVER);
		sendmsglen += UID_LEN;
		strncpy(sendmsg + sendmsglen, ciphertext, cipherLen);
		sendmsglen += cipherLen;

		free(ciphertext);
		ciphertext = malloc(sizeof(unsigned char)*sendmsglen + KEY_LEN_BITS );
		cipherLen = cipher(sendmsg, (int)sendmsglen, ciphertext, 1, uidClient, NULL);

		printf("KDC: sending to client %s\n", ciphertext);

		sendlen = send(connectionSocket, (void *)ciphertext, cipherLen, MSG_NOSIGNAL);
		if(sendlen==-1){ //error
			printf("KDC: [error] sending to socket %d\n", connectionSocket);
			close(connectionSocket);
		}else{
			printf("KDC: sent %ld bytes, cipher len %d\n", sendlen, cipherLen);
		}
		free(ciphertext);
	}
	return 0;
}


//handler for a client
void* handleConnection(void *Args){
	//get the arguments
	struct Args *con_args = Args;
	int connectionSocket = con_args->connectionSocket;
	printf("handleConnection: connectionSocket %d--------------------------------------------\n", connectionSocket);

	size_t recvmsglen = MSG_LEN*sizeof(unsigned char);
	unsigned char *recvmsg = malloc(recvmsglen);
	size_t recvlen;

	size_t sendmsglen = MSG_LEN*sizeof(unsigned char);
	unsigned char *sendmsg = malloc(sendmsglen);
	size_t sendlen;

	unsigned char *key = malloc(KEY_LEN_BYTES+1);
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
		// unsigned char *lineEnd = strchr(recvmsg, '\n');
		// if(lineEnd==0){
		// 	printf("Chat Server: no line end in recv msg\n");
		// }else{
		// 	*lineEnd = 0;
		// }
		printf("Chat Server: received %zd bytes: %s\n", recvlen, recvmsg);
		plainLen = cipher(recvmsg, (int)recvlen, plaintext, 0, uidClient, NULL);
		strncpy(key, plaintext, KEY_LEN_BYTES);
		uidClient = atoi(plaintext + KEY_LEN_BYTES);

		printf("Chat Server: uidClient %d, shared key %s\n", uidClient, key);
	}

	free(sendmsg);
	free(recvmsg);
	free(plaintext);

	//authenticate the user
	int clientInd;
	if( !(userConnected(uidClient) >= 0) && ((clientInd = getEmptySpot()) >=0 ) ){
		clientIDs[clientInd] = uidClient;
		clientSockets[clientInd] = connectionSocket;
		//allocate memory for sending and receiving
		clientSendBuf[clientInd] = malloc(MSG_LEN*sizeof(char));
		clientRecvBuf[clientInd] = malloc(MSG_LEN*sizeof(char));
		clientCipherBuf[clientInd] = malloc((MSG_LEN + KEY_LEN_BITS)*sizeof(char));
		clientPlainBuf[clientInd] = malloc((MSG_LEN + KEY_LEN_BITS)*sizeof(char));
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

	printf("Chat Server: authenticated and connected user %s.\n", getUserName(uidClient));

	unsigned char **args;
	int executed;
	unsigned char **tokens = malloc(sizeof(unsigned char *)*MAX_TOKENS);

	while(1){
		//receive the msg
		recvlen = recv_msg(clientInd, recvmsglen, 1);
		if (recvlen == -1){
			printf("Chat Server: removing client\n");
			free(tokens);
			pthread_exit(NULL);
		}
		else{
			unsigned char *lineEnd = strchr(clientPlainBuf[clientInd], '\n');
			if(lineEnd!=NULL){
				*lineEnd = 0;
			}	
			if(clientPlainBuf[clientInd][0] == 0) {
				printf("blank msg received\n");
			}else{
				args = parseCommand(clientPlainBuf[clientInd], tokens);
				// printf("parsed the command\n");

				//check and execute if internal command
				exec_internal(args, clientInd);
			}
			sendmsglen = sprintf(clientSendBuf[clientInd], "%s@server$ ", getUserName(clientIDs[clientInd]));
			clientSendBuf[clientInd][sendmsglen] = 0;
			sendlen = send_msg(clientInd, sendmsglen);
			if(sendlen==-1){ //error
				printf("[error] sending to socket %d\n", connectionSocket);
			}
		}
	}
	free(tokens);
	pthread_exit(NULL);
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
	printf("userConnected: user %d %s first connection\n", userID, getUserName(userID));
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
	for(int i=0; i<MAX_GROUPS; i++){
		for(int j=0; j<MAX_USER_PER_GROUP; j++){
			groupUsers[i][j] = EMPTY_CLIENT_MARK;
			groupInvites[i][j] = EMPTY_CLIENT_MARK;
		}
	}
}

//exitrequest listener
void* informExit(void *args){
	int serverSocket = *(int *)args;
	// printf("server socket %d\n", serverSocket);

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
		free(command);
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
			int ret = COMMAND_FUNCS[i](args+1, clientInd);
			if(ret<0){
				strcpy(clientSendBuf[clientInd], "[error] check the arguments\n");
				if(send_msg(clientInd, strlen(clientSendBuf[clientInd])) < 0){
					return -1;
				}
				return -1;
			}
			return 0;
		}
	}
		//informing client
	// int sendmsglen = sprintf(clientSendBuf[clientInd], "[error] %s is not valid command\n", args[0]);
	// clientSendBuf[clientInd][sendmsglen] = 0;
	// int sendlen = send_msg(clientInd, sendmsglen);
	// if(sendlen==-1){ //error
	// 	printf("[error] sending to socket %d\n", clientSockets[clientInd]);
	// }
	printf("command %s not recognized\n", command);
	return -1;
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
	printf("exitShell: client %s disconnected\n", getUserName(clientIDs[clientInd]));
	//takeaway client's ID and close socket
	clientIDs[clientInd] = EMPTY_CLIENT_MARK;
	close(clientSockets[clientInd]);
	numClients--;
	pthread_exit(NULL);
	return 0;
}

//receive msg and decrypt
int recv_msg(int clientInd, size_t recvmsglen, int decrypt){
	int recvlen = recv(clientSockets[clientInd], clientRecvBuf[clientInd], recvmsglen, 0);
	if (recvlen <= 0){
		if(recvlen==-1){ //error
			printf("recv_msg: error receiving from %s\n", getUserName(clientIDs[clientInd]));
		}else if(recvlen==0){
			printf("recv_msg: client %s disconnected abruptly\n", getUserName(clientIDs[clientInd]));
		}
		
		//takeaway client's ID and close socket
		clientIDs[clientInd] = EMPTY_CLIENT_MARK;
		close(clientSockets[clientInd]);
		numClients--;
		// printf("recv_msg: closed client socket\n");
		return -1;
	}
	if(decrypt){
		int plainLen = cipher(clientRecvBuf[clientInd], (int)recvlen, clientPlainBuf[clientInd], 0, -1, clientKeyBuf[clientInd]);
		printf("recv_msg: cipher ret %d\n", plainLen);
		if(plainLen == -1){
			return 0;
		}
		return plainLen;
	}
	return recvlen;
}

//encrypt the message and send
int send_msg(int clientInd, size_t msg_len){
	printf("send_msg: %s to %s\n", clientSendBuf[clientInd], getUserName(clientIDs[clientInd]));
	int cipherLen = cipher(clientSendBuf[clientInd], (int)msg_len, clientCipherBuf[clientInd],
		1, -1, clientKeyBuf[clientInd]);

	strncpy(clientSendBuf[clientInd], clientCipherBuf[clientInd], cipherLen);
	clientSendBuf[clientInd][cipherLen] = 0;
	size_t sendlen = send(clientSockets[clientInd], (void *)clientSendBuf[clientInd], cipherLen, MSG_NOSIGNAL);

	if(sendlen==-1){ //error
		printf("error sending to socket %d\n", clientSockets[clientInd]);
		return -1;
	}else{
		printf("send_msg: sent %ld bytes to %s\n", sendlen, getUserName(clientIDs[clientInd]));
	}
	return sendlen;
}

int who(unsigned char **args, int clientInd){
	printf("who: num clients %d\n", numClients);
	printf("who: client %s called\n", getUserName(clientIDs[clientInd]));
	for(int i=0; i<MAX_CLIENTS; i++){
		// printf("check %d\n", clientIDs[i]);
		if(clientIDs[i] != EMPTY_CLIENT_MARK) {
			int len = sprintf(clientSendBuf[clientInd], "%-13s  %-4d\n", getUserName(clientIDs[i]), clientIDs[i]);
			clientSendBuf[clientInd][len] = 0;
			// printf("who: %ld\n", strlen(clientSendBuf[clientInd]));
			if(send_msg(clientInd, 20)<0){
				return -1;
			}
		}
	}
	printf("who: done\n");
	return 0;
}

int write_all(unsigned char **args, int clientInd){
	printf("write_all: client %s called\n", getUserName(clientIDs[clientInd]));
	for(int i=0; i<MAX_CLIENTS; i++){
		if((clientIDs[i] != EMPTY_CLIENT_MARK) && (i!=clientInd)) {
			int j=0;
			int len = 0;
			len += sprintf(clientSendBuf[i]+len, "%s: ", getUserName(clientIDs[clientInd]));
			while(args[j]!=NULL){
				len += sprintf(clientSendBuf[i]+len, "%s ", args[j]);
				j+=1;
			}
			clientSendBuf[i][len] = '\n';
			clientSendBuf[i][len+1] = 0;
			if(send_msg(i, strlen(clientSendBuf[i]))<0){
				return -1;
			}			
		}
	}
	printf("write_all: done\n");
	return 0;
}

int isUser(int uid){
	struct passwd *pw_s;
	pw_s = getpwuid(uid);
	if(pw_s == NULL){
		return -1;
	}
	return 0;
}

int getClientInd(int uid){
	for(int i=0; i<MAX_USERS; i++){
		if(clientIDs[i] == uid){
			return i;
		}
	}
	return -1;
}

int create_group(unsigned char **args, int clientInd){
	int gid = EMPTY_CLIENT_MARK;
	for(int i=0; i<MAX_USERS; i++){
		if(groupUsers[i][0] == EMPTY_CLIENT_MARK){
			gid = i;
			printf("create_group: created grp gid %d\n", gid);
			break;
		}
	}

	if(gid == EMPTY_CLIENT_MARK){
		printf("create_group: max group limit reached\n");
		return -1;
	}

	groupUsers[gid][0] = clientIDs[clientInd];
	printf("create_group: added user %s id %d to grp %d\n", getUserName(clientIDs[clientInd]), clientIDs[clientInd], gid);

	int ind=0;
	while(args[ind]!=NULL) {
		int uid = atoi(args[ind]);
		if(uid>0 && isUser(uid)>=0){		
			printf("create_group: user %s uid %d ok\n", args[ind], uid);	
			for(int j=0; j<MAX_USER_PER_GROUP; j++) {
				// printf("create_group: groupUsers[gid][%d] %d\n", j, groupUsers[gid][j]);
				if(groupUsers[gid][j] == uid){
					printf("create_group: user %s already added to grp\n", args[ind]);
					break;
				}
				if(groupUsers[gid][j] == EMPTY_CLIENT_MARK){
					groupUsers[gid][j] = uid;
					printf("create_group: added user %s id %d to grp %d\n", args[ind], uid, gid);
					break;
				}
			}
			if(groupUsers[gid][MAX_USER_PER_GROUP-1] != EMPTY_CLIENT_MARK){
				printf("create_group: max group members limit reached\n");
				break;
			}
		}else{
			printf("create_group: %s or id %d is invalid user id\n", args[ind], uid);
		}
		ind++;
	}

	printf("create_group: informing added users\n");	

	for(int i=0; i<MAX_USER_PER_GROUP; i++){
		if(groupUsers[gid][i] != EMPTY_CLIENT_MARK){
			int curClientInd = getClientInd(groupUsers[gid][i]);
			if(curClientInd < 0){
				printf("create_group: user %d not online\n", groupUsers[gid][i]);
				continue;
			}
			printf("create_group: informing %d client ind %d\n",groupUsers[gid][i], curClientInd);
			int len = sprintf(clientSendBuf[curClientInd], "create_group: you are added to group %d\n", gid);
			clientSendBuf[curClientInd][len] = 0;
			if(send_msg(curClientInd, len) < 0){
				return -1;
			}
		}else{
			break;
		}
	}
	return 0;
}

int inGrp(int uid, int gid){
	if((gid >= 0) && (gid < MAX_GROUPS) && (groupUsers[gid][0] != EMPTY_CLIENT_MARK)){
	 	for(int i=0; i<MAX_USER_PER_GROUP; i++){
			// printf("inGrp: grpUsers[%d][%d] = %d, uid %d\n", gid, i, groupUsers[gid][i], uid);
			if(groupUsers[gid][i] == uid){
				return i;
			}
		}
	}else{
		// printf("inGrp: error\n");
	}
	return -1;
}


int emptySpace(int gid, int arr[][MAX_USER_PER_GROUP]){
	for(int i=0; i<MAX_USER_PER_GROUP; i++){
		if(arr[gid][i] == EMPTY_CLIENT_MARK){
			return i;
		}
	}
	return -1;
}


int group_invite(unsigned char **args, int clientInd){
	if(args[0]==NULL || args[1]==NULL){
		printf("group_invite: 2 arguments required\n");
		return -1;
	}

	int gid = atoi(args[0]);
	int uid = atoi(args[1]);
	printf("group_invite: group %s id %d, user %s id %d\n", args[0], gid, args[1], uid);

	if(isUser(uid)<0){
		printf("group_invite: user %d is invalid\n", uid);
		return -1;
	}

	// check if the calling user is allowed to invite to group
	if( (inGrp(clientIDs[clientInd], gid) >= 0) ){
		printf("group_invite: allowed to invite to %d\n", gid);
		if(inGrp(uid, gid) >= 0){
			printf("group_invite: user %d already in grp %d\n", uid, gid);
		}else{
			int curClientInd = getClientInd(uid);
			int len;
			int inviteInd = emptySpace(gid, groupInvites);
			if(inviteInd < 0){
				printf("group_invite: max invitation limit reached\n");
				len = sprintf(clientSendBuf[curClientInd], "max invitation limit reached to group %d\n", gid);
				clientSendBuf[curClientInd][len] = 0;
			}else{
				groupInvites[gid][inviteInd] = uid;
				len = sprintf(clientSendBuf[curClientInd], "group_invite: you are invited to group %d\n", gid);
				clientSendBuf[curClientInd][len] = 0;
				printf("group_invite: user %d invited in grp %d\n", uid, gid);
			}
			send_msg(curClientInd, len);
		}
	}else{
		printf("group_invite: gid %d invalid\n", gid);
		return -1;
	}
	return 0;
}

int isInvited(int uid, int gid){
	for(int i=0; i<MAX_USER_PER_GROUP; i++){
		if(groupInvites[gid][i] == uid){
			return i;
		}
	}
	return -1;
}

int group_invite_accept(unsigned char **args, int clientInd){
	if(args[0]==NULL){
		printf("group_invite_accept: 1 argument required\n");
		return -1;
	}

	int gid = atoi(args[0]);
	int uid = clientIDs[clientInd];

	printf("group_invite_accept: user %d accpeted invited to group %s id %d\n", uid, args[0], gid);
	
	if( (gid >= 0) && (gid < MAX_GROUPS) && (groupUsers[gid][0] != EMPTY_CLIENT_MARK) ){
		
		if(inGrp(uid, gid)>=0){
			printf("group_invite_accept: user %d already in grp %d\n", uid, gid);
			return -1;
		}
		int ind = isInvited(uid, gid);
		int len;
		if(ind >= 0){
			ind = emptySpace(gid, groupUsers);
			if(ind<0){
				printf("group_invite_accept: max group users limit reached in grp %d\n", gid);
				len = sprintf(clientSendBuf[clientInd], "group_invite_accept: max group users limit reached in grp %d\n", gid);
				clientSendBuf[clientInd][len] = 0;
			}else{
				groupUsers[gid][ind] = uid;
				groupInvites[gid][ind] = EMPTY_CLIENT_MARK;
				len = sprintf(clientSendBuf[clientInd], "group_invite_accept: you are added to group %d\n", gid);
				clientSendBuf[clientInd][len] = 0;
				printf("group_invite: user %d added in grp %d\n", uid, gid);
			}
		}else{
			printf("group_invite_accept: user %d not invited in grp %d\n", uid, gid);
			len = sprintf(clientSendBuf[clientInd], "group_invite_accept: you were not invited in grp %d\n", gid);
			clientSendBuf[clientInd][len] = 0;
		}
		send_msg(clientInd, len);
	}else{
		printf("group_invite_accept: gid %d invalid\n", gid);
		return -1;
	}
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
	if(args[0]==NULL || args[1]==NULL){
		printf("write_group: 2 argument required\n");
		return -1;
	}

	int gid = atoi(args[0]);
	int uid = clientIDs[clientInd];

	printf("write_group: user %d wants to write to group %s id %d\n", uid, args[0], gid);
	
	if( (inGrp(uid, gid) >= 0) ){
		printf("write_group: client %s is in grp %d\n", getUserName(clientIDs[clientInd]), gid);
		for(int i=0; i<MAX_USER_PER_GROUP; i++){
			if(groupUsers[gid][i] == EMPTY_CLIENT_MARK){
				break;
			}
			int curClientInd = getClientInd(groupUsers[gid][i]);
			printf("write_group: cur client ind %d client ind %d\n", curClientInd, clientInd);
			if((clientIDs[curClientInd] != EMPTY_CLIENT_MARK) && (curClientInd!=clientInd)) {
				int j=1;
				int len = 0;
				len += sprintf(clientSendBuf[curClientInd]+len, "%s: ", getUserName(uid));
				while(args[j]!=NULL){
					len += sprintf(clientSendBuf[curClientInd]+len, "%s ", args[j]);
					j+=1;
				}
				clientSendBuf[curClientInd][len] = '\n';
				clientSendBuf[curClientInd][len+1] = 0;
				if(send_msg(i, strlen(clientSendBuf[curClientInd]))<0){
					return -1;
				}			
			}
		}
	}else{
		printf("write_group: invalid grp id or user is not a member\n");
		return -1;
	}
	return 0;
}

unsigned char *getHomeDir(int uid){
	struct passwd *pw_s;
	pw_s = getpwuid(uid);
	if(pw_s == NULL) {
		perror("getHomeDir: error finding uid ");
		return NULL;
	}
	return (unsigned char *)pw_s->pw_dir;
}

int list_user_files(unsigned char **args, int clientInd){
	if(args[0]==NULL){
		printf("list_user_files: 1 argument required\n");
		return -1;
	}

	int uid_caller = clientIDs[clientInd];
	int uid_owner = atoi(args[0]);
	printf("list_user_files: called by %d for files of %s id %d\n", uid_caller, args[0], uid_owner);
	if( (isUser(uid_owner) < 0) ){
		printf("list_user_files: invalid user id passed %s\n", args[0]);
		return -1;
	}


	DIR *d;
	struct dirent *dir;
	d = opendir(getHomeDir(uid_owner));

	unsigned char *path = malloc(MAX_DIR_LEN);
	strcpy(path, getHomeDir(uid_owner));
	strcpy(strchr(path,0), "/");
	int len = 0;
	len += sprintf(clientSendBuf[clientInd]+len, "list_user_files: files with read access\n");
	clientSendBuf[clientInd][len] = 0;
	
	seteuid(uid_caller);
	if (d) {
		while (( dir = readdir(d)) != NULL ) {
			// printf("ls: file entry %s\n", dir->d_name);
			if ( dir->d_name[0]=='.' ) {
				// printf("ls: skipping %s\n", dir->d_name);
				continue;
			}
			strcpy(strrchr(path, '/')+1, dir->d_name);
			if( ( (access(path, R_OK)==0) && !(dir->d_type == DT_DIR) ) ){
				printf("list_user_files: read access allowed for %s\n", path);
				len += sprintf(clientSendBuf[clientInd]+len, "%s\n", dir->d_name);
				clientSendBuf[clientInd][len] = 0;
			}else{
				printf("list_user_files: read access not allowed or is a dir %s\n", path);
			}
		}
	}
	seteuid(0);
	int sendlen = send_msg(clientInd, len);
	if(sendlen==-1){ //error
		printf("[error] sending to socket %d\n", clientSockets[clientInd]);
		return -1;
	}
	return 0;
}

int request_file(unsigned char **args, int clientInd){
	return 0;
}