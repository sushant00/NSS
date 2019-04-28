//Sushant Kumar Singh
//2016103

//reference:https://www.binarytides.com/tcp-syn-portscan-in-c-with-linux-sockets/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>

#include <pthread.h>

#define SERVER_PORT 12000
#define MSG_LEN 4096

#define PORT_MIN 0
#define PORT_MAX 65535

unsigned short calcCheckSum(unsigned short *addr, size_t len);

// SERVER
int main(int argc, char **args){
	//dont buffer output
	setbuf(stdout, NULL);

	if(argc<2){
		printf("please supply the ip to scan\n");
		return -1;
	}

	printf("starting server...\n");

	int scannerSocket, len, ret, enable=1;
	struct sockaddr_in local_addr, dest_addr;


	scannerSocket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if(scannerSocket==-1){	//error
		fprintf(stderr, "could not create server\n");
		exit(1);
	}
	printf("socket created %d\n", scannerSocket);
	ret = setsockopt(scannerSocket, IPPROTO_IP, IP_HDRINCL, (const void *)&enable, sizeof(enable));
	if(ret==-1){	//error
		fprintf(stderr, "could not set socket options\n");

	}

	//IPv4 local address
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(SERVER_PORT);		//host to network byte(big endian) short
	inet_pton(AF_INET, (const char *)"127.0.0.1", &local_addr.sin_addr);
	
	//IPv4 address
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = 0;		//we would set this later
	inet_pton(AF_INET, (const char *)args[1], &dest_addr.sin_addr);

	printf("scanner ready\n");

	char datagram[MSG_LEN];
	memset(datagram, 0, MSG_LEN);

	struct iphdr *ip_h = (struct iphdr *) datagram;
	struct tcphdr *tcp_h = (struct tcphdr *) (datagram + sizeof(struct ip));

	ip_h->ihl = 5; //ip header len
	ip_h->version = 4;
	ip_h->tos = 0;
	ip_h->tot_len = sizeof(struct iphdr)+sizeof(tcphdr);
	ip_h->id = htons(12345);
	ip_h->frag_off = 0;
	ip_h->ttl = 100;
	ip_h->protocol = IPPROTO_TCP;
	ip_h->check = 0; //skip checksum field for now
	ip_h->saddr = local_addr.sin_addr;
	ip_h->daddr = dest_addr.sin_addr;

	// now calc checksum
	ip_h->check = calcCheckSum( (unsigned short *)datagram, (ip_h->tot_len)/2 );

	//add tcp headers
	tcp_h->source = htons(SERVER_PORT);
	tcp_h->dest = 0; // we would set this later
	tcp_h->seq = htonl(12345);
	tcp_h->ack_seq = 0;
	tcp_h->doff = sizeof(struct tcphdr)/4;
	tcp_h->fin = 0;
	tcp_h->syn = 1;
	tcp_h->rst = 0;
	tcp_h->psh = 0;
	tcp_h->ack = 0
	tcp_h->urg = 0;
	tcp_h->window = htons(MSG_LEN);
	tcp_h->check = 0;
	tcp_h->urg_ptr = 0;


	//scan the tcp responses
	pthread_t scan_thread;
	pthread_create(&scan_thread, NULL, scanResponse, (void *)&scannerSocket);

	printf("starting scan...\n");
	
	while(1){
		int port_min = PORT_MIN;
		int port_max = PORT_MAX;
		if(args[2]!=NULL){
			port_min = atoi(args[2]);
			port_max = atoi(args[2]);
		}
		for(int port_num = port_min; port_num <= port_max; port_num++){
			//set the port number
			tcp_h->dest = htons(port_num);
			tcp_h->check = calcCheckSum( (unsigned short *), sizeof());

			size_t sendlen = sizeof(struct iphdr) + sizeof(struct tcphdr);
			ret = sendto( scannerSocket, datagram, sendlen, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
			if(ret < 0){
				printf("error sending syn to port %d\n",port_num);
				return -1;
			}else{
				printf("sent syn pkt to port %d\n", port_num);
			}
		}

	}
	pthread_join(scan_thread, NULL);
	return 0;
}


void *scanResponse(void *args){

	return;
}


unsigned short calcCheckSum(unsigned short *data, size_t len){
	unsigned int csum = 0; //32 bits

	while(len > 1){
		csum += *data; //summing each 16bits
		data++;
		len-=2;
	}

	if (len > 0){
		//oddbyte
		csum += *(unsigned char *)data;
		len-=1;
	}

	//convert to 16 bits checksum
	csum = (csum >> 16) + (csum & 0xffff);
	csum = csum + (csum >> 16);

	csum = ~csum

	return (unsigned short)csum;
}