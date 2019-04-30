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

  ./port_scanner <-S/-F> <target ip> [port]

Please type in the ip of target machine in target_ip field.
-S is for syn scan, -F is for fin scan.
If port is set then only that port is checked


Design
======
- This Scanner supports the SYN Scan and FIN Scan, which work similar to NMAP
- SYN Scan: packets with SYN bit are send to all ports of target. In response if the SYN and the ACK bits are set then, it is assumed that the port is opened.
- FIN Scan: packets with FIN bit are send to all ports of target. In response if the RST and the ACK bits are set then, it is assumed that the port is closed.


Assumptions
============

- The scanner should be stopped using SIG_INT, otherwise it keeps on flooding the target ip
- The scanner by default scans all the port numbers of the given ip


Developed by Sushant Kumar Singh