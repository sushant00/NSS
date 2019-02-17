obj-m = nfmod.o

all: nfmod.c
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(shell pwd) clean

install:
	insmod nfmod.ko

remove:
	rmmod nfmod.ko
