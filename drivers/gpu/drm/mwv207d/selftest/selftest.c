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
#include "mwv207d_drv.h"
#include "mwv207d_bo.h"
#include "mwv207d_drm.h"
#include "mwv207d_vbios.h"

static int mwv207d_test_create_and_map_bo(struct mwv207d_device *mdev, u64 size,
		u32 domain, u32 flags, struct mwv207d_bo **pmbo, void **plogical)
{
	struct mwv207d_bo *mbo;
	void *logical;
	int ret;

	ret = mwv207d_bo_create(mdev, size, 0, true,
				domain, flags, NULL, NULL, &mbo);
	if (ret)
		return ret;
	ret = mwv207d_bo_reserve(mbo, true);
	if (ret)
		goto free_bo;
	ret = mwv207d_bo_kmap(mbo, &logical);
	if (ret)
		goto unreserve_bo;

	*pmbo = mbo;
	*plogical = logical;
	return 0;

unreserve_bo:
	mwv207d_bo_unreserve(mbo);
free_bo:
	mwv207d_bo_unref(&mbo);
	return ret;
}

static void mwv207d_test_destroy_and_unmap_bo(struct mwv207d_bo *mbo)
{
	if (!mbo)
		return;

	mwv207d_bo_kunmap(mbo);
	mwv207d_bo_unreserve(mbo);
	mwv207d_bo_unref(&mbo);
}

static int mwv207d_test_bo_acc(struct mwv207d_device *mdev)
{
	struct mwv207d_bo *mbo;
	int i, ret;
	void *logical;

	ret = mwv207d_test_create_and_map_bo(mdev, 0x1000,
			0x2, 0x1, &mbo, &logical);
	if (ret)
		return ret;

	memset(logical, 0x5a, 0x1000);
	for (i = 0; i < 0x1000; i++) {
		if (*((char *)logical + i) != 0x5a) {
			ret = -EIO;
			break;
		}
	}

	mwv207d_test_destroy_and_unmap_bo(mbo);

	return ret;
}

static int mwv207d_test_bo(struct mwv207d_device *mdev)
{
	int ret;

	ret = mwv207d_test_bo_acc(mdev);
	if (ret) {
		pr_err("mwv207d: bo access test failed with %d", ret);
		return ret;
	}

	return ret;
}

static int mwv207d_test_vbios(struct mwv207d_device *mdev)
{
	const struct mwv207d_vdat *dat0, *dat;
	int i;

	if (!mdev->hw.is_pf)
		return 0;

	dat0 = mwv207d_vbios_vdat(mdev, 0, 0x3E8);
	if (!dat0 || dat0->len != 4)
		return -EBADF;

	if (le32_to_cpu(*(uint32_t *)dat0->dat) != 0xa5a5a5a5)
		return -EBADF;

	for (i = 1; i < 4; i++) {
		dat = mwv207d_vbios_vdat(mdev, i, 0x3E8);
		if (dat != dat0)
			return -EBADF;
	}

	return 0;
}

int mwv207d_test(struct mwv207d_device *mdev)
{
	int ret;

	ret = mwv207d_test_bo(mdev);
	if (ret)
		pr_debug("warning, mwv207d selftest failed");

	ret = mwv207d_test_vbios(mdev);
	if (ret)
		pr_debug("warning, mwv207d vbios test failed");

	return ret;
}
