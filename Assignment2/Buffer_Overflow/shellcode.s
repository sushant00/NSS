.global _start
.text
_start:
	jmp callback;
shellcode:
	pop %rsi;
	xor %rax, %rax;
	movb $1, %al;
	xor %rdi, %rdi;
	movb $1, %dil;
	xor %rdx, %rdx;
	movb $12, %dl;
	syscall;
	movb $60, %al;
	xor %rdi, %rdi;
	syscall;
callback:
	call shellcode;
	.ascii "Hello World!";
