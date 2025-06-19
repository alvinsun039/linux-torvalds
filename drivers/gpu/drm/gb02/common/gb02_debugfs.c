/*
 *  Copyright (C) Sietium Electronics Co.Ltd
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/namei.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_fb_helper.h>

#include "common/gb_common.h"
#include "kms/gbdc_mode.h"
#include "kms/gb_pcie_map.h"
#include "kms/gbdc_fbdev.h"
#include "kms/gbdc_pm.h"
#include "kms/gbdc_mm.h"
#include "kms/gbdc_connector.h"
#include "kms/device/gb_dev_res.h"
#include "kms/gb_kms.h"
#include "kms/device/gbdc_device.h"
#include "kms/device/gbdc_regs.h"
#include "kms/device/reg_ops.h"
#include "kms/gbdc_planes.h"
#include "kms/gbdc_crtc.h"
#include "kms/gbdc_encoder.h"
#include "kms/gbdc_mode.h"
#include "kms/gbdc_display.h"
#include "kms/gbdc_drv.h"
#include "kms/device/gb_ip.h"
#include "kms/gbdc_hdmi_ddc.h"
#include "kms/gbdc_backlight.h"
#include "gb_common.h"

int GB02FUNC4(enum gb_board_type gb_type, int id)
{
	int ret = 0;

	if (id < 0 || id > 5)
		return -1;

	if ((gb_type == PCIE_LPDDR4 || gb_type == PCIE_C0_200) && (id > 1))
		ret = 1;
	else if ((gb_type == PCIE_FULL_LPDDR4) && (id > 4))
		ret = 1;
	else if ((gb_type == PCIE_HIE1LP4_LPDDR4) && (id > 3))
		ret = 1;

	return ret;
}

int GB02FUNC7(enum gb_board_type gb_type, int crtc_id)
{
	int ret_id = 0;

	if (gb_type == PCIE_LPDDR4 || gb_type == PCIE_C0_200) {
		if (crtc_id == GB02MAC2830)
			ret_id =  2;
		else if (crtc_id == GB02MAC2831)
			ret_id =  3;
	} else if (gb_type == PCIE_FULL_LPDDR4) {
		if (crtc_id == GB02MAC2830)
			ret_id = 0;
		else if (crtc_id == GB02MAC2831)
			ret_id = 2;
		else if (crtc_id == GB02MAC2832)
			ret_id = 3;
		else if (crtc_id == GB02MAC2833)
			ret_id = 4;
		else if (crtc_id == GB02MAC2834)
			ret_id = 5;
	} else if (gb_type == PCIE_M6FL8G_LPDDR4) {
			ret_id = crtc_id;
	} else if (gb_type == PCIE_HIE1LP4_LPDDR4) {
		if (crtc_id == GB02MAC2830)
			ret_id = 0;
		else if (crtc_id == GB02MAC2831)
			ret_id = 2;
		else if (crtc_id == GB02MAC2832)
			ret_id = 3;
		else if (crtc_id == GB02MAC2833)
			ret_id = 5;
	} else {
		ret_id = crtc_id;
	}

	return ret_id;
}

static int GB02FUNC12(const char __user *buf, size_t len, u32 *val_u32, char *file_name)
{
	char tmpbuf[64];
	int err, count, i, count_tmp;
	char *param[GB02MAC242], *file_tmp, *tmp, *str = " ";

	if (copy_from_user(tmpbuf, buf, len))
		return -EFAULT;

	tmpbuf[len] = '\0';
	tmp = tmpbuf;
	err = kstrtoint(strsep(&tmp, str), 0, &count);
	if (err) {
		printk(KERN_INFO "[%s]:Param error\n", __func__);
		return err;
	}
	printk(KERN_INFO "count = 0x%x\n", count);
	if (count <= GB_DBG_MAX && count >= 0) {
		if (count < GB02MAC242)
			count_tmp = count + 1;
		else
			count_tmp = GB02MAC242;

		for (i = 0; i < count_tmp; i++) {
			param[i] = strsep(&tmp, str);
			if (param[i] == NULL) {
				printk(KERN_INFO "[%s]:Param %d is NULL !\n",
						__func__, i);
				return -EINVAL;
			}
			if (val_u32 != NULL) {
				err = kstrtouint(param[i], 0, &val_u32[i]);
				if (err) {
					printk(KERN_INFO "[%s]:u32 param too large\n",
						__func__);
					return err;
				}
				printk(KERN_INFO "val_u32[%d] = 0x%x\n", i, val_u32[i]);
			}
		}

		if (count == GB_DBG_MAX) {
			file_tmp = strsep(&tmp, str);
			if (file_tmp == NULL) {
				printk(KERN_INFO "[%s]:Param filename is NULL !\n",
					__func__);
				return -EINVAL;
			}
			printk(KERN_INFO "file_tmp:%s\n", file_tmp);
			strcpy(file_name, file_tmp);
			file_name[strcspn(file_name, "\n")] = 0;
			printk(KERN_INFO "file_name:%s\n", file_name);
		}

	} else {
		printk(KERN_INFO"[%s]:The number of param is error!\n", __func__);
		return -EINVAL;
	}
	return 0;
}

/*dc debug*/
static int GB02FUNC29(void __iomem *dc_base)
{
	struct file *filp;
	char obuf[64] = "dc";
	u32 dc_ret, i;

	filp = filp_open(DC_REG_DATA_FILE, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(filp)) {
		printk(KERN_INFO "create file error\n");
		return -1;
	}

	if (dc_base == NULL)
		return -EINVAL;
	sprintf(obuf, "%s", "\nDisplay engine control registers \n\n");
	kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	for (i = 0; i <= GB02MAC245; i++) {
		dc_ret = GB02FUNC730(dc_base, i * 4);
		sprintf(obuf, "%s%x%s%08x%s", "addr:0x", i * 4,
					" data:0x", dc_ret, "\n");
		kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	}
	sprintf(obuf, "%s", "\nScaling engine control registers\n\n");
	kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	for (i = 0; i <= GB02MAC247; i++) {
		dc_ret = GB02FUNC730(dc_base + GB02MAC256, i * 4);
		sprintf(obuf, "%s%x%s%08x%s", "addr:0x",
				(u32)(GB02MAC256 + i*4),
				" data:0x", dc_ret, "\n");
		kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	}

	sprintf(obuf, "%s", "\ndisplay core control registers \n\n");
	kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	for (i = 0; i <= GB02MAC249; i++) {
		dc_ret = GB02FUNC730(dc_base + GB02MAC258, i * 4);
		sprintf(obuf, "%s%x%s%08x%s", "addr:0x",
					(u32)(GB02MAC258 + i * 4),
					" data:0x", dc_ret, "\n");
		kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	}

	for (i = 0; i <= GB02MAC251; i++) {
		dc_ret = GB02FUNC730(dc_base + GB02MAC260, i * 4);
		sprintf(obuf, "%s%x%s%08x%s", "addr:0x",
					(u32)(GB02MAC260 + i * 4),
					" data:0x", dc_ret, "\n");
		kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	}

	filp_close(filp, NULL);

	return 0;
}

