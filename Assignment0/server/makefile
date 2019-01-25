.PHONY: clean

all: server client
	
server: server_16103.c
	gcc -pthread server_16103.c -o server	
	
client: client_16103.c
	gcc -pthread client_16103.c -o client

runclient: client
	./client

runserver: server
	./server

clean:
	rm server client
