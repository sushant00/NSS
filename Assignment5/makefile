.PHONY: clean

all: port_scanner
	
port_scanner: port_scanner.c
	gcc -pthread port_scanner.c -o port_scanner	

run: port_scanner
	./port_scanner

clean:
	rm port_scanner