static int GB02FUNC32(void __iomem *fb_map, struct GB02STR21 *flush,
			struct GB02STR19 *mode_info)
{
	u32 v, h;

	if (NULL == fb_map || NULL == mode_info || NULL == flush)
		return -EINVAL;
	printk(KERN_INFO "crtc: %d fb_offset: 0x%x mode_info: %dx%d pitch: %d\n",
						flush->crtc,
						flush->fb_offset,
						mode_info->hdisplay,
						mode_info->vdisplay,
						mode_info->pitch);
	if (flush->value == 0) {
		flush->value = 0xff00ff00;
		printk(KERN_INFO "flush->value = 0x%x\n", flush->value);
	}

	for (v = 0; v < mode_info->vdisplay; ++v) {
		for (h = 0; h < mode_info->hdisplay; ++h)
			GB02FUNC733(fb_map, v * mode_info->pitch + h * 4, flush->value);
	}

	return 0;
}

static int GB02FUNC39(void __iomem *fb_map,
		struct GB02STR19 *mode_info, char *file_name)
{
	struct file *filp;
	u32 rx, v;

	if (file_name == NULL || strlen(file_name) == 0)
		filp = filp_open(DC_FB_TEST_FILE, O_RDONLY, 0644);
	else
		filp = filp_open(file_name, O_RDONLY, 0644);

	if (IS_ERR(filp)) {
		printk(KERN_INFO "can't not open file %s\n", file_name);
		return -1;
	}

