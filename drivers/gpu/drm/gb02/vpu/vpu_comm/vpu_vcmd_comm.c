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
#include <linux/cpumask.h>
#include <asm/irq.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "vpu_comm.h"
#include "vpu_list.h"
#include "vpu/vpu_dec/vpu_vcmd.h"

/* process manager object management*/
static struct GB02STR17 *GB02FUNC237(void)
{
	struct GB02STR17 *GB02STR17 = NULL;
	GB02STR17 = vmalloc(sizeof(struct GB02STR17));
	if (GB02STR17 == NULL) {
		gb_printf(KERN_ERR, "%s: vmalloc for GB02STR17 fail!\n",
		 __func__);
		return GB02STR17;
	}
	memset(GB02STR17, 0, sizeof(struct GB02STR17));
	return GB02STR17;
}

static void GB02FUNC239(struct GB02STR17 *GB02STR17)
{
	if (GB02STR17 == NULL) {
		gb_printf(KERN_ERR, "%s: GB02STR17 = NULL, return\n", __func__);
		return;
	}
	vfree(GB02STR17);	//free current GB02STR18
}

bi_list_node *GB02FUNC243(void)
{
	bi_list_node *current_node = NULL;
	struct GB02STR17 *GB02STR17 = NULL;

	GB02STR17 = GB02FUNC237();
	if (GB02STR17 == NULL) {
		gb_printf(KERN_ERR, "%s: error GB02STR17 == NULL!\n", __func__);
		return NULL;
	}
	GB02STR17->total_exe_time = 0;
	spin_lock_init(&GB02STR17->spinlock);
	init_waitqueue_head(&GB02STR17->wait_queue);
	current_node = GB02FUNC215();
	if (current_node == NULL) {
		gb_printf(KERN_ERR, "%s: GB02FUNC215 fail!", __func__);
		GB02FUNC239(GB02STR17);
		return NULL;
	}
	current_node->data = (void *)GB02STR17;
	return current_node;
}

void GB02FUNC247(bi_list *global_process_manager)
{
	bi_list_node *node;
	struct GB02STR17 *GB02STR17 = NULL;

	node = GB02FUNC243();
	GB02STR17 = (struct GB02STR17 *)node->data;
	GB02STR17->filp = NULL;
	GB02FUNC210(global_process_manager, node);
}

/* cmdbuf object management */
struct GB02STR18 *GB02FUNC249(void)
{
	struct GB02STR18 *GB02STR18 = NULL;

	GB02STR18 = vmalloc(sizeof(struct GB02STR18));
	if (GB02STR18 == NULL) {
		gb_printf(KERN_ERR, "%s: vmalloc GB02STR18 fail GB02STR18 = NULL\n",
		 __func__);
		return GB02STR18;
	}
	memset(GB02STR18, 0, sizeof(struct GB02STR18));
	return GB02STR18;
}

void GB02FUNC254(struct GB02STR18 *GB02STR18)
{
	if (GB02STR18 == NULL) {
		gb_printf(KERN_ERR, "%s: remove_cmdbuf_obj GB02STR18 == NULL\n", __func__);
		return;
	}
	vfree(GB02STR18);	/* free current GB02STR18 */
}

int GB02FUNC256(struct GB02STR17 *manager_obj)
{
	return manager_obj->total_exe_time <= GB02MAC163;
}

bi_list_node *GB02FUNC260(bi_list_node *current_node)
{
	bi_list_node *new_cmdbuf_node = current_node;
	bi_list_node *last_cmdbuf_node;
	struct GB02STR18 *GB02STR18 = NULL;

	if (current_node == NULL)
		return NULL;
	last_cmdbuf_node = new_cmdbuf_node;
	new_cmdbuf_node = new_cmdbuf_node->previous;
	while (1) {
		if (new_cmdbuf_node == NULL)
			return last_cmdbuf_node;
		GB02STR18 = (struct GB02STR18 *)new_cmdbuf_node->data;
		if (GB02STR18->cmdbuf_data_linked)
			return new_cmdbuf_node;

		last_cmdbuf_node = new_cmdbuf_node;
		new_cmdbuf_node = new_cmdbuf_node->previous;
	}
	return NULL;
}

unsigned long long GetMMUAddress(void)
{
	unsigned long long address = 0;

	address = GB02MAC165;
	return address;
}

void GB02FUNC264(bi_list_node *process_node)
{
	struct GB02STR17 *GB02STR17 = NULL;

	if (process_node == NULL) {
		gb_printf(KERN_ERR, "%s: process_node = NULL, so return\n", __func__);
		return;
	}
	GB02STR17 = (struct GB02STR17 *)process_node->data;
	/* free struct GB02STR17 */
	GB02FUNC239(GB02STR17);
	/* free current GB02STR17 entity. */
	GB02FUNC230(process_node);
}

/* just remove, not free the node. */
bi_list_node *GB02FUNC176(bi_list *list,
	bi_list_node *cmdbuf_node)
{
	struct GB02STR18 *GB02STR18 = NULL;

	if (cmdbuf_node == NULL) {
		gb_printf(KERN_ERR, "%s: error remove_cmdbuf = NULL, return\n", __func__);
		return NULL;
	}
	if (cmdbuf_node->next) {
		GB02STR18 = (struct GB02STR18 *)cmdbuf_node->next->data;
		if (GB02STR18->cmdbuf_need_remove == 1) {
			GB02FUNC225(list, cmdbuf_node);
			return cmdbuf_node;
		} else {
			return NULL; /* cmdbuf_node not last one, should not be removed. */
		}
	} else
		return NULL; /* the last one, should not be removed. */
}

/* calculate executing_time of each vcmd */
u64 GB02FUNC272(bi_list_node *exe_cmdbuf_node)
{
	u64 time_run_all = 0;
	struct GB02STR18 *cmdbuf_obj_temp = NULL;

	while (1) {
		if (exe_cmdbuf_node == NULL)
			break;
		cmdbuf_obj_temp = (struct GB02STR18 *)exe_cmdbuf_node->data;
		time_run_all += cmdbuf_obj_temp->executing_time;
		exe_cmdbuf_node = exe_cmdbuf_node->next;
	}
	return time_run_all;
}

long GB02FUNC277(bi_list *list)
{
	bi_list_node *new_process_node = NULL;

	while (1) {
		new_process_node = list->head;
		if (new_process_node == NULL)
			break;
		/* remove node from list */
		GB02FUNC225(list, new_process_node);
		/* remove node from list */
		GB02FUNC264(new_process_node);
	}
	return 0;
}


