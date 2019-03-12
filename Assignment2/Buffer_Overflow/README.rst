SIMPLE BUFFER OVERLOW ATTACK
************************

.. contents:: **Contents of this document**
   :depth: 2


Description
===========
We are given a binary (named 'victim') and we want to perform a buffer overflow attack. We would force the victim to print 'Hello World!' and exit gracefully after it returns from its main function.

Get Started
===========

Step 0
-----------
We would first turn the ASLR off (which is enabled by default on modern systems). We add the line **kernel.randomize_va_space=0** to end of file **/etc/sysctl.conf** and apply the changes by running the following command

.. code:: shell

	sysctl -p

	
Step 1
-----------
compile the assembly code 'shellcode.s', which would be the shellcode executed by victim on returning from main

.. code:: shell

	make

	
Run the shellcode to check if it is working 

.. code:: shell

	make run

	
Step 2
------------
Generate the input for victim program that would be responsible for buffer overflow

.. code:: shell

	sh gen_input.sh
	

Step 3
-------
Finally, exploit the victim program

.. code:: shell

	./victim < input
	


Implementation
==============

shellcode.s
------------
Shell script for printing 'Hello World!' and exiting

gen_input.sh
------------
This shell script uses the xxd command to generate the hex dump in little endian of the shellcode and puts in file 'shellcode_hex'
It then compiles the explot.c file to binary 'exploit'.
It then runs the exploit binary with arguments as 80 -32 20 (buffersize offset nopslen)
**Note:** We cannot have 0a (hex value for newline character) in our shell code, otherwise the shell code would only be copied till the newline. This is because **gets** has been used to copy the shellcode to buffer. Similarly if strcpy was used, we would have to prevent the presence of 00 (hex value for null terminator). We can use **disas main** command inside **gdb** to analyse the same. 


exploit.c
---------
Takes argument <buffersize> <offset> [nopslen]
It creates the input to be passed to victim program of length buffersize. It first pads the input with nopslen (default is buffersize/2) number of nops followed by the shellcode read from 'shellcode_hex' file and finally return address. The return address is picked using current stack pointer - offset. Finally this input is written to file 'input'.






Reference
=========
http://www-inst.eecs.berkeley.edu/~cs161/fa08/papers/stack_smashing.pdf

https://crypto.stanford.edu/~blynn/rop/

http://vadramanienka.blogspot.com/2013/04/how-to-disable-aslr-in-linux-permanently.html

Author: Sushant Kumar Singh

	
