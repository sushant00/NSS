obj-m += nfmod.o

all: nfmod.c
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

install:
	insmod nfmod.ko

remove:
	insmod nfmod.ko