//Sushant Kumar Singh
//2016103

//reference:https://www.binarytides.com/tcp-syn-portscan-in-c-with-linux-sockets/
//https://www.quora.com/What-is-TCP-checksum

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

#define PORT_MIN 5540
#define PORT_MAX 5560

unsigned short calcCheckSum(unsigned short *addr, size_t len);
void *scanResponse(void *args);

//for the checksum calculation
struct pseudo_header{
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned char reserved;
	unsigned char protocol;
	unsigned short tcp_len;

	struct tcphdr tcp_h;
};

int syn_scan;
struct sockaddr_in target_addr;

// SERVER
int main(int argc, char **args){
	//dont buffer output
	setbuf(stdout, NULL);

	if(argc<3){
		printf("please supply the type of scan and ip to scan\n");
		return -1;
	}
	if(strcmp(args[1], "-S")==0){
		printf("doing syn scan\n");
		syn_scan = 1;
	}else{
		printf("doing fin scan\n");
		syn_scan = 0;
	}

	printf("starting server...\n");

	int scannerSocket, len, ret, enable=1;
	struct sockaddr_in local_addr, dst_addr;


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
	inet_pton(AF_INET, (const char *)"10.0.0.4", &local_addr.sin_addr);
	
	//IPv4 address
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = 0;		//we would set this later
	inet_pton(AF_INET, (const char *)args[2], &dst_addr.sin_addr);

	target_addr = dst_addr;
	
	printf("scanner ready\n");

	//scan the tcp responses
	pthread_t scan_thread;
	pthread_create(&scan_thread, NULL, scanResponse, (void *)&scannerSocket);

	char datagram[MSG_LEN];
	memset(datagram, 0, MSG_LEN);

	struct iphdr *ip_h = (struct iphdr *) datagram;
	struct tcphdr *tcp_h = (struct tcphdr *) (datagram + sizeof(struct ip));
	struct pseudo_header ps_h;

	ip_h->ihl = 5; //ip header len
	ip_h->version = 4;
	ip_h->tos = 0;
	ip_h->tot_len = sizeof(struct iphdr)+sizeof(struct tcphdr);
	ip_h->id = htons(12345);
	ip_h->frag_off = 0;
	ip_h->ttl = 100;
	ip_h->protocol = IPPROTO_TCP;
	ip_h->check = 0; //skip checksum field for now
	ip_h->saddr = local_addr.sin_addr.s_addr;
	ip_h->daddr = dst_addr.sin_addr.s_addr;

	// now calc checksum
	ip_h->check = calcCheckSum( (unsigned short *)datagram, (ip_h->tot_len)/2 );

	//add tcp headers
	tcp_h->source = htons(SERVER_PORT);
	tcp_h->dest = 0; // we would set this later
	tcp_h->seq = htonl(12345);
	tcp_h->ack_seq = 0;
	tcp_h->doff = sizeof(struct tcphdr)/4;
	if(syn_scan){
		tcp_h->fin = 0;
	}else{
		printf("setting fin\n");
		tcp_h->fin = 1;
	}
	if(syn_scan){
		printf("setting syn\n");
		tcp_h->syn = 1;
	}else{
		tcp_h->syn = 0;
	}
	tcp_h->rst = 0;
	tcp_h->psh = 0;
	tcp_h->ack = 0;
	tcp_h->urg = 0;
	tcp_h->window = htons(MSG_LEN);
	tcp_h->check = 0;
	tcp_h->urg_ptr = 0;

	printf("starting scan...\n");
	
	// while(1){
		int port_min = PORT_MIN;
		int port_max = PORT_MAX;
		printf("checking port specs\n");
		if(args[3]!=NULL){
			port_min = atoi(args[3]);
			port_max = atoi(args[3]);
		}
		printf("looping ports\n");
		for(int port_num = port_min; port_num <= port_max; port_num++){
			//set the port number
			// printf("sending pkt to port %d\n", port_num);
			tcp_h->dest = htons(port_num);
			
			// printf("adding source addr to ps_h %d to %d\n", local_addr.sin_addr.s_addr, ps_h.src_addr);
			ps_h.src_addr = local_addr.sin_addr.s_addr;
			// printf("added source addr to ps_h\n");

			// printf("adding dest addr to ps_h\n");
			ps_h.dst_addr = dst_addr.sin_addr.s_addr;
			// printf("added dest addr to ps_h\n");

			ps_h.reserved = 0;
			ps_h.protocol = IPPROTO_TCP;
			// printf("adding tcplen to ps_h\n");
			ps_h.tcp_len = htons( sizeof(struct tcphdr) );

			// printf("calculating checksum\n");
			memcpy(&ps_h.tcp_h, tcp_h, sizeof(struct tcphdr));
			tcp_h->check = calcCheckSum( (unsigned short *)&ps_h, sizeof(struct pseudo_header));

			size_t sendlen = sizeof(struct iphdr) + sizeof(struct tcphdr);
			ret = sendto( scannerSocket, datagram, sendlen, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
			if(ret < 0){
				printf("error sending to port %d\n",port_num);
				return -1;
			}else{
				printf("sent pkt to port %d\n", port_num);
			}
		// }

	}
	pthread_join(scan_thread, NULL);
	return 0;
}


void *scanResponse(void *args){
	printf("scanResponse: starting receiver...\n");

	int recvSocket, len, ret, enable=1;
	struct sockaddr from_addr;


	recvSocket = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if(recvSocket==-1){	//error
		fprintf(stderr, "scanResponse: could not create receiver\n");
		exit(1);
	}
	printf("scanResponse: socket created %d\n", recvSocket);

	unsigned char *recv_buf = malloc(MSG_LEN);
	size_t recvlen;
	int from_addr_size;
	//keep receiving the incoming reponses and scan for useful info
	while(1){
		printf("scanResponse: receiving...\n");
		recvlen = recvfrom(recvSocket, recv_buf, MSG_LEN, 0, &from_addr, &from_addr_size);
		printf("scanResponse: received a response\n");
		if(recvlen==-1){ //error
			printf("scanResponse: error receiving\n");
			exit(1);			
		}

		struct iphdr *ip_h;
		struct tcphdr *tcp_h;
		struct sockaddr_in src_addr, dst_addr;
		memset(&src_addr, 0, sizeof(struct sockaddr_in));
		memset(&dst_addr, 0, sizeof(struct sockaddr_in));

		ip_h = (struct iphdr *)recv_buf;

		if(ip_h->protocol == IPPROTO_TCP){
			tcp_h = (struct tcphdr *)( recv_buf + (ip_h->ihl)*4 );
			src_addr.sin_addr.s_addr = ip_h->saddr;
			dst_addr.sin_addr.s_addr = ip_h->daddr;

			//Syn and Ack are set means port is open
			if( (src_addr.sin_addr.s_addr == target_addr.sin_addr.s_addr) ){
				if ( syn_scan && (tcp_h->syn == 1) && (tcp_h->ack == 1) ) {
					printf("scanResponse: Port %d/tcp open|filtered\n", ntohs(tcp_h->source));
				}else if( !syn_scan && (tcp_h->rst == 1) && (tcp_h->ack == 1) ){
					printf("scanResponse: Port %d/tcp closed|filtered\n", ntohs(tcp_h->source));
				}
			}else{
				printf("scanResponse: target ip did not match\n");
			}
		}else{
			printf("scanResponse: not a tcp protocol packet\n");
		}

	}
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
	while( csum >> 16){
		csum = (csum >> 16) + (csum & 0xffff);
	}

	csum = ~csum;

	return (unsigned short)csum;
}