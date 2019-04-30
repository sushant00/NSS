Simple Port Scanner
**********

**This is a simple port scanner using SOCK_RAW sockets.**


.. contents:: **Contents of this document**
   :depth: 2


Get Started
===========

Compiling Scanner Programs
--------------------------
Run the following command on shell to compile the scanner

.. code:: shell

  make
  
  
Starting Port Scanner
---------------
Step 1.
Run the following to start the scanner.


.. code:: shell

  sudo ./port_scanner <-S/-F> <target ip> [port]

Please type in the ip of target machine in target_ip field.
-S is for syn scan, -F is for fin scan.
If port is set then only that port is checked

For udp scanning the following command is used:

.. code:: shell

  sudo ./port_scanner_udp <target ip> [port]


Design
======
- This Scanner supports the SYN Scan and FIN Scan, which work similar to NMAP
- SYN Scan: packets with SYN bit are send to all ports of target. In response if the SYN and the ACK bits are set then, it is assumed that the port is opened.
- FIN Scan: packets with FIN bit are send to all ports of target. In response if the RST and the ACK bits are set then, it is assumed that the port is closed.
- for udp port scanning it is checked that on sending a packet to port if ICMP type 3 code 3 message of host unreachable in received, in which case we mark the port as closed 


Assumptions
============

- The scanner only sends one syn pkt to ports
- The scanner by default scans all the port numbers of the given ip
- fin scanner report closed ports, others are assumed to be open
- syn scanner report open ports, others are assumed to be closed

Commands Used
=============
-sudo nmap -sX 10.0.0.6
-sudo nc -lv 5555
-sudo nc -luv 5555

Developed by Sushant Kumar Singh