	printk(KERN_INFO "file tp fb, pitch = %d. vdisplay = %d\n",
				mode_info->pitch, mode_info->vdisplay);
	for (v = 0; v < mode_info->vdisplay; ++v) {
		rx = kernel_read(filp, fb_map + (v * mode_info->pitch),
			mode_info->hdisplay * 4, &filp->f_pos);
	}
	filp_close(filp, NULL);

	return 0 ;
}

static int GB02FUNC45(void __iomem *fb_map,
			struct GB02STR19 *mode_info)
{
	struct file *filp;
	u32 v;

	filp = filp_open(DC_FB_READ_FILE, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(filp)) {
		printk(KERN_INFO "create file error\n");
		return -1;
	}

	for (v = 0; v < mode_info->vdisplay; ++v) {
		kernel_write(filp, fb_map + (v * mode_info->pitch),
			mode_info->hdisplay * 4, &filp->f_pos);
	}

	filp_close(filp, NULL);

	return 0;
}

static int GB02FUNC50(u32 crtc_id, u32 reg_offset, u32 value)
{
	struct file *filp;
	char obuf[64] = {0};

	filp = filp_open(DC_REG_WRITE_RECORD, O_RDWR | O_CREAT | O_APPEND, 0644);
	if (IS_ERR(filp)) {
		printk(KERN_INFO "create file error\n");
		return -1;
	}
	sprintf(obuf, "%s%d %s%x %s%08x%s", "crtc_id:", crtc_id,
			 "reg_offset:0x", reg_offset, "value:0x", value, "\n");
	kernel_write(filp, obuf, strlen(obuf), &filp->f_pos);
	filp_close(filp, NULL);

	return 0;
}

static int GB02FUNC53(struct seq_file *m, void *data)
{
	struct GB02STR155 *gdev = m->private;

	seq_printf(m, "0x%x\n", gdev->gb_dbg_info.r_val_dc);

	return 0;
}

int GB02FUNC56(struct inode *inode, struct file *file)
{
	struct GB02STR39 *gdev =  inode->i_private;

	return single_open(file, GB02FUNC53, gdev);
}

