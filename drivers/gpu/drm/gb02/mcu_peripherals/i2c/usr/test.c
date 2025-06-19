// SPDX-License-Identifier: GPL-2.0-only
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define GB02MAC1185 'I'
#define I2C_MCU_CMD_I2C0 _IOW(GB02MAC1185, 1u, int)
#define I2C_MCU_CMD_I2C1 _IOW(GB02MAC1185, 2u, int)
#define I2C_MCU_CMD_I2C2 _IOW(GB02MAC1185, 3u, int)
#define I2C_MCU_CMD_I2C3 _IOW(GB02MAC1185, 4u, int)
#define I2C_MCU_CMD_I2C0_RD _IOW(GB02MAC1185, 5u, int)
#define I2C_MCU_CMD_I2C0_WR _IOW(GB02MAC1185, 6u, int)

int main(int argc, char *argv[])
{
	int fd, i, arg, ret, req;
	unsigned int data[10];

	if (argc < 2)
		return -1;

	fd = open("/dev/gbfpga_i2c", O_RDWR);
	if (fd == -1) {
		printf("open gbfpgai2c fail");
		return -1;
	}

	req = atoi(argv[1]);
	switch (req) {
	case 0:
		printf("Start to test I2C0!case:%d\n", req);
		ret = ioctl(fd, I2C_MCU_CMD_I2C0, NULL);
		break;
	case 1:
		printf("Start to test I2C1!case:%d\n", req);
		ret = ioctl(fd, I2C_MCU_CMD_I2C1, NULL);
		break;
	case 2:
		printf("Start to test I2C2!case:%d\n", req);
		ret = ioctl(fd, I2C_MCU_CMD_I2C2, NULL);
		break;
	case 3:
		printf("Start to test I2C3!case:%d\n", req);
		ret = ioctl(fd, I2C_MCU_CMD_I2C3, NULL);
		break;
	case 4:
		printf("Start to test I2C0 read!case:%d\n", req);
		ret = ioctl(fd, I2C_MCU_CMD_I2C0_RD, NULL);
		break;
	case 5:
		printf("Start to test I2C0 write!case:%d\n", req);
		ret = ioctl(fd, I2C_MCU_CMD_I2C0_WR, NULL);
		break;
	default:
		printf("cmd invalid!%d\n", req);
		break;
	}

	if (ret < 0) {
		printf("testcase %x ioctrl failed with code %d\n", req,
			   ret);
		return -1;
	}

	close(fd);
	return 0;
}
