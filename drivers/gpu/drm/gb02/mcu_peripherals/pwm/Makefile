CONFIG_MODULE_SIG=n

obj-m := pwm-gb02.o
pwm-gb02-objs := pwm-gb02-mcu.o

KSRC=/lib/modules/$(shell uname -r)/build
#KSRC=/opt/WSL2-Linux-Kernel
PWD=$(shell pwd)


all:
	make -C $(KSRC) M=$(PWD)
	# cd ./pwm_test/
	# cc pwmconfig.c pwmtest.c -o pwmtestapp
clean:
	make -C $(KSRC) M=$(PWD) clean
	rm -rf pwm_test/pwm_test
