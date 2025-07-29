CONFIG_MODULE_SIG=n

obj-m := gpu_freq.o
gpu_freq-objs := gpu_freq.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel
PWD=$(shell pwd)


all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
