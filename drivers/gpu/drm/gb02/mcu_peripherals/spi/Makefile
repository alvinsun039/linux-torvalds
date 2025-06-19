# CONFIG_MODULE_SIG=n

obj-m += spi-gb02.o
spi-gb02-objs := spi-gb02-mcu.o

KSRC=/lib/modules/$(shell uname -r)/build
PWD=$(shell pwd)
all:
	make -C $(KSRC) M=$(PWD)
clean:
	make -C $(KSRC) M=$(PWD) clean
