/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/kernel.h>

#include "mwv207d_bo.h"
#include "mwv207d_sched.h"

static int monitor_period = 500;
module_param(monitor_period, int, 0444);
MODULE_PARM_DESC(monitor_period, "monitor period");

struct mwv207d_monitor {
	struct mwv207d_device *mdev;
	u32 *vaddr;
	struct delayed_work work;
	int period;
	u32 heartbeat;
};

static inline void monitor_update(struct mwv207d_monitor *m, int idx, u32 val)
{
	WRITE_ONCE(m->vaddr[idx], val);
}

static void mwv207d_monitor_update_pipe_usage(struct mwv207d_monitor *m,
					      u8 type, u8 id, u32 usage)
{
	int idx;

	BUG_ON(id > 1);

	switch (type) {
	case 0x0:
		idx = 0x19;
		break;
	case 0x3:
		idx = 0x18;
		break;
	case 0x5:
		idx = 0x1A;
		break;
	case 0x2:
		idx = 0x1B + id * 2;
		break;
	case 0x1:
		idx = 0x1C + id * 2;
		break;
	default:
		return;
	}

	monitor_update(m, idx, usage);
}

static void mwv207d_monitor_info_init(struct mwv207d_monitor *m)
{
	u32 version = 1 << 24 | DRIVER_MAJOR << 16 | DRIVER_MINOR << 8;

	monitor_update(m, 0x0, 0);
	monitor_update(m, 0x1, version);
	monitor_update(m, 0x8, 0);
	monitor_update(m, 0x9, 0);
	monitor_update(m, 0xA, 0);
	monitor_update(m, 0x18, 0);
	monitor_update(m, 0x19, 0);
	monitor_update(m, 0x1A, 0);
	monitor_update(m, 0x1B, 0);
	monitor_update(m, 0x1C, 0);
	monitor_update(m, 0x1D, 0);
	monitor_update(m, 0x1E, 0);
	monitor_update(m, 0x1F, 0);
}

static void monitor_work(struct work_struct *work)
{
	struct mwv207d_monitor *m = container_of(to_delayed_work(work),
						 struct mwv207d_monitor,
						 work);
	struct mwv207d_device *mdev = m->mdev;
	u32 usage;
	int i, j;

	for (i = 0; i < 0x6; i++) {
		for (j = 0; j < mdev->nr_pipe[i]; j++) {
			usage = mwv207d_pipe_get_usage(mdev, i, j);
			mwv207d_monitor_update_pipe_usage(m, i, j, usage);
		}
	}

	monitor_update(m, 0x0, ++m->heartbeat);
	monitor_update(m, 0x8,
		(u32)(atomic64_read(&mdev->vram_used_bytes) >> 10));
	monitor_update(m, 0x9,
		(u32)(atomic64_read(&mdev->visible_vram_used_bytes) >> 10));
	monitor_update(m, 0xA,
		(u32)(atomic64_read(&mdev->gtt_used_bytes) >> 10));

	schedule_delayed_work(&m->work, msecs_to_jiffies(m->period));
}

int mwv207d_monitor_init(struct mwv207d_device *mdev)
{
	struct mwv207d_monitor *m;

	m = devm_kzalloc(mdev->dev, sizeof(*m), GFP_KERNEL);
	if (!m)
		return -ENOMEM;
	m->mdev = mdev;
	mdev->monitor = m;

	BUG_ON(test_bit(0, mdev->pt_bitmap) || test_bit(1, mdev->pt_bitmap));
	bitmap_set(mdev->pt_bitmap, 0, 2);
	m->vaddr = mdev->ap_vaddr;

	mwv207d_monitor_info_init(m);

	if (monitor_period < 100)
		monitor_period = 100;
	if (monitor_period > 2000)
		monitor_period = 2000;
	m->period = monitor_period;

	INIT_DELAYED_WORK(&m->work, monitor_work);

	schedule_delayed_work(&m->work, msecs_to_jiffies(m->period));

	return 0;
}

void mwv207d_monitor_fini(struct mwv207d_device *mdev)
{
	if (!mdev->monitor)
		return;
	cancel_delayed_work_sync(&mdev->monitor->work);
}

int mwv207d_monitor_suspend(struct mwv207d_device *mdev)
{
	struct mwv207d_monitor *m = mdev->monitor;

	if (!m)
		return 0;

	cancel_delayed_work_sync(&m->work);

	return 0;
}

void mwv207d_monitor_resume(struct mwv207d_device *mdev)
{
	struct mwv207d_monitor *m = mdev->monitor;

	schedule_delayed_work(&m->work, msecs_to_jiffies(m->period));
}
