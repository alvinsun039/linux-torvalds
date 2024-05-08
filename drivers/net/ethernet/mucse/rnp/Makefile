# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 2022 - 2023 Mucse Corporation.
# kbuild part of makefile
#
# Makefile for the Mucse(R) 10GbE PCI Express Linux Network Driver
#
EXTRA_CFLAGS += -I./include/ -I./ -I./base -DCONFIG_RNP_FPGA
#EXTRA_CFLAGS += -Wno-unused-function -Wno-unused-variable -Wno-unused-label
EXTRA_CFLAGS += -g -O3
EXTRA_CFLAGS += $(CFLAGS)

obj-m := rnp.o
rnp-objs :=   \
		rnp_main.o \
		rnp_common.o \
		rnp_debugfs.o \
		rnp_ethtool.o \
		rnp_lib.o \
		rnp_mbx.o \
		rnp_pcs.o \
		rnp_n10.o \
		rnp_mbx_fw.o\
		rnp_sriov.o \
		rnp_param.o \
		rnp_sysfs.o \
		rnp_dcb.o \
		rnp_ptp.o \
		rnp_mpe.o
