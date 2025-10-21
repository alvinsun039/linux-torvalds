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
#include "mwv207d_vbios.h"
#include "linux/idr.h"

enum mwv207d_vcmd_state {
	VCMD_STATE_IDLE,
	VCMD_STATE_START,
	VCMD_STATE_RECV,
	VCMD_STATE_BUSY,
};

enum mwv207d_vcmd_errcode {
	VCMD_ERRCODE_UNSUPPORTED = 1,
	VCMD_ERRCODE_OVERRANGE,
	VCMD_ERRCODE_BUSY,
	VCMD_ERRCODE_INVALID,
	VCMD_ERRCODE_CMDERR = 1,
};

struct mwv207d_flash {
	void __iomem *base;

	u32 cfg_table_offset;
	struct mutex cfg_lock;
	struct idr cfg_table;
	const void *dat;
};

struct mwv207d_vcmd {
	void __iomem *base;
	struct mutex lock;
};

struct mwv207d_vlog {
	void __iomem *base;
};

struct mwv207d_noc_monitor {
	void __iomem *mmio;
	u32 max_duration;
	u64 coffi;
};

struct mwv207d_noc_counter {
	u32 w_vld_cnt;
	u32 r_vld_cnt;
	u32 vld_cnt;
	u32 total_cnt;
};

struct mwv207d_vbios {
	struct mwv207d_flash flash;
	struct mwv207d_vcmd vcmd;
	struct mwv207d_vlog vlog;
	struct mwv207d_noc_monitor noc_mn;
	char vbios_version[0x80 + 1];
};

struct mwv207d_pll_reg {
	u32 port;
	u32 base;
	u32 div;
	u32 frac;
	u32 post_div3_0;
};

static inline void vcmd_write(struct mwv207d_vcmd *vcmd, u32 reg, u32 value)
{
	writel(value, vcmd->base + reg);
}

static inline u32 vcmd_read(struct mwv207d_vcmd *vcmd, u32 reg)
{
	return readl(vcmd->base + reg);
}

static inline u32 vcmd_get_status(struct mwv207d_vcmd *vcmd)
{
	return vcmd_read(vcmd, 0x0034c138u);
}

static inline u32 u8_to_cpu32(u8 *dat, u32 offset)
{
	return le32_to_cpu(*(uint32_t *)&dat[offset]);
}

static inline u16 u8_to_cpu16(u8 *dat, u32 offset)
{
	return le16_to_cpu(*(uint16_t *)&dat[offset]);
}

static inline u32 mwv207d_flash_read32(struct mwv207d_flash *flash, u32 offset)
{
	BUG_ON(offset & 0x3);

	return readl_relaxed(flash->base + offset);
}

static inline u16 mwv207d_flash_read16(struct mwv207d_flash *flash, u32 offset)
{
	u32 dat;

	BUG_ON(offset & 0x1);

	if (offset & 0x2) {
		dat = mwv207d_flash_read32(flash, offset - 2);
		return (dat >> 16) & 0xffff;
	}

	dat = mwv207d_flash_read32(flash, offset);
	return dat & 0xffff;

}

static inline void mwv207d_flash_cpy(struct mwv207d_flash *flash, void *to,
							u32 offset, u32 len)
{
	void __iomem *from;

	BUG_ON((offset & 0x3) || (len & 0x3));

	for (from = flash->base + offset; len > 0; len -= 4) {
		*(u32 *)to = __raw_readl(from);
		to += 4;
		from += 4;
	}
}

static inline u16 mwv207d_flash_chksum(u8 *dat, u32 len)
{
	u32 i;
	u16 chksum;

	for (chksum = 0, i = 0; i < len; i++)
		chksum +=  dat[i];
	return chksum;
}

