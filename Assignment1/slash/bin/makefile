.PHONY: clean

all: getacl setacl setattr ls fget

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
	
clean:
	rm getacl setacl setattr ls fget
