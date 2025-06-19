CONFIG_MODULE_SIG=n

obj-m := i2c-gb02.o
i2c-gb02-objs := i2c-gb02-mcu.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel/
PWD=$(shell pwd)

all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
	rm -f usr/test
