CONFIG_MODULE_SIG=n

obj-m := gb02_fan.o
gb02-fan-objs := gb02_fan.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel
PWD=$(shell pwd)


all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
