.PHONY: clean

all: fverify fsign fget_decrypt fput_encrypt getacl setacl setattr ls fget fput create_dir do_exec

getacl: getacl.c
	gcc -o getacl getacl.c

setacl: setacl.c
	gcc -o setacl setacl.c

setattr: setattr.c
	gcc -o setattr setattr.c

ls: ls.c
	gcc -o ls ls.c

create_dir: create_dir.c
	gcc -o create_dir create_dir.c

fget: fget.c
	gcc -o fget fget.c

fput: fput.c
	gcc -o fput fput.c

do_exec: do_exec.c
	gcc -o do_exec do_exec.c



fput_encrypt: fput_encrypt.c
	gcc -o fput_encrypt fput_encrypt.c -lcrypto 

fget_decrypt: fget_decrypt.c
	gcc -o fget_decrypt fget_decrypt.c -lcrypto

fsign: fsign.c
	gcc -o fsign fsign.c -lcrypto 

fverify: fverify.c
	gcc -o fverify fverify.c -lcrypto


	
clean:
	rm getacl setacl setattr ls fget fput create_dir do_exec
