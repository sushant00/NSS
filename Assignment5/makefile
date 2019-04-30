.PHONY: clean

all: port_scanner port_scanner_udp
	
port_scanner: port_scanner.c
	gcc -pthread port_scanner.c -o port_scanner	

port_scanner_udp: port_scanner_udp.c	
	gcc -pthread port_scanner_udp.c -o port_scanner_udp	

run: port_scanner
	./port_scanner

clean:
	rm port_scanner port_scanner_udp
