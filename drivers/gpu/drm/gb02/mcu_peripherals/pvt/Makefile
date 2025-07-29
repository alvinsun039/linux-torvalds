CONFIG_MODULE_SIG=n

obj-m := pvt-gb02.o
pvt-gb02-objs := gb02-pvt.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel
PWD=$(shell pwd)


all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
