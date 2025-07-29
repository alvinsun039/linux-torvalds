// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>

#include "vpu_list.h"
#include "common/gb_common.h"

void GB02FUNC208(bi_list *list)
{
	list->head = NULL;
	list->tail = NULL;
}

void GB02FUNC210(bi_list *list, bi_list_node *current_node)
{
	if (current_node == NULL) {
		gb_printf(KERN_ERR, "%s: insert node tail  NULL\n", __func__);
		return;
	}
	if (list->tail) {
		current_node->previous = list->tail;
		list->tail->next = current_node;
		list->tail = current_node;
		list->tail->next = NULL;
	} else {
		list->head = current_node;
		list->tail = current_node;
		current_node->next = NULL;
		current_node->previous = NULL;
	}
}

bi_list_node *GB02FUNC215(void)
{
	bi_list_node *node = NULL;

	node = (bi_list_node *)vmalloc(sizeof(bi_list_node));
	if (node == NULL) {
		gb_printf(KERN_ERR, "%s: error vmalloc for node fail!\n", __func__);
		return node;
	}
	memset(node, 0, sizeof(bi_list_node));
	return node;
}

void GB02FUNC219(bi_list *list, bi_list_node *base_node,
	bi_list_node *new_node)
{
	bi_list_node *temp_node_previous = NULL;

	if (new_node == NULL) {
		gb_printf(KERN_ERR, "%s: insert node before new node NULL\n", __func__);
		return;
	}
	if (base_node) {
		if (base_node->previous) {
			//at middle position
			temp_node_previous = base_node->previous;
			temp_node_previous->next = new_node;
			new_node->next = base_node;
			base_node->previous = new_node;
			new_node->previous = temp_node_previous;
		} else {
			//at head
			base_node->previous = new_node;
			new_node->next = base_node;
			list->head = new_node;
			new_node->previous = NULL;
		}
	} else {
		//at tail
		GB02FUNC210(list, new_node);
	}
}

void GB02FUNC225(bi_list *list, bi_list_node *current_node)
{
	bi_list_node *temp_node_previous = NULL;
	bi_list_node *temp_node_next = NULL;

	if (current_node == NULL) {
		gb_printf(KERN_ERR, "%s: remove node NULL\n", __func__);
		return;
	}
	temp_node_next = current_node->next;
	temp_node_previous = current_node->previous;

	if (temp_node_next == NULL && temp_node_previous == NULL) {
		//there is only one node.
		list->head = NULL;
		list->tail = NULL;
	} else if (temp_node_next == NULL) {
		//at tail
		list->tail = temp_node_previous;
		temp_node_previous->next = NULL;
	} else if (temp_node_previous == NULL) {
		//at head
		list->head = temp_node_next;
		temp_node_next->previous = NULL;
	} else {
		//at middle position
		temp_node_previous->next = temp_node_next;
		temp_node_next->previous = temp_node_previous;
	}
}

void GB02FUNC230(bi_list_node *node)
{
	//free current node
	vfree(node);
}


