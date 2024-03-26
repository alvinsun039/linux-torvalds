# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2022 - 2024 Mucse Corporation.

# kbuild part of makefile
#
# Makefile for the Mucse(R) 10GbE PCI Express Linux Network Driver
#
EXTRA_CFLAGS += -I./include/ -I./ -I./base -DCONFIG_RNP_FPGA
#EXTRA_CFLAGS += -Wno-unused-function -Wno-unused-variable -Wno-unused-label
EXTRA_CFLAGS += -g -O3
EXTRA_CFLAGS += $(CFLAGS)

obj-m := rnpgbe.o
rnpgbe-objs :=   \
		rnpgbe_main.o \
		rnpgbe_common.o \
		rnpgbe_debugfs.o \
		rnpgbe_ethtool.o \
		rnpgbe_lib.o \
		rnpgbe_mbx.o \
		rnpgbe_chip.o \
		rnpgbe_mbx_fw.o\
		rnpgbe_sriov.o \
		rnpgbe_param.o \
		rnpgbe_sysfs.o \
		rnpgbe_ptp.o \
		rnpgbe_sfc.o


