//Sushant Kumar Singh
//201NONCE_LEN103

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <semaphore.h>

#include "utils.c"

void* receiver(void *args);
int send_msg(int socket, size_t msglen, int doEnc);
int recv_msg(int socket, size_t msglen, int decrypt);

unsigned char sharedKey[KEY_LEN_BYTES+1];
unsigned char recv_buf[MSG_LEN];
unsigned char send_buf[MSG_LEN];
unsigned char cipher_buf[MSG_LEN];
unsigned char plain_buf[MSG_LEN];


// CLIENT
int main(void){
	//dont buffer the output
	setbuf(stdout, NULL);

	size_t serverNameLen = sizeof(unsigned char)*COMMAND_LEN;
	unsigned char *serverName = "127.0.0.1";

	printf("starting client...\n");

	int clientSocket, clientSocketKDC, len, ret, retKDC, enable=1;
	struct sockaddr_in server_addr;


	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	clientSocketKDC = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket==-1 || clientSocketKDC==-1){	//error
		fprintf(stderr, "could not create client\n");
		exit(1);
	}
	ret = setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (const void *)&enable, sizeof(enable));
	retKDC = setsockopt(clientSocketKDC, SOL_SOCKET, SO_KEEPALIVE, (const void *)&enable, sizeof(enable));
	if(ret==-1 || retKDC==-1){	//error
		fprintf(stderr, "could not set socket options\n");

	}

	printf("client socket created\n");

	//created the buffer
	size_t msglen = MSG_LEN*sizeof(unsigned char);
	size_t sendlen, recvlen;
	unsigned char *msg = malloc(msglen);


	//----------connect to KDC----------------------
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(KDC_PORT);		//host to network byte(big endian) short
	inet_pton(AF_INET, (const unsigned char *)serverName, &server_addr.sin_addr);

	len = sizeof(server_addr);
	ret = connect(clientSocketKDC, (const struct sockaddr *)&server_addr, (socklen_t)len);
	if(ret==-1){//error
		fprintf(stderr, "could not connect to KDC\n");
		exit(1);
	}

	printf("enter your %d digit uid\n", UID_LEN);
	fgets(send_buf, UID_LEN+1, stdin);
	printf("uid client %s\n", send_buf);
	int clientUID = atoi(send_buf);
	strcpy(send_buf + UID_LEN, UID_CHAT_SERVER);
	printf("uid client, uid server %s\n", send_buf);
	int nonce = rand()%(90000) + 100000;
	sprintf(send_buf+UID_LEN+UID_LEN, "%d", nonce);
	printf("msg 1 to kdc:%s", send_buf);
	msglen = strlen(send_buf);
	sendlen = send_msg(clientSocketKDC, msglen, 0);

	printf("recieving ticket from KDC\n");
	//receive the ticket
	recvlen = recv_msg(clientSocketKDC, MSG_LEN, 0);
	printf("received %ld bytes, msg %s\n", recvlen, recv_buf);
	unsigned char *plaintext = malloc(sizeof(unsigned char)*recvlen + KEY_LEN_BITS );
	int plainLen = cipher(recv_buf, recvlen, plaintext, 0, clientUID, NULL);

	//-------------------TICKET----------------------------
	unsigned char *ticket;
	int ticket_len;

	unsigned char tmp = plaintext[NONCE_LEN];
	plaintext[NONCE_LEN] = 0;
	printf("stored nonce: %d, received nonce str: %s, received nonce int: %d\n", nonce, plaintext, atoi(plaintext));
	if(atoi(plaintext) == nonce){
		printf("KDC response's nonce match success!\n");
		plaintext[NONCE_LEN] = tmp;
		if(strncmp(plaintext+NONCE_LEN + KEY_LEN_BYTES, UID_CHAT_SERVER, UID_LEN)==0) {
			printf("KDC sent shared key for chat server\n");

			strncpy(sharedKey, plaintext+NONCE_LEN, KEY_LEN_BYTES);
			sharedKey[KEY_LEN_BYTES] = 0;
			ticket_len = plainLen - NONCE_LEN - UID_LEN - (KEY_LEN_BYTES);
			ticket = malloc( ticket_len );
			strncpy(ticket, plaintext+NONCE_LEN+UID_LEN+(KEY_LEN_BYTES), ticket_len );
			ticket[ticket_len] = 0;
			printf("ticket len %d received from KDC: %s\n", ticket_len, ticket);
		}else{
			printf("KDC sent shared key for unknown client\n");
			return -1;
		}
	}else{
		printf("KDC response's nonce match failed\n");
		return -1;
	}

	close(clientSocketKDC);
	printf("closed KDC socket\n");

	//connect to server
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);		//host to network byte(big endian) short
	inet_pton(AF_INET, (const unsigned char *)serverName, &server_addr.sin_addr);

	//connect with server
	len = sizeof(server_addr);
	ret = connect(clientSocket, (const struct sockaddr *)&server_addr, (socklen_t)len);
	if(ret==-1){	//error
		fprintf(stderr, "could not connect to Chat Server\n");
		exit(1);
	}

	strncpy(send_buf, ticket, ticket_len);
	send_buf[ticket_len] = 0;
	msglen = ticket_len;
	sendlen = send_msg(clientSocket, msglen, 0);
	// printf("sent sendlen %ld, msglen %ld\n", sendlen, msglen);

	printf("client connected to server.\nType msg (max %d chars) and press enter. Type '%s' to close the client.\n", COMMAND_LEN, EXIT_REQUEST);

	//handle the client in new thread
	pthread_t receiver_thread;
	pthread_create(&receiver_thread, NULL, receiver, (void *)&clientSocket);

	//send msgs
	while(1){		
		//take msg line input
		while(1){
			fgets(send_buf, msglen, stdin);
			unsigned char *newline = strchr(send_buf, '\n');
			// *newline = 0;
			// send_buf[(int)(newline - msg)] = '\0';
			if(strlen(send_buf) != 0){
				break;
			}
		}
		sendlen = send_msg(clientSocket, msglen, 1);
	}
	close(clientSocket);
	return 0;
}

