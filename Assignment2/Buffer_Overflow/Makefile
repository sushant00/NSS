all: shellcode.s
	gcc -c shellcode.s -o shellcode.o
	ld shellcode.o -o shellcode

run: shellcode
	./shellcode

clean:
	rm -r shellcode shellcode.o input shellcode_hex exploit
