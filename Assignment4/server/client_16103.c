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

#define KDC_PORT 5555
#define SERVER_PORT 12000
#define EXIT_REQUEST "exit"
#define MSG_LEN 2024
#define COMMAND_LEN 1024

// const char *COMMANDS[5] = {"ls", "fput", "fget", "cd", "create_dir", "exit"};

void* receive(void *args);

// CLIENT
int main(void){
	//dont buffer the output
	setbuf(stdout, NULL);

	size_t serverNameLen = sizeof(char)*COMMAND_LEN;
	char *serverName = "127.0.0.1";

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

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);		//host to network byte(big endian) short
	inet_pton(AF_INET, (const char *)serverName, &server_addr.sin_addr);

	//connect with server
	len = sizeof(server_addr);
	ret = connect(clientSocket, (const struct sockaddr *)&server_addr, (socklen_t)len);
	if(ret==-1){	//error
		fprintf(stderr, "could not connect\n");
		exit(1);
	}

	printf("client connected to server.\nType command (max %d chars) and press enter. Type '%s' to close the client.\n", COMMAND_LEN, EXIT_REQUEST);

	//handle the client in new thread
	pthread_t receiver_thread;
	pthread_create(&receiver_thread, NULL, receive, (void *)&clientSocket);

	//send msgs
	while(1){		
		size_t commandlen = COMMAND_LEN*sizeof(char), sendlen;
		char *command = malloc(commandlen);
		
		//take msg line input
		while(1){
			fgets(command, commandlen, stdin);
			char *newline = strchr(command, '\n');
			// command[(int)(newline - command)] = '\0';
			if(strlen(command) != 0){
				break;
			}
		}


		sendlen = send(clientSocket, (void *)command, commandlen, MSG_NOSIGNAL);
		if(sendlen==-1){ //error
			printf("error sending\n");			
			close(clientSocket);
			exit(1);
		}
		*strrchr(command, '\n') = 0;
		if(strcmp(EXIT_REQUEST, command)==0){	//-1 for \n added to command			
			printf("closing client\n");
			close(clientSocket);
			exit(0);
		}
	}
	close(clientSocket);
	return 0;
}

//receiver for a client
void* receive(void *args){
	int clientSocket = *((int *)args);
	size_t msglen = MSG_LEN*sizeof(char);
	char *msg = malloc(msglen);
	size_t recvlen;
	while(1){
		//receive the msg
		recvlen = recv(clientSocket, (void *)msg, msglen, 0);
		if(recvlen==-1){ //error
			printf("error receiving\n");			
		}else if(recvlen==0){
			printf("server disconnected abruptly\n");
			close(clientSocket);
			exit(0);	

		}else{
			if(strcmp(EXIT_REQUEST, msg)==0){				
				printf("server disconnecting\n");
				close(clientSocket);
				exit(0);
			}
			printf("%s", msg);
		}
	}
}
