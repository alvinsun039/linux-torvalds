CONFIG_MODULE_SIG=n

obj-m := pinctrl-gb02.o
pinctrl-gb02-objs := pinctrl-gb02-mcu.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel/
PWD=$(shell pwd)

all:
	make -C $(KSRC) M=$(PWD) modules
clean:
	make -C $(KSRC) M=$(PWD) clean
	rm -f usr/test
