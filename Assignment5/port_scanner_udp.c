//Sushant Kumar Singh
//2016103

//reference:https://www.binarytides.com/udp-syn-portscan-in-c-with-linux-sockets/
//https://nmap.org/book/scan-methods-udp-scan.html
//https://en.wikipedia.org/wiki/User_Datagram_Protocol#IPv4_Pseudo_Header
//https://stackoverflow.com/questions/27164271/how-to-convert-sock-dgram-to-sock-raw
//https://www.tenouk.com/Module43a.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

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
	unsigned short udp_len;

	struct udphdr udp_h;
};

struct sockaddr_in target_addr;

int icmp_reply;

// SERVER
int main(int argc, char **args){
	//dont buffer output
	setbuf(stdout, NULL);

	if(argc<2){
		printf("please supply ip to scan\n");
		return -1;
	}
	printf("starting server...\n");

	int scannerSocket, len, ret, enable=1;
	struct sockaddr_in local_addr, dst_addr;


	scannerSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
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
	inet_pton(AF_INET, (const char *)args[1], &dst_addr.sin_addr);

	target_addr = dst_addr;
	
	printf("scanner ready\n");

	//scan the udp responses
	pthread_t scan_thread;
	pthread_create(&scan_thread, NULL, scanResponse, (void *)&scannerSocket);

	sleep(2);

	char datagram[MSG_LEN];
	memset(datagram, 0, MSG_LEN);

	struct iphdr *ip_h = (struct iphdr *) datagram;
	struct udphdr *udp_h = (struct udphdr *) (datagram + sizeof(struct iphdr));
	struct pseudo_header ps_h;

	ip_h->ihl = 5; //ip header len
	ip_h->version = 4;
	ip_h->tos = 16;
	ip_h->tot_len = sizeof(struct iphdr)+sizeof(struct udphdr);
	ip_h->id = htons(12345);
	ip_h->frag_off = 0;
	ip_h->ttl = 100;
	ip_h->protocol = IPPROTO_UDP;
	ip_h->check = 0; //skip checksum field for now
	ip_h->saddr = local_addr.sin_addr.s_addr;
	ip_h->daddr = dst_addr.sin_addr.s_addr;

	//add udphdr headers
	udp_h->source = htons(SERVER_PORT);
	udp_h->dest = 0; // we would set this later
	udp_h->len = htons(sizeof(struct udphdr));
	udp_h->check = 0;
	
	// now calc checksum
	ip_h->check = calcCheckSum( (unsigned short *)datagram, (ip_h->tot_len) );

	printf("starting scan...\n");
	
	int port_min = PORT_MIN;
	int port_max = PORT_MAX;
	printf("checking port specs\n");
	if(args[2]!=NULL){
		port_min = atoi(args[2]);
		port_max = atoi(args[2]);
	}
	printf("looping ports\n");
	for(int port_num = port_min; port_num <= port_max; port_num++){
		icmp_reply = 0;
		//set the port number
		// printf("sending pkt to port %d\n", port_num);
		udp_h->dest = htons(port_num);
		udp_h->check = 0; //important as this is inside a loop, otherwise wrong checksum calculated
		// printf("adding source addr to ps_h %d to %d\n", local_addr.sin_addr.s_addr, ps_h.src_addr);
		ps_h.src_addr = local_addr.sin_addr.s_addr;
		// printf("added source addr to ps_h\n");

		// printf("adding dest addr to ps_h\n");
		ps_h.dst_addr = dst_addr.sin_addr.s_addr;
		// printf("added dest addr to ps_h\n");

		ps_h.reserved = 0;
		ps_h.protocol = IPPROTO_UDP;
		// printf("adding udplen to ps_h\n");
		ps_h.udp_len = htons(sizeof(struct udphdr));

		// printf("calculating checksum\n");
		memcpy(&ps_h.udp_h, udp_h, sizeof(struct udphdr));
		// udp_h->check = calcCheckSum( (unsigned short *)&ps_h, sizeof(struct pseudo_header));

		size_t sendlen = sizeof(struct iphdr) + sizeof(struct udphdr);
		ret = sendto( scannerSocket, datagram, sendlen, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr));
		if(ret < 0){
			printf("error sending to port %d\n",port_num);
			return -1;
		}else{
			printf("sent pkt to port %d\n", port_num);
		}
		sleep(1);
		if( icmp_reply == 1){
			printf("scanResponse: Port %d/udp closed\n", port_num);
		}else{
			printf("scanResponse: Port %d/udp open | filered\n", port_num);
		}
	}

	pthread_join(scan_thread, NULL);
	return 0;
}


void *scanResponse(void *args){
	printf("scanResponse: starting receiver...\n");

	int recvSocket, len, ret, enable=1;
	struct sockaddr from_addr;


	recvSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
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
		// printf("scanResponse: receiving...\n");
		recvlen = recvfrom(recvSocket, recv_buf, MSG_LEN, 0, &from_addr, &from_addr_size);
		// printf("scanResponse: received a response\n");
		if(recvlen<0){ //error
			printf("scanResponse: error receiving\n");
			exit(1);			
		}

		struct iphdr *ip_h;
		struct icmphdr *icmp_h;
		struct sockaddr_in src_addr, dst_addr;
		memset(&src_addr, 0, sizeof(struct sockaddr_in));
		memset(&dst_addr, 0, sizeof(struct sockaddr_in));

		ip_h = (struct iphdr *)recv_buf;

		if(ip_h->protocol == IPPROTO_ICMP){
			icmp_h = (struct icmphdr *)( recv_buf + (ip_h->ihl)*4 );
			src_addr.sin_addr.s_addr = ip_h->saddr;
			dst_addr.sin_addr.s_addr = ip_h->daddr;

			//Syn and Ack are set means port is open
			if( (src_addr.sin_addr.s_addr == target_addr.sin_addr.s_addr) ){
				if ( (icmp_h->type == ICMP_DEST_UNREACH) && (icmp_h->code == ICMP_PORT_UNREACH) ) {
					icmp_reply = 1;
				}
				// else{
				// 	printf("scanResponse: Port %d syn %d, ack %d fin %d\n", ntohs(udp_h->source), udp_h->syn, udp_h->ack, udp_h->fin);
				// }
			}else{
				printf("scanResponse: target ip did not match\n");
			}
		}else{
			printf("scanResponse: not a icmp protocol packet\n");
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