static ssize_t GB02FUNC58(struct file *file,
				      const char __user *ubuf, size_t len,
				      loff_t *ppos)
{
	struct seq_file *m = file->private_data;
	struct GB02STR155 *gdev = m->private;
	struct GB02STR249 *dc_cfg_crtc[GB02MAC656];
	struct GB02STR253 *vram_cfg = &gdev->pcie_info.vram_config;
	struct GB02STR21 flush_info = {0, 0, 0}, *flush;
	void __iomem *de_base_crtc[GB02MAC656];
	int err, i;
	u32 val_32[GB_DBG_MAX], cmd, fb_base, crtc_id, pitch;

	struct GB02STR19 *mode_info[GB02MAC656];
	char file_name[64] = {0};
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);

	flush = &flush_info;
	flush->value = 0;
	printk(KERN_INFO "vram_cfg->fb_base = 0x%lx vram_cfg->fb_map = 0x%p\n",
				vram_cfg->fb_base, vram_cfg->fb_map);
	for (i = 0; i < GB02MAC656; i++) {
		mode_info[i] = &gdev->gb_dbg_info.modeinfo[i];
		dc_cfg_crtc[i] = &gdev->pcie_info.dc_config[i];
		de_base_crtc[i] = dc_cfg_crtc[i]->de_base;
		printk(KERN_INFO "crtc: %d mode_info: %dx%d pitch: %d\n",
						i,
						mode_info[i]->hdisplay,
						mode_info[i]->vdisplay,
						mode_info[i]->pitch);
	}
	/*get param*/
	memset(val_32, 0, sizeof(val_32));
	err = GB02FUNC12(ubuf, len, val_32, file_name);
	if (err)
		return -EINVAL;

	cmd = val_32[GB_DBG_CMD];
	if (GB02FUNC4(gb_type, val_32[GB_DBG_PORT])) {
		printk(KERN_INFO "input error crtc id\n");
		return -EINVAL;
	} else {
		crtc_id = GB02FUNC7(gb_type, val_32[GB_DBG_PORT]);
	}

	if (val_32[GB_DBG_OFFSET] >= GB02MAC264) {
		printk(KERN_INFO "input invalid address\n");
		return -EINVAL;
	}

	switch (cmd) {
	case GB02MAC270:
		gdev->gb_dbg_info.r_val_dc =
			GB02FUNC730(de_base_crtc[crtc_id], val_32[GB_DBG_OFFSET]);
		printk(KERN_INFO "read 0x%x is 0x%x\n",
			val_32[GB_DBG_OFFSET], gdev->gb_dbg_info.r_val_dc);
		break;
	case GB02MAC271:
		GB02FUNC733(de_base_crtc[crtc_id],
			val_32[GB_DBG_OFFSET], val_32[GB_DBG_VALUE]);
		GB02FUNC50(val_32[GB_DBG_PORT],
			val_32[GB_DBG_OFFSET], val_32[GB_DBG_VALUE]);
		break;
	case GB02MAC272:
		fb_base = GB02FUNC730(de_base_crtc[crtc_id], GB02MAC268);
		printk(KERN_INFO "fb_base = 0x%x\n", fb_base);
		if (!fb_base)
			break;
		pitch = GB02FUNC730(de_base_crtc[crtc_id], GB02MAC266);
		mode_info[crtc_id]->pitch = (pitch & 0x1FFFF);
		flush->value = val_32[GB_DBG_VALUE];
		flush->fb_offset = fb_base;
		flush->crtc = crtc_id;
		err = GB02FUNC32(vram_cfg->fb_map + fb_base,
				flush, mode_info[crtc_id]);
		if (err < 0)
			return err;
		break;
	case GB02MAC273:
		err = GB02FUNC29(de_base_crtc[crtc_id]);
		if (err < 0)
			return err;
		break;
	case GB02MAC274:
		fb_base = GB02FUNC730(de_base_crtc[crtc_id], GB02MAC268);
		printk(KERN_INFO "fb_base = 0x%x\n", fb_base);
		if (!fb_base)
			break;
		pitch = GB02FUNC730(de_base_crtc[crtc_id], GB02MAC266);
		mode_info[crtc_id]->pitch = pitch & 0x1FFFF;
		printk(KERN_INFO "file_name = %s\n", file_name);
		err = GB02FUNC39(vram_cfg->fb_map + fb_base,
				mode_info[crtc_id], file_name);
		if (err < 0)
			return err;
		break;
	case GB02MAC275:
		fb_base = GB02FUNC730(de_base_crtc[crtc_id], GB02MAC268);
		printk(KERN_INFO "fb_base = 0x%x\n", fb_base);
		if (!fb_base)
			break;
		pitch = GB02FUNC730(de_base_crtc[crtc_id], GB02MAC266);
		mode_info[crtc_id]->pitch = (pitch & 0x1FFFF);
		err = GB02FUNC45(vram_cfg->fb_map + fb_base, mode_info[crtc_id]);
		if (err < 0)
			return err;
		break;
	default:
		printk(KERN_INFO"[%s]:cmd error\n", __func__);
		return -EINVAL;
	}

	return len;
}

static const struct file_operations gb_debugfs_dc_ops = {
	.open = GB02FUNC56,
	.read = seq_read,
	.write = GB02FUNC58,
};

/*dp*/
static int GB02FUNC90(struct seq_file *m, void *data)
{
	struct GB02STR155 *gdev = m->private;

	seq_printf(m, "0x%x\n", gdev->gb_dbg_info.r_val_dp);

	return 0;
}

int GB02FUNC93(struct inode *inode, struct file *file)
{
	struct GB02STR39 *gdev =  inode->i_private;

	return single_open(file, GB02FUNC90, gdev);
}

