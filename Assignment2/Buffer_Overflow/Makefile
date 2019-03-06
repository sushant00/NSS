all: shellcode.s
	gcc -c shellcode.s -o shellcode.o
	ld shellcode.o -o shellcode

clean:
	rm -r shellcode shellcode.o input_to_victim.txt shellcode_hex

run: shellcode
	./shellcode

gen_input: gen_input.sh
	sh gen_input.sh

attack: victim input_to_victim.txt
	./victim < input_to_victim.txt