// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define GB02MAC544 'P'
#define PINCTRL_MCU_CMD_GPIO40 _IOW(GB02MAC544, 1u, int)
#define PINCTRL_MCU_CMD_GPIO41 _IOW(GB02MAC544, 2u, int)
#define PINCTRL_MCU_CMD_GPIO42 _IOW(GB02MAC544, 3u, int)
#define PINCTRL_MCU_CMD_GPIO43 _IOW(GB02MAC544, 4u, int)

int main(int argc, char *argv[])
{
	int fd, i, arg, ret, req;
	unsigned int data[10];

	if (argc < 2)
		return -1;

	fd = open("/dev/gbfpgagpio", O_RDWR);
	if (fd == -1) {
		printf("open gbfpgagpio fail");
		return -1;
	}

	req = atoi(argv[1]);
	switch (req) {
	case 0:
		printf("Start to test GPIO(GPIONO:40)!case:%d\n", req);
		ret = ioctl(fd, PINCTRL_MCU_CMD_GPIO40, NULL);
		break;
	case 1:
		printf("Start to test GPIO(GPIONO:41)!case:%d\n", req);
		ret = ioctl(fd, PINCTRL_MCU_CMD_GPIO41, NULL);
		break;
	case 2:
		printf("Start to test GPIO(GPIONO:42)!case:%d\n", req);
		ret = ioctl(fd, PINCTRL_MCU_CMD_GPIO42, NULL);
		break;
	case 3:
		printf("Start to test GPIO(GPIONO:43)!case:%d\n", req);
		ret = ioctl(fd, PINCTRL_MCU_CMD_GPIO43, NULL);
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