static int mwv207d_flash_parse_indexer(struct mwv207d_flash *flash)
{
	u16 calc, chksum, nr_entries;
	u8 *indexer;
	int ret = 0;

	if (mwv207d_flash_read16(flash, 0x6000) != 0xaa55)
		return -EINVAL;

	nr_entries = mwv207d_flash_read16(flash, 0x6000 + 2);
	if (nr_entries + 1 >= PAGE_SIZE / 8 || nr_entries < 0x6 + 1)
		return -EINVAL;

	indexer = kmalloc((nr_entries + 1) * 8, GFP_KERNEL);
	if (!indexer)
		return -ENOMEM;
	mwv207d_flash_cpy(flash, indexer, 0x6000,
			(nr_entries + 1) * 8);

	chksum = u8_to_cpu16(indexer, 4);
	calc = mwv207d_flash_chksum(&indexer[6], nr_entries * 8 + 2);
	if (calc != chksum) {
		ret = -EINVAL;
		goto out;
	}

	flash->cfg_table_offset = u8_to_cpu32(indexer, 0x6 * 8 + 8 + 4);
	if (flash->cfg_table_offset >= SZ_8M) {
		flash->cfg_table_offset = 0;
		ret = -EINVAL;
		goto out;
	}
out:
	kfree(indexer);
	return ret;
}

static int mwv207d_flash_insert_vdat(struct mwv207d_flash *flash, u32 key,
		struct mwv207d_vdat *vdat)
{
	int ret;
	struct idr *table;

	table = &flash->cfg_table;
	mutex_lock(&flash->cfg_lock);
	ret = idr_alloc(table, vdat, key, key + 1, GFP_KERNEL);
	mutex_unlock(&flash->cfg_lock);

	return ret;
}

static void mwv207d_flash_cleanup_cfg(struct mwv207d_flash *flash)
{
	struct mwv207d_vdat *vdat;
	u32 id;

	idr_for_each_entry(&flash->cfg_table, vdat, id) {
		kfree(vdat);
	}
	idr_destroy(&flash->cfg_table);
	kvfree(flash->dat);
}

static int mwv207d_flash_parse_cfg(struct mwv207d_flash *flash)
{
	u8 header[0x14];
	struct mwv207d_vdat *vdat;
	u32 ulen, nlen, len, offset;
	u16 calc, item_len;
	u8 *dat;
	int ret;

	mwv207d_flash_cpy(flash, header, flash->cfg_table_offset,
			  0x14);
	if (memcmp(header, "jcfg", 4))
		return -EINVAL;
	calc = mwv207d_flash_chksum(&header[6], 0x14 - 6);
	if (calc != u8_to_cpu16(header, 4))
		return -EINVAL;

	ulen = u8_to_cpu32(header, 0x8);
	nlen = u8_to_cpu32(header, 0xc);
	len  = ulen + nlen;

	if (len >= SZ_8M || len == 0 || (ulen & 0x3) || (nlen & 0x3))
		return -EINVAL;

	dat = kvmalloc(len, GFP_KERNEL);
	if (!dat)
		return -ENOMEM;
	mwv207d_flash_cpy(flash, dat,
			  flash->cfg_table_offset + 0x14, len);

	mutex_init(&flash->cfg_lock);
	idr_init(&flash->cfg_table);
	flash->dat = dat;

	ret = -EINVAL;
	if (ulen) {
		calc = mwv207d_flash_chksum(dat, ulen);
		if (calc != u8_to_cpu16(header, 0x10))
			goto err_out;
	}
	if (nlen) {
		calc = mwv207d_flash_chksum(&dat[ulen], nlen);
		if (calc != u8_to_cpu16(header, 0x12))
			goto err_out;
	}

	for (offset = 0; offset < len; offset += item_len) {
		vdat = kmalloc(sizeof(struct mwv207d_vdat), GFP_KERNEL);
		if (!vdat) {
			ret = -ENOMEM;
			goto err_out;
		}
		vdat->len = u8_to_cpu16(dat, offset + 2);
		vdat->dat = &dat[offset + 4];
		ret = mwv207d_flash_insert_vdat(flash,
						u8_to_cpu16(dat, offset),
						vdat);
		if (ret < 0) {
			kfree(vdat);
			goto err_out;
		}
		item_len = round_up(vdat->len + 4, 4);
	}
	return 0;
err_out:
	mwv207d_flash_cleanup_cfg(flash);
	return ret;
}

