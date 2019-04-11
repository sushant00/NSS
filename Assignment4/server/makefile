.PHONY: clean

all: server client
	
server: server_16103.c utils.c
	sudo gcc -o server -pthread server_16103.c -lcrypto
	sudo chmod u+s server
	
client: client_16103.c utils.c
	sudo gcc -o client -pthread client_16103.c -lcrypto
	sudo chmod u+s client

runclient: client client_16103.c utils.c
	./client

runserver: server server_16103.c utils.c
	./server

clean:
	sudo rm server client
