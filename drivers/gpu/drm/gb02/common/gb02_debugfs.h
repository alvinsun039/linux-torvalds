/*
 * Copyright (C) Sietium Electronics Co.Ltd
 */

#ifndef	__GB_DEBUGFS_H__
#define	__GB_DEBUGFS_H__

#define GB02MAC240
#define GB02MAC242 4

/*dc info*/
#define GB02MAC245			285 /*de:0x00000~0x00474*/
#define GB02MAC247			85 /*dc:0x08000~0x08154*/
#define GB02MAC249			6  /*dc:0x0c000~0x0c018*/
#define GB02MAC251			11 /*dc:0x0FFD0~0x0fffc*/
#define GB02MAC252			4 /*dcs:0x10000~0x01ffff*/
#define GB02MAC254			0x00474
#define GB02MAC256			0x08000
#define GB02MAC258		0x0c000
#define GB02MAC260		0x0ffd0
#define GB02MAC262			0x0fffc
#define GB02MAC264		0x10000
#define GB02MAC266			0x318
#define GB02MAC268			0x31c

/*dc cmd*/
#define GB02MAC270			0x01
#define GB02MAC271			0x02
#define GB02MAC272			0x03
#define GB02MAC273		0x04
#define GB02MAC274		0x05
#define GB02MAC275		0x06

#define DC_FB_TEST_FILE			"/data/1920_1080.argb"
#define DC_REG_DATA_FILE		"/data/dc_reg_data.txt"
#define DC_FB_READ_FILE			"/data/dc_fb_data.argb"
#define DC_REG_WRITE_RECORD		"/data/dc_reg_w_record.txt"

/*dp*/
#define GB02MAC276			0x01
#define GB02MAC277			0x02
#define GB02MAC278			0x03
#define GB02MAC279			0x04
#define GB02MAC280		0x05
#define GB02MAC281		0x06

#define GB02MAC282			1
#define GB02MAC283			2
#define GB02MAC284			4
#define GB02MAC285			0x3


enum control_param {
	GB_DBG_CMD = 0,
	GB_DBG_PORT,
	GB_DBG_OFFSET,
	GB_DBG_VALUE,
	GB_DBG_MAX,
};

struct GB02STR19 {
	int vdisplay;
	int hdisplay;
	unsigned int pitch;
};
struct GB02STR21 {
	u32 value;
	u32 fb_offset;
	u32 crtc;
};
struct GB02STR22 {
    /* for debugfs */
	struct dentry *dentry_root;
	struct dentry *dentry_file_dc;
	struct dentry *dentry_file_dp;
	struct GB02STR19 *modeinfo;
	u32  r_val_dc;
	u32  r_val_dp;
};
int GB02FUNC115(struct GB02STR155 *gb_dev);
void GB02FUNC129(struct GB02STR155 *gb_dev);
int GB02FUNC131(struct drm_minor *minor);

#endif