static int mwv207d_flash_init(struct mwv207d_device *mdev,
			      struct mwv207d_flash *flash)
{
	int ret;

	flash->base = mdev->mmio + 0x700000;

	ret = mwv207d_flash_parse_indexer(flash);
	if (ret)
		return ret;

	return mwv207d_flash_parse_cfg(flash);
}

static void mwv207d_flash_fini(struct mwv207d_flash *flash)
{
	mwv207d_flash_cleanup_cfg(flash);
}

static int mwv207d_vlog_read_data(struct mwv207d_vlog *vlog,
				  char *buf, u32 size)
{
	u32 bytes_to_end, avail;
	u32 val, head, tail;

	val = readl_relaxed(vlog->base + 0x0025c800u);
	head = val & 0xFFFF;
	tail = (val >> 16) & 0xFFFF;

	bytes_to_end = (1024*5-4) - head;
	avail = (tail >= head) ? tail - head : tail + bytes_to_end;
	if (avail == 0)
		return 0;

	if (size > avail)
		size = avail;

	if (size <= bytes_to_end)
		memcpy_fromio(buf, vlog->base + 0x0025c804u + head, size);
	else {
		memcpy_fromio(buf, vlog->base + 0x0025c804u + head,
			      bytes_to_end);
		memcpy_fromio(buf + bytes_to_end, vlog->base + 0x0025c804u,
			      size - bytes_to_end);
	}

	return size;
}

static void mwv207d_vbios_read_version(struct mwv207d_vbios *vbios)
{
	u32 offset;

	offset = mwv207d_flash_read32(&vbios->flash,
				      0x6000 + 0x14);
	mwv207d_flash_cpy(&vbios->flash, vbios->vbios_version, offset, 0x80);
}

static void mwv207d_vcmd_init(struct mwv207d_device *mdev,
			      struct mwv207d_vcmd *vcmd,
			      u32 offset)
{
	vcmd->base = mdev->mmio + offset;
	mutex_init(&vcmd->lock);
}

static void mwv207d_vlog_init(struct mwv207d_device *mdev,
			      struct mwv207d_vlog *vlog,
			      u32 offset)
{
	vlog->base = mdev->mmio + offset;
}

static int mwv207d_noc_monitor_ddr_init(struct mwv207d_device *mdev,
				struct mwv207d_noc_monitor *noc_mn)
{
	u32 kfreq;
	int ret;

	noc_mn->mmio = mdev->mmio + 0x0039a700u;
	ret = mwv207d_vbios_get_pll(mdev, MWV207D_PLL_MEM_NOC_HD, &kfreq);
	if (ret)
		return ret;

	BUG_ON(!kfreq);
	noc_mn->max_duration = 0xffffffff / kfreq * 1000;
	noc_mn->coffi = (u64)kfreq * 1000 * 16 / 0x100000;

	return 0;
}

static void mwv207d_noc_ddr_monitor_read(struct mwv207d_noc_monitor *noc_mn,
				struct mwv207d_noc_counter *cnt, u32 index)
{
	cnt->w_vld_cnt = readl_relaxed(noc_mn->mmio + index * 0x100);
	cnt->r_vld_cnt = readl_relaxed(noc_mn->mmio + index  * 0x100 + 0x04);
	cnt->vld_cnt = readl_relaxed(noc_mn->mmio + index  * 0x100 + 0x08);
	cnt->total_cnt = readl_relaxed(noc_mn->mmio + index  * 0x100 + 0x0c);
}

static u32 mwv207d_noc_valid_cnt(u32 cnt0, u32 cnt1)
{
	return cnt1 >= cnt0 ? cnt1 - cnt0 : 0xffffffff - cnt0 + cnt1;
}

