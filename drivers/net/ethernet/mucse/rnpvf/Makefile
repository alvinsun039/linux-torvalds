# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2022 - 2024 Mucse Corporation.

# kbuild part of makefile
#
# Makefile for the Mucse(R) 10GbE VF PCI Express Linux Network Driver
#
EXTRA_CFLAGS += -I./include/ -I./ -I./base  
#EXTRA_CFLAGS += -Wno-unused-function -Wno-unused-variable -Wno-unused-label
EXTRA_CFLAGS += -g -O3
EXTRA_CFLAGS += $(CFLAGS)

obj-m := rnpvf.o
rnpvf-objs :=   \
		vf.o \
                mbx.o \
                ethtool.o \
                rnpvf_sysfs.o \
                rnpvf_main.o \
		rnpvf_compat.o 

