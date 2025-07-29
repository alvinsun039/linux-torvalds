# CONFIG_MODULE_SIG=n

obj-m += pwm-bl.o
pwm-bl-objs := gb02-pwm-bl.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel
PWD=$(shell pwd)
all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