static int mwv207d_noc_get_bandwidth(struct mwv207d_noc_monitor *noc_mn, u32 t,
				u32 *rbw, u32 *wbw, u32 *tbw)
{
	struct mwv207d_noc_counter cnt0[0x10];
	struct mwv207d_noc_counter cnt1[0x10];
	u32 tld, wvld, rvld, tvld, i;
	u64 bw[3];

	memset(bw, 0, sizeof(bw));
	for (i = 0; i < 0x10; i++)
		mwv207d_noc_ddr_monitor_read(noc_mn, &cnt0[i], i);

	usleep_range(t, t);

	for (i = 0; i < 0x10; i++)
		mwv207d_noc_ddr_monitor_read(noc_mn, &cnt1[i], i);

	for (i = 0; i < 0x10; i++) {
		wvld = mwv207d_noc_valid_cnt(cnt0[i].w_vld_cnt, cnt1[i].w_vld_cnt);
		rvld = mwv207d_noc_valid_cnt(cnt0[i].r_vld_cnt, cnt1[i].r_vld_cnt);
		tvld = mwv207d_noc_valid_cnt(cnt0[i].vld_cnt, cnt1[i].vld_cnt);
		tld = mwv207d_noc_valid_cnt(cnt0[i].total_cnt, cnt1[i].total_cnt);

		if (!tld)
			return -EINVAL;

		bw[0] += (u64)(wvld * noc_mn->coffi) / tld;
		bw[1] += (u64)(rvld * noc_mn->coffi) / tld;
		bw[2] += (u64)(tvld * noc_mn->coffi) / tld;
	}
	*wbw = bw[0];
	*rbw = bw[1];
	*tbw = bw[2];

	return 0;
}

int mwv207d_vbios_init(struct mwv207d_device *mdev)
{
	struct mwv207d_vbios *vbios;
	int ret;

	if (!mdev->hw.is_pf)
		return 0;

	vbios = devm_kzalloc(mdev->dev, sizeof(*vbios), GFP_KERNEL);
	if (!vbios)
		return -ENOMEM;
	mdev->vbios = vbios;

	ret = mwv207d_flash_init(mdev, &vbios->flash);
	if (ret)
		return ret;

	mwv207d_vbios_read_version(mdev->vbios);

	mwv207d_vcmd_init(mdev, &vbios->vcmd, 0x0);
	mwv207d_vlog_init(mdev, &vbios->vlog, 0x0);
	return mwv207d_noc_monitor_ddr_init(mdev, &vbios->noc_mn);
}

const char *mwv207d_vbios_get_version(struct mwv207d_device *mdev)
{
	if (!mdev || !mdev->vbios)
		return NULL;

	return mdev->vbios->vbios_version;
}

void mwv207d_vbios_fini(struct mwv207d_device *mdev)
{
	if (!mdev->vbios)
		return;
	mwv207d_flash_fini(&mdev->vbios->flash);
}

const struct mwv207d_vdat *mwv207d_vbios_vdat(struct mwv207d_device *mdev,
					      u32 die, u32 key)
{
	struct mwv207d_vdat *vdat;
	struct mwv207d_flash *flash;

	BUG_ON(!mdev->hw.is_pf);
	BUG_ON(die >= 0x4);
	BUG_ON((key >> 14) & 0x3);

	flash = &mdev->vbios->flash;
	mutex_lock(&flash->cfg_lock);
	vdat = idr_find(&flash->cfg_table, (die << 14) | key);
	if (!vdat && die)
		vdat = idr_find(&flash->cfg_table, key);

	mutex_unlock(&flash->cfg_lock);

	return vdat;
}

static inline int vcmd_is_idle(struct mwv207d_vcmd *vcmd)
{
	return (vcmd_get_status(vcmd) >> 28) == VCMD_STATE_IDLE;
}

