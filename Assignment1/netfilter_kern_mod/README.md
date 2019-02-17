**NET FILTER KERNEL MODULE**
Sushant Kumar Singh 	2016103

Description:
	This kernel module captures the packet and identifies the type of TCP reconnaissance performed. 
	Following type of TCP reconnaissance are detected:
	1. Xmas Scan
	2. Null Scan
	3. Syn Scan
	4. Ack Scan
	5. Fin Scan
	
Implementation:
	Init Phase (called when the kernel module is initialised):

Compilation :

	make
	
Install :

	sudo make install
	
Remove :

	sudo make remove
	
To see the live kernel log:

	sudo cat /proc/kmsg
	
Examples to perform various nmap reconnaissance:

	sudo nmap -sX 20.0.0.5
	sudo nmap -sS 20.0.0.5
	sudo nmap -sN 20.0.0.5
	
	
