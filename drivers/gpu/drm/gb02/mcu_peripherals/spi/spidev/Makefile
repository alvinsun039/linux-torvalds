CONFIG_MODULE_SIG=n

obj-m := spidev-gb02.o
spidev-gb02-objs := spidev.o

KSRC=/lib/modules/$(shell uname -r)/build

PWD=$(shell pwd)
all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