static int mwv207d_vcmd_wait_idle(struct mwv207d_vcmd *vcmd)
{
	return wait_for(vcmd_is_idle(vcmd), 1000);
}

static int mwv207d_vcmd_check_result(struct mwv207d_vcmd *vcmd)
{
	u32 errcode;

	errcode = (vcmd_get_status(vcmd) >> 24) & 0xf;
	if (likely(!errcode))
		return 0;

	if ((errcode & 0x1) & VCMD_ERRCODE_CMDERR)
		pr_err("error, vcmd unsupported request");

	errcode >>= 1;
	switch (errcode) {
	case VCMD_ERRCODE_UNSUPPORTED:
		pr_err("error, vcmd unsupported cmd type");
		break;
	case VCMD_ERRCODE_OVERRANGE:
		pr_info_once("vcmd over range");
		break;
	case VCMD_ERRCODE_BUSY:
		pr_err("error, vcmd is busy");
		break;
	case VCMD_ERRCODE_INVALID:
		pr_err("error, invalid vcmd argument");
		break;
	default:
		pr_err("error, vcmd error %d", errcode);
	}

	return -EIO;
}

static int mwv207d_vcmd_send_request(struct mwv207d_vcmd *vcmd, u32 req)
{
	int ret;

	mutex_lock(&vcmd->lock);

	ret = mwv207d_vcmd_wait_idle(vcmd);
	if (ret) {
		pr_err("error, wait vcmd idle timeout before request");
		goto unlock_out;
	}

	vcmd_write(vcmd, 0x0034c134u, req);
	vcmd_write(vcmd, 0x0034c138u, VCMD_STATE_START << 28);

	ret = mwv207d_vcmd_wait_idle(vcmd);
	if (ret) {
		pr_err("error, wait vcmd idle timeout after requested");
		goto unlock_out;
	}

	ret = mwv207d_vcmd_check_result(vcmd);

unlock_out:
	mutex_unlock(&vcmd->lock);
	return ret;
}

int mwv207d_vbios_set_pll(struct mwv207d_device *mdev,
			  enum mwv207d_pll_id id,
			  u32 kfreq)
{
	u32 req;

	BUG_ON(!mdev->hw.is_pf);
	BUG_ON(id >= MWV207D_PLL_NR);

	req = 1u << 28;
	req |= ((u32)(id & 0x1f)) << 23;
	req |= (kfreq & 0x3fffff) << 1;
	req |= (kfreq ? 1 : 0);

	return mwv207d_vcmd_send_request(&mdev->vbios->vcmd, req);
}

int mwv207d_vbios_set_volt(struct mwv207d_device *mdev,
			   int volt_id, u32 volt)
{
	BUG_ON(!mdev->hw.is_pf);

	return 0;
}

static const struct mwv207d_pll_reg mwv207d_pll_regs[MWV207D_PLL_NR] = {
	{ 0, 0x00340000u, 0x40, 0x44, 0x48 },
	{ 1, 0x00340000u, 0x40, 0x44, 0x48 },
	{ 0, 0x00340000u, 0x60, 0x64, 0x68 },

	{ 0, 0x00342000u, 0x40, 0x44, 0x48 },
	{ 1, 0x00342000u, 0x60, 0x64, 0x68 },
	{ 0, 0x00342000u, 0x60, 0x64, 0x68 },

	{ 0, 0x00344000u, 0x40, 0x44, 0x48 },
	{ 1, 0x00344000u, 0x40, 0x44, 0x48 },
	{ 0, 0x00344000u, 0x60, 0x64, 0x68 },

	{ 0, 0x00346000u, 0x40, 0x44, 0x48 },
	{ 1, 0x00346000u, 0x40, 0x44, 0x48 },
	{ 0, 0x00346000u, 0x60, 0x64, 0x68 },

	{ 0, 0x00348000u, 0xa40, 0xa44, 0xa48 },
	{ 0, 0x00348000u, 0xa60, 0xa64, 0xa68 },
	{ 0, 0x00348000u, 0xa80, 0xa84, 0xa88 },
	{ 0, 0x00348000u, 0xaa0, 0xaa4, 0xaa8 },

	{ 0, 0x2B0000, 0xa00, 0xa04, 0xa08 },
	{ 0, 0x2B0000, 0xa20, 0xa24, 0xa28 },
	{ 0, 0x2B0000, 0xa40, 0xa44, 0xa48 },
	{ 0, 0x2B0000, 0xa60, 0xa64, 0xa68 },
	{ 0, 0x2B0000, 0xa80, 0xa84, 0xa88 },
	{ 0, 0x2B0000, 0xaa0, 0xaa4, 0xaa8 },
	{ 0, 0x2B0000, 0xac0, 0xac4, 0xac8 },
	{ 0, 0x2B0000, 0xb00, 0xb04, 0xb08 },
	{ 1, 0x2B0000, 0xb00, 0xb04, 0xb08 },
	{ 3, 0x2B0000, 0xb00, 0xb04, 0xb08 },
	{ 0, 0x2B0000, 0xb20, 0xb24, 0xb28 }
};

