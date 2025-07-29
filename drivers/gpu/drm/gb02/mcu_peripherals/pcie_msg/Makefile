CONFIG_MODULE_SIG=n

obj-m := gb02_pcie_msg.o
gb02_pcie_msg-objs := gb02_pcie_msg.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel
PWD=$(shell pwd)


all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
