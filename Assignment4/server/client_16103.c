//Sushant Kumar Singh
//2016103

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
int send_msg(int socket, unsigned char *msg, size_t msglen);
int receive_msg(int socket, unsigned char *msg, size_t msglen);

// CLIENT
int main(void){
	//dont buffer the output
	setbuf(stdout, NULL);

	size_t serverNameLen = sizeof(unsigned char)*COMMAND_LEN;
	unsigned char *serverName = "127.0.0.1";

	printf("starting client...\n");

	int clientSocket, len, ret, enable=1;
	struct sockaddr_in server_addr;


	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket==-1){	//error
		fprintf(stderr, "could not create client\n");
		exit(1);
	}
	ret = setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, (const void *)&enable, sizeof(enable));
	if(ret==-1){	//error
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

	//connect with server
	len = sizeof(server_addr);
	ret = connect(clientSocket, (const struct sockaddr *)&server_addr, (socklen_t)len);
	if(ret==-1){//error
		fprintf(stderr, "could not connect\n");
		exit(1);
	}

	printf("enter your %d digit uid\n", UID_LEN);
	fgets(msg, UID_LEN, stdin);
	int clientUID = atoi(msg);
	strcpy(msg+UID_LEN, UID_CHAT_SERVER);
	int nonce = rand()%(900000) + 1000000;
	sprintf(msg+UID_LEN+UID_LEN, "%d", nonce);
	printf("msg 1 to kdc:%s", msg);
	sendlen = send_msg(clientSocket, msg, msglen);

	//receive the ticket
	recvlen = receive_msg(clientSocket, msg, msglen);
	unsigned char *ciphertext = malloc(sizeof(unsigned char)*recvlen + KEY_LEN_BITS );
	int cipherLen = cipher(msg, strlen(msg), ciphertext, 0, clientUID);

	unsigned char *ticket;
	int ticket_len;
	unsigned char *sharedKey = malloc(KEY_LEN_BITS/(sizeof(unsigned char)*8));

	unsigned char tmp = msg[6];
	msg[6] = 0;
	if(atoi(msg) == nonce){
		printf("KDC response's nonce match\n");
		msg[6] = tmp;
		if(strncmp(msg+6, UID_CHAT_SERVER, UID_LEN)) {
			printf("KDC sent shared key for chat server\n");

			strncpy(sharedKey, msg+6+UID_LEN, KEY_LEN_BITS/(sizeof(unsigned char)*8));
			ticket_len = recvlen - 6 - UID_LEN - (KEY_LEN_BITS/(sizeof(unsigned char)*8));
			ticket = malloc( ticket_len );
			strncpy(ticket, msg+6+UID_LEN+(KEY_LEN_BITS/(sizeof(unsigned char)*8)), ticket_len );
		}else{
			printf("KDC sent shared key for unknown client\n");
			return -1;
		}
	}else{
		printf("KDC response's nonce match failed\n");
		return -1;
	}


	//connect to server
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);		//host to network byte(big endian) short
	inet_pton(AF_INET, (const unsigned char *)serverName, &server_addr.sin_addr);

	//connect with server
	len = sizeof(server_addr);
	ret = connect(clientSocket, (const struct sockaddr *)&server_addr, (socklen_t)len);
	if(ret==-1){	//error
		fprintf(stderr, "could not connect\n");
		exit(1);
	}

	strncpy(msg, ticket, ticket_len);
	msglen = ticket_len;
	sendlen = send_msg(clientSocket, msg, msglen);

	printf("client connected to server.\nType msg (max %d chars) and press enter. Type '%s' to close the client.\n", COMMAND_LEN, EXIT_REQUEST);

	//handle the client in new thread
	pthread_t receiver_thread;
	pthread_create(&receiver_thread, NULL, receiver, (void *)&clientSocket);

	//send msgs
	while(1){		
		//take msg line input
		while(1){
			fgets(msg, msglen, stdin);
			unsigned char *newline = strchr(msg, '\n');
			// msg[(int)(newline - msg)] = '\0';
			if(strlen(msg) != 0){
				break;
			}
		}
		sendlen = send_msg(clientSocket, msg, msglen);
	}
	close(clientSocket);
	return 0;
}

//receiver for a client
void* receiver(void *args){
	int clientSocket = *((int *)args);
	size_t msglen = MSG_LEN*sizeof(unsigned char);
	unsigned char *msg = malloc(msglen);
	size_t recvlen;
	while(1){
		//receive the msg
		recvlen = receive_msg(clientSocket, msg, msglen);
	}
}

int receive_msg(int socket, unsigned char *msg, size_t msglen){
	size_t recvlen = recv(socket, (void *)msg, msglen, 0);
	if(recvlen==-1){ //error
		printf("error receiving\n");
		return -1;			
	}else if(recvlen==0){
		printf("server disconnected abruptly\n");
		close(socket);
		exit(0);	

	}else{
		if(strcmp(EXIT_REQUEST, msg)==0){				
			printf("server disconnecting\n");
			close(socket);
			exit(0);
		}
		printf("%s", msg);
	}
	return recvlen;
}



int send_msg(int socket, unsigned char *msg, size_t msglen){
	size_t sendlen = send(socket, (void *)msg, msglen, MSG_NOSIGNAL);
	if(sendlen==-1){ //error
		printf("error sending\n");			
		close(socket);
		exit(1);
	}
	*strrchr(msg, '\n') = 0;
	if(strcmp(EXIT_REQUEST, msg)==0){	//-1 for \n added to msg			
		printf("closing client\n");
		close(socket);
		exit(0);
	}
	return sendlen;
}