//receiver for a client
void* receiver(void *args){
	int clientSocket = *((int *)args);
	size_t msglen;
	size_t recvlen;
	while(1){
		//receive the msg
		msglen = MSG_LEN;
		recvlen = recv_msg(clientSocket, msglen, 1);
	}
}

int recv_msg(int socket, size_t msglen, int decrypt){
	size_t recvlen = recv(socket, (void *)recv_buf, msglen, 0);
	printf("recvlen: %ld, msglen %ld\n", recvlen, msglen);
	if(recvlen==-1){ //error
		printf("error receiving\n");
		return -1;			
	}else if(recvlen==0){
		printf("server disconnected abruptly\n");
		close(socket);
		exit(0);	

	}
	if(decrypt){
		// printf("recv_msg: using shared key %s\n", sharedKey);
		int plainLen = cipher(recv_buf, recvlen, plain_buf, 0, -1, sharedKey);

		if(strcmp(EXIT_REQUEST, plain_buf)==0){				
			printf("server disconnecting\n");
			close(socket);
			exit(0);
		}
		printf("%s", plain_buf);
		return plainLen;
	}else{

		if(strcmp(EXIT_REQUEST, recv_buf)==0){				
			printf("server disconnecting\n");
			close(socket);
			exit(0);
		}
		printf("%s", recv_buf);
		return recvlen;
	}
}



int send_msg(int socket, size_t msglen, int doEnc){
	if(doEnc){
		int cipherLen = cipher(send_buf, msglen, cipher_buf, 1, -1, sharedKey);
		strncpy(send_buf, cipher_buf, cipherLen);
		send_buf[cipherLen] = 0;
		msglen = cipherLen;
	}
	size_t sendlen = send(socket, (void *)send_buf, msglen, MSG_NOSIGNAL);
	if(sendlen==-1){ //error
		printf("error sending\n");			
		close(socket);
		exit(1);
	}
	unsigned char *lineEnd = strrchr(send_buf, '\n');
	if(lineEnd==NULL){
		// printf("no line end in send_msg\n");
	}else{
		*lineEnd = 0;
	}
	if(strcmp(EXIT_REQUEST, send_buf)==0){	//-1 for \n added to msg			
		printf("closing client\n");
		close(socket);
		exit(0);
	}
	printf("send_msg: sent %ld bytes\n", sendlen);
	return sendlen;
}
