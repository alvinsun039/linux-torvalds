/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * jessy 20221019 Sietium
 */

#ifndef __VPU_LIST_H__
#define __VPU_LIST_H__
#include <linux/ioctl.h>    /* needed for the _IOW etc stuff used later */

typedef struct bi_list_node {
	void *data;
	struct bi_list_node *next;
	struct bi_list_node *previous;
} bi_list_node;
typedef struct bi_list {
	bi_list_node *head;
	bi_list_node *tail;
} bi_list;

void GB02FUNC208(bi_list *list);
bi_list_node *GB02FUNC215(void);
void GB02FUNC225(bi_list *list, bi_list_node *current_node);
void GB02FUNC230(bi_list_node *node);
void GB02FUNC219(bi_list *list, bi_list_node *base_node,
	 bi_list_node *new_node);
void GB02FUNC210(bi_list *list, bi_list_node *current_node);
#endif

