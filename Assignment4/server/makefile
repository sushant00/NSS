.PHONY: clean

all: server client
	
server: server_16103.c
	gcc -o server -pthread server_16103.c -lcrypto
	
client: client_16103.c
	gcc -o client -pthread client_16103.c -lcrypto

runclient: client
	./client

runserver: server
	./server

clean:
	rm server client
