// SPDX-License-Identifier: GPL-2.0-only
#include <getopt.h>
#include "pwmconfig.h"
unsigned int pwmchip;
unsigned char pwmchannel;
unsigned long pwmperiod;
unsigned int pwmduty;

static void GB02FUNC1548(const char *prog)
{
	printf("Usage: %s [-pcPD]\n", prog);
	puts("  -p --pwmchip	pwmchip to use (default pwmchip0)\n"
	     "  -c --pwmchannel	pwmchannel[0-3]\n"
	     "  -P --period	pwmperiod (nsec)\n"
	     "  -D --duty_cycle	duty(nsec)\n");
}
static void GB02FUNC1551(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{"pwmchip", 1, 0, 'p'},
			{"pwmchannel", 1, 0, 'c'},
			{"pwnperiod", 1, 0, 'P'},
			{"pwmduty", 1, 0, 'D'},
			{NULL, 0, 0, 0},
		};
		int c;

		c = getopt_long(argc, argv, "p:c:P:D:",
				lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'p':
			pwmchip = atoi(optarg);
			break;
		case 'c':
			pwmchannel = atoi(optarg);
			break;
		case 'P':
			pwmperiod = atoi(optarg);
			break;
		case 'D':
			pwmduty = atoi(optarg);
			break;
		default:
			GB02FUNC1548(argv[0]);
		}
	}
}
/* PWM export */
static int GB02FUNC1556(unsigned int pwmchip, unsigned char pwmno)
{
	int fd;
	unsigned char chippath[64];
	unsigned char chan[4];
	int len;

	sprintf(chippath, SYSFS_PWM_DIR "/pwmchip%d/export", pwmchip);
	len = sprintf(chan, "%d", pwmno);
	fd = open(chippath, O_WRONLY);
	if (fd < 0) {
		printf("\nFailed export %s pwm%d\n", chippath, pwmno);
		return -1;
	}
	write(fd, chan, len);
	close(fd);
	printf("export %s pwm%d\n", chippath, pwmno);
	return 0;
}

/* PWM unexport */
static int GB02FUNC1559(unsigned int pwmchip, unsigned char pwmno)
{
	int fd;
	unsigned char chippath[64];
	unsigned char chan[4];
	int len;

	len = sprintf(chan, "%d", pwmno);
	sprintf(chippath, SYSFS_PWM_DIR "/pwmchip%d/unexport", pwmchip);

	fd = open(chippath, O_WRONLY);
	if (fd < 0) {
		printf("\nFailed unexport PWM%d_%d\n", pwmchip, pwmno);
		return -1;
	}
	write(fd, chan, len);
	close(fd);
	printf("unexport PWM%d_%d\n", pwmchip, pwmno);
	return 0;
}

/* PWM configuration */
static int pwm_config(unsigned int pwmchip, unsigned char pwmchan, unsigned int period, unsigned int duty_cycle)
{
	int fd, len_p, len_d;
	char buf_p[GB02MAC2043];
	char buf_d[GB02MAC2043];
	unsigned char periodpath[64];
	unsigned char duty_cyclepath[64];

	sprintf(periodpath, SYSFS_PWM_DIR "/pwmchip%d/pwm%d/period", pwmchip, pwmchan);
	sprintf(duty_cyclepath, SYSFS_PWM_DIR "/pwmchip%d/pwm%d/duty_cycle", pwmchip, pwmchan);

	len_p = snprintf(buf_p, sizeof(buf_p), "%d", period);
	len_d = snprintf(buf_d, sizeof(buf_d), "%d", duty_cycle);

	/* set pwm period */
	fd = open(periodpath, O_WRONLY);
	if (fd < 0) {
		printf("\nFailed set %s period\n", periodpath);
		return -1;
	}

	write(fd, buf_p, len_p);
	close(fd);

	printf("set %s:%d\n", periodpath, period);
	/* set pwm duty cycle */
	fd = open(duty_cyclepath, O_WRONLY);
	if (fd < 0) {
		printf("\nFailed set %s\n", duty_cyclepath);
		return -1;
	}

	write(fd, buf_d, len_d);

	close(fd);
	printf("set %s:%d\n", duty_cyclepath, duty_cycle);
	return 0;
}

/* PWM enable */
static int pwm_enable(unsigned int pwmchip, unsigned char pwmchan, unsigned char enable)
{
	int fd;
	unsigned char chippath[64];
	unsigned char buff_enbale[4];
	int len;

	len = sprintf(buff_enbale, "%d", enable);
	sprintf(chippath, SYSFS_PWM_DIR "/pwmchip%d/pwm%d/enable", pwmchip, pwmchan);
	fd = open(chippath, O_WRONLY);
	if (fd < 0) {
		printf("\nFailed %s\n", chippath);
		return -1;
	}
	write(fd, buff_enbale, len);
	close(fd);
	printf(enable == 1 ? "enable %s\n" : "disable %s\n", chippath);
	return 0;
}
int main(int argc, char **argv)
{
	unsigned char pwmchan;
	unsigned int chipnum;
	unsigned long myperiod = 100000; // period 设置为100ms
	unsigned int myduty = 50000;

	printf("%d %s\n", argc, *argv);

	/* handle (optional) flags first */
	if (argc < 3) {
		GB02FUNC1548(argv[0]);
		return -1;
	}
	GB02FUNC1551(argc, argv);
	chipnum = pwmchip;
	pwmchan = pwmchannel;
	myperiod = pwmperiod;
	myduty = pwmduty;
	/*export corresponding PWM Channel*/
	if (GB02FUNC1556(chipnum, pwmchan) < 0) {
		printf("PWM export error!\n");
		return (-1);
	}
	/* set period and duty cycle time in ns */
	if (pwm_config(chipnum, pwmchan, myperiod, myduty) < 0) {
		printf("PWM configure error!\n");
		return (-1);
	}
	/* enable corresponding PWM Channel */
	if (pwm_enable(chipnum, pwmchan, 1) < 0) {
		printf("PWM enable error!\n");
		return (-1);
	}
	printf("successfully enabled with period - %ldms, duty cycle - %dms\n", myperiod / 1000, myduty / 1000);
	return 0;
}