static ssize_t GB02FUNC98(struct file *file,
				const char __user *ubuf, size_t len,
				loff_t *ppos)
{
	struct seq_file *m = file->private_data;
	struct GB02STR155 *gdev = m->private;
	u32 val_32[GB_DBG_MAX], cmd, crtc_id, reg, val;
	char file_name[64] = {0};
	int err;
	struct dptx *dptx_info = NULL;
	struct GB02STR70 *pcie_info = GB02FUNC518();
	enum gb_board_type gb_type = GB02FUNC503(pcie_info);

	/*get param*/
	memset(val_32, 0, sizeof(val_32));
	err = GB02FUNC12(ubuf, len, val_32, file_name);
	if (err)
		return -EINVAL;

	cmd = val_32[GB_DBG_CMD];

	if (GB02FUNC4(gb_type, val_32[GB_DBG_PORT])) {
		printk(KERN_ERR "input error crtc id\n");
		return -EINVAL;
	} else {
		crtc_id = GB02FUNC7(gb_type, val_32[GB_DBG_PORT]);
	}
	dptx_info = pcie_info->dptx[crtc_id];

	switch (cmd) {
	case GB02MAC276:
		gdev->gb_dbg_info.r_val_dp =
			dptx_readl(dptx_info, val_32[GB_DBG_OFFSET]);
		break;
	case GB02MAC277:
		dptx_writel(dptx_info, val_32[GB_DBG_OFFSET], val_32[GB_DBG_VALUE]);
		break;
	case GB02MAC278:
		reg = dptx_readl(dptx_info, GB02MAC2057);
		if (val_32[GB_DBG_OFFSET])
			reg |= GB02MAC2157;
		else
			reg &= (~GB02MAC2157);
		dptx_writel(dptx_info, GB02MAC2057, reg);
		break;
	case GB02MAC279:
		if (val_32[GB_DBG_OFFSET] > GB02MAC285) {
			printk(KERN_INFO"[%s]:rate error\n", __func__);
			return -EINVAL;
		}
		GB02FUNC1090(dptx_info, val_32[GB_DBG_OFFSET]);
		break;
	case GB02MAC280:
		val = dptx_readl(dptx_info, GB02MAC2107);
		val &= DPTX_PHYIF_CTRL_RATE_MASK;
		val = val >> GB02MAC2183;
		gdev->gb_dbg_info.r_val_dp = val;
		break;
	case GB02MAC281:
		val = dptx_readl(dptx_info, GB02MAC2107);
		val &= DPTX_PHYIF_CTRL_LANES_MASK;
		val = val >> GB02MAC2184;
		if (val == 0)
			gdev->gb_dbg_info.r_val_dp = GB02MAC282;
		else if (val == 1)
			gdev->gb_dbg_info.r_val_dp = GB02MAC283;
		else if (val == 2)
			gdev->gb_dbg_info.r_val_dp = GB02MAC284;
		else
			printk(KERN_INFO"lane is error set: %d\n", val);
		break;
	default:
		printk(KERN_INFO"[%s]:cmd error\n", __func__);
		return -EINVAL;
	}

	return len;
}

static const struct file_operations gb_debugfs_dp_ops = {
	.open = GB02FUNC93,
	.read = seq_read,
	.write = GB02FUNC98,
};

int GB02FUNC115(struct GB02STR155 *gb_dev)
{
	struct drm_minor *minor = gb_dev->drm_dev->primary;
	struct dentry *root;

	root = debugfs_create_dir("gb_dbg", minor->debugfs_root);
	if (!root) {
		printk(KERN_INFO "Failed to create gb debugfs directory\n");
		return -1;
	}

	gb_dev->gb_dbg_info.dentry_root = root;
	/*dc debug*/
	gb_dev->gb_dbg_info.dentry_file_dc = debugfs_create_file("dc", 0644,
					  root, gb_dev, &gb_debugfs_dc_ops);
	if (!gb_dev->gb_dbg_info.dentry_file_dc) {
		printk(KERN_INFO "Failed to create gb dentry_file dc!\n");
	    return -1;
	}
	/*dp debug*/
	gb_dev->gb_dbg_info.dentry_file_dp = debugfs_create_file("dp", 0644,
					  root, gb_dev, &gb_debugfs_dp_ops);
	if (!gb_dev->gb_dbg_info.dentry_file_dp) {
		printk(KERN_INFO "Failed to create gb dentry_file dp!\n");
		return -1;
	}

	return 0;
}

void GB02FUNC129(struct GB02STR155 *gb_dev)
{
	debugfs_remove_recursive(gb_dev->gb_dbg_info.dentry_root);
	gb_dev->gb_dbg_info.dentry_root = NULL;
	if (gb_dev->gb_dbg_info.modeinfo != NULL) {
		kfree(gb_dev->gb_dbg_info.modeinfo);
		gb_dev->gb_dbg_info.modeinfo = NULL;
	}
}

int GB02FUNC131(struct drm_minor *minor)
{
	struct dentry *root;

	root = debugfs_create_dir("gb_dbg_test", minor->debugfs_root);
	if (!root) {
		printk(KERN_INFO "Failed to create gb debugfs directory\n");
		return -1;
	}

	return 0;
}

