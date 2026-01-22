# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2022 - 2025 Mucse Corporation.

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
#
# Makefile for the Mucse(R) 10GbE VF PCI Express Linux Network Driver
#
	EXTRA_CFLAGS += -I./include/ -I./ -I./base  
	#EXTRA_CFLAGS += -Wno-unused-function -Wno-unused-variable -Wno-unused-label
	EXTRA_CFLAGS += -g -O3
	EXTRA_CFLAGS += $(CFLAGS)

	obj-$(CONFIG_MGBEVF) += rnpgbevf.o
	rnpgbevf-objs :=   \
			   vf.o \
			   rnpgbevf_mbx.o \
			   rnpgbevf_ethtool.o \
			   rnpgbevf_sysfs.o \
			   rnpvf_compat.o\
			   rnpgbevf_main.o



endif	# ifneq($(KERNELRELEASE),)