int mwv207d_vbios_get_pll(struct mwv207d_device *mdev,
			  enum mwv207d_pll_id id,
			  u32 *kfreq)
{
	const struct mwv207d_pll_reg *reg;
	u32 ref_div, fbint_div, fbrac_div, post_div;
	u32 div_state, frac_state, post_div_state;
	u32 post_div_mask, post_div_offset;
	u32 freq;

	BUG_ON(!mdev->vbios);

	if (id < 0 || id >= MWV207D_PLL_NR)
		return -EINVAL;

	reg = &mwv207d_pll_regs[id];
	mutex_lock(&mdev->vbios->vcmd.lock);
	div_state = mdev_read(mdev, reg->base + reg->div);
	frac_state = mdev_read(mdev, reg->base + reg->frac);
	post_div_state = mdev_read(mdev, reg->base + reg->post_div3_0);
	mutex_unlock(&mdev->vbios->vcmd.lock);

	ref_div = div_state & 0x0000003f;
	fbint_div = (div_state & 0x0fff0000) >> 16;
	fbrac_div = frac_state & 0x00ffffff;
	post_div_offset = 8 * ((reg->port) % 4);
	post_div_mask = 0x0000007f << post_div_offset;

	post_div = (post_div_state & post_div_mask) >> post_div_offset;
	if (!ref_div)
		ref_div = 1;
	if (!post_div)
		post_div = 1;

	freq = 100000u / ref_div;
	freq = freq * fbint_div + ((freq * fbrac_div) >> 24);
	freq /= 4;
	freq /= post_div;

	*kfreq = freq;
	return 0;
}

u32 mwv207d_vbios_get_temp(struct mwv207d_device *mdev)
{
	return mdev_read(mdev, 0x0034c128u);
}

u32 mwv207d_vbios_get_fan_speed(struct mwv207d_device *mdev)
{
	return mdev_read(mdev, 0x0034c13cu);
}

int mwv207d_read_vlog(struct mwv207d_device *mdev, char *buf, u32 size)
{
	BUG_ON(!mdev->hw.is_pf);

	return mwv207d_vlog_read_data(&mdev->vbios->vlog, buf, size);
}

int mwv207d_vbios_get_ddr_bandwidth(struct mwv207d_device *mdev, u32 t,
					u32 *rbw, u32 *wbw, u32 *tbw)
{
	BUG_ON(!mdev->hw.is_pf);

	return mwv207d_noc_get_bandwidth(&mdev->vbios->noc_mn, t, rbw, wbw, tbw);
}

u32 mwv207d_vbios_get_max_duration(struct mwv207d_device *mdev)
{
	struct mwv207d_noc_monitor *noc_mn = &mdev->vbios->noc_mn;

	return noc_mn->max_duration;
}
