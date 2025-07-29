/*
 * Copyright (C) Xi'an Sietium Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef	__GB_GPUINFO_H__
#define	__GB_GPUINFO_H__
#define	GB02MAC517	128
#define	GB02MAC518	2
#define	GB02MAC519	32

#define	GB02MAC520	11072962
#define	GB02MAC521	5704253
#define	GB02MAC522	80905

//#define FULL_GPUINFO

struct GB02STR63 {
	struct GB02STR47 *filp;
	int frame_num;
	long start_time;
	long vpu_ave_time;
	long resv_start_time;
	long resv_time;
	long resv_free_ave_time;
	bool stop_flag;
};

struct gpu_info {
	char devmodel[GB02MAC517];
	char devname[GB02MAC517];
	char product[GB02MAC517];
	unsigned long long vram;
	int  clk;
	int	 bitwidth;
	int	 bandwidth;
	char driver_info[GB02MAC517];
	int	 vender_id;
	int	 device_id;
	char bus_info[GB02MAC517];
	unsigned int board_type ;
	unsigned long vpu_used;
	unsigned long vpu_total;
	unsigned long gpu_used;
	unsigned long gpu_total;
	int temp;
	struct GB02STR63 vpu_time[GB02MAC519];
};

struct gpu_info  *GB02FUNC314(void);

int GB02FUNC317(struct gpu_info *gbinfo);

void GB02FUNC322(void);
int GB02FUNC334(struct GB02STR39 *gb_dev);
void GB02FUNC336(struct GB02STR39 *gb_dev);
#endif
