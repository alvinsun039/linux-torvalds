// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/module.h>

unsigned int rnp_loglevel;
module_param(rnp_loglevel, uint, S_IRUSR | S_IWUSR);


